// -------------------------------------------------------------
// Copyright (C) 2016- Adam Petrone
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//		* Redistributions of source code must retain the above copyright notice,
//		this list of conditions and the following disclaimer.

//		* Redistributions in binary form must reproduce the above copyright notice,
//		this list of conditions and the following disclaimer in the documentation
//		and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//		 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------
#include <runtime/audio_mixer.h>
#include <runtime/assets/asset_sound.h>

#include <core/array.h>
#include <core/logging.h>
#include <core/mathlib.h>
#include <core/linearfreelist.h>

#include <platform/audio.h>
#include <platform/platform.h>


// enable this to add locks in with mixing and audio functions.
//#define AUDIO_USE_LOCK 1

namespace gemini
{
	namespace audio
	{
		struct SoundInstance
		{
			enum class State
			{
				// An invalid instance.
				Inactive,

				// A handle which has finished playing.
				Stopped,

				// An instance which is currently playing.
				Playing
			};

			assets::Sound* sound_asset;
			uint64_t frames_buffered;
			uint64_t total_frames;
			uint64_t sample_count_started; // sample count this started playing
			State state;
		};

		//
		// audio_mixer state
		size_t total_active_sounds = 0;

		LinearFreeList<SoundInstance>* sound_list;

		platform::Mutex* audio_lock = nullptr;

		float master_gain = 1.0f;

		int16_t generate_sine_wave(float& t, uint32_t frequency, int32_t advance_t)
		{
			const float sine_period = mathlib::PI * 2.0f;
			const float short_multiplier = (SHRT_MAX / 2.0f);
			const float wave_period = (AUDIO_FREQUENCY_HZ / static_cast<float>(frequency));

			float s = sinf(t);

			// increment t
			if (advance_t)
			{
				t += static_cast<float>((1.0f * sine_period) / wave_period);

				// wrap to avoid glitches
				if (t > sine_period)
				{
					t -= sine_period;
				}
			}

			return static_cast<int16_t>(s * short_multiplier);
		} // generate_sine_wave

		void audio_fill_buffer(void* data, size_t frames_available, size_t sample_rate_hz, void* context)
		{
			int16_t* output = reinterpret_cast<int16_t*>(data);
			uint64_t current_frame = platform::audio_frame_position();

#if defined(AUDIO_USE_LOCKS)
			platform::mutex_lock(audio_lock);
#endif

			for (size_t frame = 0; frame < frames_available; ++frame)
			{
				const size_t frame_index = frame * AUDIO_MAX_OUTPUT_CHANNELS;

				// loop through all active instances and sum the output for this frame
				float channels[AUDIO_MAX_OUTPUT_CHANNELS] = { 0.0f, 0.0f };

				for (size_t index = 0; index < sound_list->size(); ++index)
				{
					if (sound_list->is_valid(index))
					{
						SoundInstance* instance = sound_list->from_handle(index);
						if (instance->state != SoundInstance::State::Playing)
						{
							continue;
						}

						// The sound will finish buffering before it finishes playing. We need to catch this.
						const bool is_finished_playing = (current_frame - instance->sample_count_started) >= instance->total_frames;

						if (is_finished_playing)
						{
							// return to the free list
							sound_list->release(index);
							--total_active_sounds;
							continue;
						}

						// check if we can continue buffering this instance
						if (instance->frames_buffered < instance->total_frames)
						{
							const size_t instance_frame = (instance->frames_buffered * audio::InMemoryChannelCount);
							assert(instance->sound_asset);
							float current_sample[2];
							instance->sound_asset->get_frame(instance->frames_buffered, current_sample);
							channels[0] += current_sample[0];
							channels[1] += current_sample[1];
							++instance->frames_buffered;
						}
					}
				}

				// prevent clipping
				channels[0] = glm::clamp(channels[0], -1.0f, 1.0f);
				channels[1] = glm::clamp(channels[1], -1.0f, 1.0f);

				output[frame_index + 0] = static_cast<int16_t>(channels[0] * InMemorySampleValueMax * master_gain);
				output[frame_index + 1] = static_cast<int16_t>(channels[1] * InMemorySampleValueMax * master_gain);
			}

#if defined(AUDIO_USE_LOCKS)
			platform::mutex_unlock(audio_lock);
#endif
		} // audio_fill_buffer
	} // namespace audio

} // namespace gemini

#include <runtime/filesystem.h>



#if 0
	// generate a sine wave
	int16_t* buffer = reinterpret_cast<int16_t*>(data);
	for (uint32_t frame = 0; frame < frames_available; ++frame)
	{
		uint32_t index = (frame * 2);
		buffer[index + 0] = generate_sine_wave(t_sin, 440, 0);
		buffer[index + 1] = generate_sine_wave(t_sin, 440, 1);
	}
#endif

