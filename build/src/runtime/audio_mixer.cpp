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

#include <core/array.h>
#include <core/logging.h>

#include <platform/audio.h>
#include <platform/platform.h>

#include <core/mathlib.h>
#include <core/linearfreelist.h>

#include <core/atomic.h>


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

				// An allocated, valid handle which is not yet playing.
				Loaded,

				// An instance which is currently playing.
				Playing
			};

			uint64_t frames_buffered;
			uint64_t total_frames;
			uint64_t sample_count_started; // sample count this started playing
			struct SoundInstance* next;
			Array<InMemorySampleType> pcmdata;
			State state;
			SoundHandle handle;
		};

		// The following assumptions are made:
		// For loaded sounds; they must be TWO-CHANNEL STEREO sounds.
		// Sounds are 16-bit.
		// Frequency is 44.1 kHz.

		//
		// audio_mixer state
		const size_t AUDIO_FREQUENCY_HZ				= 44100;
		const size_t AUDIO_MAX_OUTPUT_CHANNELS		= 2;
		const size_t AUDIO_BITS_PER_SAMPLE			= 16;

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
							const size_t instance_frame = (instance->frames_buffered * 2);
							channels[0] += instance->pcmdata[instance_frame + 0];
							channels[1] += instance->pcmdata[instance_frame + 1];
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
		SoundHandle load_sound(const char* asset_name)
		{
			core::filesystem::IFileSystem* fs = core::filesystem::instance();
			assert(fs->file_exists(asset_name));

			SoundInstance* instance;
			SoundHandle handle = sound_list->acquire();

			instance = sound_list->from_handle(handle);
			assert(instance);

			load_wave(instance->pcmdata, asset_name);

			instance->total_frames = (instance->pcmdata.size() / 2);
			instance->state = SoundInstance::State::Loaded;
			instance->frames_buffered = 0;

			return handle;
		}

		void play_sound(SoundHandle handle, int32_t repeats)
		{
			SoundInstance* instance = sound_list->from_handle(handle);
			assert(instance);

			if (instance->state != SoundInstance::State::Playing)
			{
				// only if the sound wasn't previously playing.
				instance->state = SoundInstance::State::Playing;
				instance->frames_buffered = 0;
				instance->sample_count_started = platform::audio_frame_position();

				++total_active_sounds;
			}
		}

		void stop_sound(SoundHandle handle)
		{
			SoundInstance* instance = sound_list->from_handle(handle);
			assert(instance);
			instance->state = SoundInstance::State::Inactive;
			instance->frames_buffered = 0;

			--total_active_sounds;
		}

		void stop_all_sounds()
		{
			for (size_t index = 0; index < sound_list->size(); ++index)
			{
				if (sound_list->is_valid(index))
				{
					SoundInstance* instance = sound_list->from_handle(index);
					if (instance->state == SoundInstance::State::Playing)
					{
						instance->state = SoundInstance::State::Loaded;
						instance->frames_buffered = 0;
						instance->sample_count_started = 0;
					}
					else
					{
						instance->state = SoundInstance::State::Inactive;
					}
				}
			}
			total_active_sounds = 0;
		}

		void set_master_volume(float new_volume)
		{
			master_gain = new_volume;
		}

		float get_master_volume()
		{
			return master_gain;
		}

		void startup()
		{
#if defined(AUDIO_USE_LOCK)
			audio_lock = platform::mutex_create();
#endif
			sound_list = MEMORY_NEW(LinearFreeList<SoundInstance>, core::memory::global_allocator());

			platform::audio_startup();

			Array<platform::audio_device> devices;
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
			platform::audio_close_output_device();

			platform::audio_shutdown();

			// clear free lists and used lists
			MEMORY_DELETE(sound_list, core::memory::global_allocator());

#if defined(AUDIO_USE_LOCK)
			platform::mutex_destroy(audio_lock);
#endif
		}

		size_t get_total_playing_sounds()
		{
			return total_active_sounds;
		}
	} // namespace audio
} // namespace gemini


namespace gemini
{
	// code for reading and writing WAVE files
	// http://www-mmsp.ece.mcgill.ca/documents/audioformats/wave/wave.html
	// http://soundfile.sapp.org/doc/WaveFormat/
#define MAKE_RIFF_CODE(a) static_cast<uint32_t>(((a[0]) << 0 | (a[1]) << 8 | (a[2]) << 16 | (a[3]) << 24))
#ifndef WAVE_FORMAT_PCM
	const size_t WAVE_FORMAT_PCM = 0x001;
	const size_t WAVE_FORMAT_IEEE_FLOAT = 0x0003;
	const size_t WAVE_FORMAT_ALAW = 0x0006;
	const size_t WAVE_FORMAT_MULAW = 0x0007;
	const size_t WAVE_FORMAT_EXTENSIBLE = 0xFFFE;
#endif