#if 0
	// generate a square wave
	const float volume = 0.5f;

	const uint32_t frames_per_period = sample_rate_hz / 256;
	const uint32_t half_period = frames_per_period / 2;
	const uint32_t total_frames = frames_available;

	int16_t* buffer = reinterpret_cast<int16_t*>(data);
	for (uint32_t frame = 0; frame < total_frames; ++frame)
	{
		short value = ((time_period / half_period) % 2) ? 3000 : -3000;
		uint32_t index = frame * 2;
		buffer[index + 0] = static_cast<short>(value * volume);
		buffer[index + 1] = static_cast<short>(value * volume);

		time_period += 1;
		if (time_period > frames_per_period)
		{
			time_period -= frames_per_period;
		}
	}
#endif

namespace gemini
{
	namespace audio
	{
		gemini::Allocator* audio_allocator = nullptr;

		SoundHandle_t play_sound(assets::Sound* sound, int32_t repeats)
		{
#if defined(AUDIO_USE_LOCK)
			platform::mutex_lock(audio_lock);
#endif

			SoundHandle_t handle = sound_list->acquire();
			SoundInstance* instance = sound_list->from_handle(handle);
			assert(instance);

			// only if the sound wasn't previously playing.
			instance->state = SoundInstance::State::Playing;
			instance->frames_buffered = 0;
			instance->sample_count_started = platform::audio_frame_position();
			instance->sound_asset = sound;
			instance->total_frames = sound->get_total_frames();
			++total_active_sounds;

#if defined(AUDIO_USE_LOCK)
			platform::mutex_unlock(audio_lock);
#endif

			return handle;
		}

		void stop_sound(SoundHandle_t handle)
		{
			SoundInstance* instance = sound_list->from_handle(handle);
			assert(instance);
			instance->state = SoundInstance::State::Inactive;
			instance->frames_buffered = 0;
			instance->sound_asset = nullptr;

			--total_active_sounds;
		}

		void stop_all_sounds()
		{
#if defined(AUDIO_USE_LOCK)
			platform::mutex_lock(audio_lock);
#endif
			for (size_t index = 0; index < sound_list->size(); ++index)
			{
				if (sound_list->is_valid(index))
				{
					SoundInstance* instance = sound_list->from_handle(index);
					if (instance->state == SoundInstance::State::Playing)
					{
						instance->state = SoundInstance::State::Stopped;
						instance->frames_buffered = 0;
						instance->sample_count_started = 0;
						instance->sound_asset = nullptr;
					}
					else
					{
						instance->state = SoundInstance::State::Inactive;
					}
				}
			}
			total_active_sounds = 0;

#if defined(AUDIO_USE_LOCK)
			platform::mutex_unlock(audio_lock);
#endif
		}

		void set_master_volume(float new_volume)
		{
			master_gain = new_volume;
		}

		float get_master_volume()
		{
			return master_gain;
		}

		void startup(gemini::Allocator& allocator)
		{
			audio_allocator = &allocator;

#if defined(AUDIO_USE_LOCK)
			audio_lock = platform::mutex_create();
#endif
			sound_list = MEMORY2_NEW(allocator, LinearFreeList<SoundInstance>)(allocator);

			platform::audio_startup();

			Array<platform::audio_device*> devices;
			audio_enumerate_devices(devices);

			//LOGV("total audio devices: %i\n", devices.size());
			//for (size_t index = 0; index < devices.size(); ++index)
			//{
			//	const platform::audio_device& device = devices[index];
			//	LOGV("device: %s\n", device.name());
			//}


			// try to open the device
			platform::Result res = audio_open_output_device(devices[0]);
			assert(res.succeeded());

			platform::audio_set_callback(audio_fill_buffer, nullptr);
		}

		void shutdown()
		{
			stop_all_sounds();

#if defined(AUDIO_USE_LOCK)
			platform::mutex_lock(audio_lock);
#endif
			platform::audio_close_output_device();

			platform::audio_shutdown();

#if defined(AUDIO_USE_LOCK)
			platform::mutex_unlock(audio_lock);
#endif

			// clear free lists and used lists
			MEMORY2_DELETE(*audio_allocator, sound_list);

#if defined(AUDIO_USE_LOCK)
			platform::mutex_destroy(audio_lock);
#endif
		}

		size_t get_total_playing_sounds()
		{
			return total_active_sounds;
		}

		float get_total_time_seconds(SoundHandle_t handle)
		{
			SoundInstance* instance = sound_list->from_handle(handle);
			assert(instance);

			return (instance->total_frames / static_cast<float>(AUDIO_FREQUENCY_HZ));
		}

		float get_current_playhead(SoundHandle_t handle)
		{
			if (sound_list->is_valid(handle))
			{
				SoundInstance* instance = sound_list->from_handle(handle);
				assert(instance);
				assert(instance->state != SoundInstance::State::Stopped);

				uint64_t current_frame = platform::audio_frame_position();
				return (current_frame - instance->sample_count_started) / static_cast<float>(AUDIO_FREQUENCY_HZ);
			}

			return 0.0f;
		}
	} // namespace audio
} // namespace gemini