	const uint32_t RIFF_CHUNK_ID = MAKE_RIFF_CODE("RIFF");
	const uint32_t RIFF_WAVE_FORMAT = MAKE_RIFF_CODE("WAVE");
	const uint32_t WAVE_FORMAT_CHUNK_ID = MAKE_RIFF_CODE("fmt ");
	const uint32_t WAVE_DATA_CHUNK_ID = MAKE_RIFF_CODE("data");

	// riff header
	struct wave_chunk_descriptor
	{
		uint32_t chunk_id; // should be 'RIFF'
		uint32_t chunk_size;
		uint32_t format; // should be 'WAVE'

		uint32_t advance_size() const
		{
			return 8 + 4;
		}
	};

	struct wave_format_chunk
	{
		uint32_t chunk_id; // should be 'fmt '
		uint32_t chunk_size; // should be 16, 18, or 40.
		uint16_t format_code; // format tag
		uint16_t total_channels; // number of interleaved channels
		uint32_t sample_rate; // blocks per second
		uint32_t data_rate; // avg bytes per sec
		uint16_t block_align; // data block size (bytes)
		uint16_t bits_per_sample;
		uint16_t extended_size; // size of extension (0 or 22)
		uint16_t valid_bits_per_sample; // number of valid bits
		uint32_t channel_mask; // speaker position mask
		uint8_t subformat[16]; // GUID including the data format code

		uint32_t advance_size() const
		{
			return 8 + chunk_size;
		}
	};

	struct wave_data_chunk
	{
		uint32_t chunk_id; // should be 'data'
		uint32_t chunk_size; // == num_samples * num_channels * bits_per_sample / 8

		uint32_t advance_size() const
		{
			return 8;
		}
	};

	// TODO: This could fail; add error checking/codes.
	void load_wave(Array<audio::InMemorySampleType>& samples, const char* path)
	{
		Array<unsigned char> filecontents;
		core::filesystem::instance()->virtual_load_file(filecontents, path);

		LOGV("[WAVE] loaded %s\n", path);
		LOGV("[WAVE] file size: %i bytes\n", filecontents.size());


		unsigned char* wavedata = static_cast<unsigned char*>(&filecontents[0]);
		wave_chunk_descriptor* desc = reinterpret_cast<wave_chunk_descriptor*>(wavedata);
		if (desc->chunk_id != RIFF_CHUNK_ID)
		{
			LOGV("[WAVE] Is not a valid RIFF file\n");
			return;
		}

		if (desc->format != RIFF_WAVE_FORMAT)
		{
			LOGV("[WAVE] Is not a valid WAVE file format\n");
			return;
		}

		wave_format_chunk* format = reinterpret_cast<wave_format_chunk*>(
			reinterpret_cast<char*>(desc) + desc->advance_size()
		);
		if (format->chunk_id != WAVE_FORMAT_CHUNK_ID)
		{
			LOGV("[WAVE] Expected WAVE 'fmt ' chunk!\n");
			return;
		}

		// We only support PCM.
		assert(format->format_code == WAVE_FORMAT_PCM);

		// Only support our sample rate of 44.1KHz.
		assert(format->sample_rate == audio::AUDIO_FREQUENCY_HZ);

		// TODO: Support mono and stereo sounds.
		//assert(format->total_channels == 1 || format->total_channels == 2);

		// Currently, only stereo WAVE files are supported.
		assert(format->total_channels == 2);

		wave_data_chunk* data = reinterpret_cast<wave_data_chunk*>(
			reinterpret_cast<char*>(format) + format->advance_size()
		);

		if (data->chunk_id != WAVE_DATA_CHUNK_ID)
		{
			LOGV("[WAVE] Expected WAVE 'data' chunk!\n");
			return;
		}

		// length of the file in seconds: data->chunk_size / format->data_rate
		const bool has_pad_byte = ((data->chunk_size + 1) & ~1) == 0;

		const uint32_t total_samples = (data->chunk_size / format->block_align);

		samples.resize(total_samples * format->total_channels);
		int16_t* sample_data = reinterpret_cast<int16_t*>(reinterpret_cast<char*>(data) + data->advance_size());
		for (size_t sample = 0; sample < total_samples * 2; ++sample)
		{
			samples[sample] = glm::clamp(sample_data[sample] / audio::InMemorySampleValueMax, -1.0f, 1.0f);
		}
	} // load_wave
} // namespace gemini