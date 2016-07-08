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
//
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

#include <platform/audio.h>
#include <platform/platform_internal.h>

#include <core/atomic.h>
#include <core/logging.h>

#if defined(GEMINI_ENABLE_AUDIO)

// use the newer alsa api
#define ALSA_PCM_NEW_HW_PARAMS_API

// The old sys/asoundlib is deprecated in favor of this.
#include <alsa/asoundlib.h>


// /usr/share/sounds/alsa/Front_Center.wav


// REFERENCES:
// http://www.linuxjournal.com/article/6735?page=0,1
// http://home.roadrunner.com/~jgglatt/tech/aplay.htm
// http://alsa.opensrc.org/HowTo_Asynchronous_Playback
// http://www.alsa-project.org/alsa-doc/alsa-lib/_2test_2pcm__min_8c-example.html
// http://www.alsa-project.org/alsa-doc/alsa-lib/_2test_2pcm_8c-example.html

namespace platform
{
	struct alsa_audio_device
	{
		int32_t device_number;
		int32_t sub_device_number;

		alsa_audio_device()
			: device_number(-1)
			, sub_device_number(-1)
		{
		}
	}; // alsa_audio_state

	struct alsa_audio_state
	{
		uint32_t sample_rate_hz;
		snd_pcm_t* handle;
		audio_sound_callback audio_pull_callback;
		void* audio_pull_context;
		Array<audio_device*, platform::PlatformAllocatorType> devices;
		snd_pcm_uframes_t period_frames;
		char* sound_buffer;
		size_t sound_buffer_size;
		snd_pcm_sframes_t frame_delay;
		uint64_t frames_written;

		uint32_t running_thread;
		Thread* thread;

		alsa_audio_state()
			: handle(nullptr)
			, devices(16, get_platform_allocator())
			, audio_pull_callback(nullptr)
			, sound_buffer(nullptr)
			, sound_buffer_size(0)
			, frame_delay(0)
			, frames_written(0)
			, thread(nullptr)
		{
		}
	}; // alsa_audio_state

	static int check_alsa_error(int result, const char* action)
	{
		if (result < 0)
		{
			LOGW("Failed on '%s', error: '%s'\n", action,
				snd_strerror(result)
			);
			assert(0);
		}

		return result;
	}

	void dump_alsa_params()
	{
		LOGV("ALSA library version: %s\n", SND_LIB_VERSION_STR);

		LOGV("PCM stream types:\n");
		for (int32_t index = 0; index < SND_PCM_STREAM_LAST; ++index)
		{
			LOGV("\t[%i] %s\n", index, snd_pcm_stream_name((snd_pcm_stream_t)index));
		}

		LOGV("PCM access types:\n");
		for (int32_t index = 0; index < SND_PCM_ACCESS_LAST; ++index)
		{
			LOGV("\t[%i] %s\n", index, snd_pcm_access_name((snd_pcm_access_t)index));
		}

		LOGV("PCM formats:\n");
		for (int32_t index = 0; index < SND_PCM_FORMAT_LAST; ++index)
		{
			if (snd_pcm_format_name((snd_pcm_format_t)index) != NULL)
			{
				LOGV("\t[%i] %s %s\n", index,
					snd_pcm_format_name((snd_pcm_format_t)index),
					snd_pcm_format_description((snd_pcm_format_t)index));
			}
		}

		LOGV("PCM subformats:\n");
		for (int32_t index = 0; index < SND_PCM_SUBFORMAT_LAST; ++index)
		{
			if (snd_pcm_subformat_name((snd_pcm_subformat_t)index) != NULL)
			{
				LOGV("\t[%i] %s %s\n", index,
					snd_pcm_subformat_name((snd_pcm_subformat_t)index),
					snd_pcm_subformat_description((snd_pcm_subformat_t)index));
			}
		}

		LOGV("PCM states:\n");
		for (int32_t index = 0; index < SND_PCM_STATE_LAST; ++index)
		{
			LOGV("\t[%i] %s\n", index, snd_pcm_state_name((snd_pcm_state_t)index));
		}
	}

	void audio_submission_thread(Thread* thread)
	{
		alsa_audio_state* state = reinterpret_cast<alsa_audio_state*>(thread->user_data);

		int32_t last_result = 0;
		while(state->running_thread)
		{
			state->audio_pull_callback(state->sound_buffer, state->period_frames, state->sample_rate_hz, state->audio_pull_context);

			// This is used to keep track of audio frame position.
			state->frames_written += state->period_frames;

			last_result = snd_pcm_writei(state->handle, state->sound_buffer, state->period_frames);
			if (last_result < 0)
			{
				if (last_result == -EPIPE)
				{
					LOGW("[ALSA] buffer underrun\n");
					snd_pcm_prepare(state->handle);
					last_result = -1;
				}
				else
				{
					check_alsa_error(last_result, "snd_pcm_writei");
				}
			}
		}
	} // audio_submission_thread


	alsa_audio_state* audio_state;


	platform::Result audio_enumerate_devices(Array<audio_device*>& devices)
	{
		for (size_t index = 0; index < audio_state->devices.size(); ++index)
		{
			devices.push_back(audio_state->devices[index]);
		}

		return platform::Result::success();
	} // audio_enumerate_devices

	platform::Result audio_open_output_device(audio_device* device)
	{
		assert(device);
		char* memory = reinterpret_cast<char*>(device);
		alsa_audio_device* audio_device = reinterpret_cast<alsa_audio_device*>(memory + sizeof(audio_device));

		// construct the device name
		// NOTE: We could use the plug interface if we want to use ALSA's
		// sample rate conversion.
		char device_name[64] = {0};
		core::str::sprintf(device_name,
			64,
			"hw:%i,%i",
			audio_device->device_number,
			audio_device->sub_device_number
		);

		// TODO@APP: If the 'default' device is opened, instead of a named hw
		// device, there's a significant latency induced. Determine why.
		int32_t error;
		if ((error = snd_pcm_open(&audio_state->handle, device_name, SND_PCM_STREAM_PLAYBACK, 0)) < 0)
		{
			if (error == -ENOENT)
			{
				LOGW("ENOENT: Please make sure you have permissions to access the audio device.\n");
			}
			else
			{
				LOGW("Cannot open the wave output: %s\n", snd_strerror(error));
			}

			return platform::Result::failure(snd_strerror(error));
		}

		// Configure the device
		snd_pcm_hw_params_t* params;
		snd_pcm_hw_params_alloca(&params);

		check_alsa_error(snd_pcm_hw_params_any(audio_state->handle, params), "set defaults");

		// set interleaved mode
		check_alsa_error(snd_pcm_hw_params_set_access(audio_state->handle,
			params,
			SND_PCM_ACCESS_RW_INTERLEAVED),
			"set interleaved"
		);

		// signed 16-bit little-endian format
		check_alsa_error(snd_pcm_hw_params_set_format(audio_state->handle,
			params,
			SND_PCM_FORMAT_S16_LE),
			"set format"
		);

		// set two channels
		check_alsa_error(snd_pcm_hw_params_set_channels(audio_state->handle,
			params,
			2),
			"set channels"
		);

		uint32_t sample_rate;
		int direction;
		snd_pcm_hw_params_get_rate(params, &sample_rate, &direction);
		LOGV("current rate: %i, direction: %i\n", sample_rate, direction);

		// uint32_t buffer_size = 8192;
		// check_alsa_error(snd_pcm_hw_params_set_buffer_size(audio_state->handle,
		// 	params,
		// 	buffer_size),
		// 	"set buffer size"
		// );

		// set sample rate
		audio_state->sample_rate_hz = 44100;
		int32_t subunit_direction = 0;
		check_alsa_error(snd_pcm_hw_params_set_rate_near(audio_state->handle,
			params,
			&audio_state->sample_rate_hz,
			&subunit_direction),
			"set sample rate"
		);

		LOGV("frequency: %i, subunit: %i\n", audio_state->sample_rate_hz, subunit_direction);

		// set period size and buffer
		snd_pcm_uframes_t frames = 128;
		snd_pcm_hw_params_set_period_size_near(audio_state->handle,
			params,
			&frames,
			&subunit_direction
		);

		if ((error = snd_pcm_hw_params(audio_state->handle, params)) < 0)
		{
			LOGW("ALSA: Unable to set one or more parameters on the sound card.\n");
			snd_pcm_close(audio_state->handle);
			return platform::Result::failure(snd_strerror(error));
		}

		// In ALSA parlance:
		// - 1 frame = 1 sample from ALL channels.

		// retrieve the actual period frames.
		check_alsa_error(snd_pcm_hw_params_get_period_size(params,
			&audio_state->period_frames,
			&subunit_direction),
			"get_period_size"
		);
		LOGV("period_frames: %i, subunit: %i\n", audio_state->period_frames, subunit_direction);

		// uint32_t period_time;
		// check_alsa_error(snd_pcm_hw_params_get_period_time(params,
		// 	&period_time,
		// 	&subunit),
		// 	"get_period_time"
		// );
		// LOGV("period_time: %i\n", period_time);

		audio_state->sound_buffer_size = (sizeof(uint16_t) * 2 * audio_state->period_frames);
		audio_state->sound_buffer = static_cast<char*>(
			MEMORY_ALLOC(audio_state->sound_buffer_size,
				get_platform_allocator())
		);

		//
		// Software Parameters
		//
		snd_pcm_sw_params_t* sw_params;
		snd_pcm_sw_params_alloca(&sw_params);


		error = snd_pcm_sw_params_current(audio_state->handle, sw_params);
		check_alsa_error(error, "get current sw params");

		// error = snd_pcm_sw_params_set_start_threshold(
		// 	audio_state->handle,
		// 	sw_params,
		// 	0);
		// check_alsa_error(error, "set start threshold");

		// error = snd_pcm_sw_params_set_tstamp_mode(audio_state->handle,
		// 	sw_params,
		// 	SND_PCM_TSTAMP_ENABLE);
		// check_alsa_error(error, "enable timestamp mode");

		error = snd_pcm_sw_params(audio_state->handle, sw_params);
		check_alsa_error(error, "set sw params");


		memset(audio_state->sound_buffer, 0, audio_state->sound_buffer_size);
		snd_pcm_start(audio_state->handle);

		// prime the buffer.
		snd_pcm_writei(audio_state->handle,
			audio_state->sound_buffer,
			audio_state->period_frames
		);
		snd_pcm_prepare(audio_state->handle);

		// cache start time here.
		snd_pcm_delay(audio_state->handle, &audio_state->frame_delay);
		LOGV("[ALSA] playback delay: %i frames\n", audio_state->frame_delay);

		// create an audio submission thread.
		audio_state->running_thread = 1;
		audio_state->thread = thread_create(audio_submission_thread, audio_state);

		return platform::Result::success();
	} // audio_open_output_device

	void audio_close_output_device()
	{
		assert(audio_state->handle);

		audio_state->running_thread = 0;

		// finish playing sound samples
		snd_pcm_drain(audio_state->handle);

		LOGV("[ALSA] Waiting for audio to stop.\n");

		snd_pcm_state_t state = snd_pcm_state(audio_state->handle);
		while(state == SND_PCM_STATE_RUNNING)
		{
			state = snd_pcm_state(audio_state->handle);
		}

		thread_join(audio_state->thread);
		thread_destroy(audio_state->thread);
		audio_state->thread = nullptr;

		// stop playing everything.
		snd_pcm_drop(audio_state->handle);

		snd_pcm_close(audio_state->handle);
		audio_state->handle = nullptr;

		// free sound buffer memory
		MEMORY_DEALLOC(audio_state->sound_buffer, get_platform_allocator());
	} // audio_close_output_device

	platform::Result audio_startup()
	{
		audio_state = MEMORY_NEW(alsa_audio_state, get_platform_allocator());

		// dump_alsa_params();

		// create the device list
		int32_t error = -1;
		int32_t card_index = -1;

		for (;;)
		{
			snd_ctl_t* card_handle;
			if ((error = snd_card_next(&card_index)) < 0)
			{
				LOGW("Can't get next card number: %s\n", snd_strerror(error));
				break;
			}

			// No more cards left.
			if (card_index < 0)
			{
				break;
			}

			// Open card's control interface.
			char card_name[64] = {0};
			core::str::sprintf(card_name, 64, "hw:%i", card_index);
			if ((error = snd_ctl_open(&card_handle, card_name, 0)) < 0)
			{
				LOGW("Cannot open card %i: %s\n", snd_strerror(error));
				continue;
			}

			int32_t device_number = -1;
			int32_t total_devices = 0;

			for (;;)
			{
				// Get the index of the next wave device on this card.
				if ((error = snd_ctl_pcm_next_device(card_handle, &device_number)) < 0)
				{
					LOGW("Can't get next wave device number: %s\n", snd_strerror(error));
					break;
				}

				// No more wave devices.
				// MIDI devices won't have any wave devices.
				if (device_number < 0)
				{
					break;
				}

				// We must iterate over sub devices.
				snd_pcm_info_t* pcm_info;
				snd_pcm_info_alloca(&pcm_info);
				memset(pcm_info, 0, snd_pcm_info_sizeof());

				// Request info for device_number from ALSA.
				snd_pcm_info_set_device(pcm_info, device_number);

				// playback (output) or capture (input)
				snd_pcm_info_set_stream(pcm_info, SND_PCM_STREAM_PLAYBACK);

				int32_t sub_device_index = -1;
				int32_t total_subdevices = 1;

				// If there's only one sub device, the subdevice number can
				// be omitted when specifying the hardware name.
				while(++sub_device_index < total_subdevices)
				{
					snd_pcm_info_set_subdevice(pcm_info, sub_device_index);
					if ((error = snd_ctl_pcm_info(card_handle, pcm_info)) < 0)
					{
						LOGW("Can't get info for wave output subdevice hw:%i, %i, %i: %s\n",
							card_index,
							device_number,
							sub_device_index,
							snd_strerror(error)
						);
						continue;
					}

					if (sub_device_index == 0)
					{
						total_subdevices = snd_pcm_info_get_subdevices_count(pcm_info);
						// LOGV("Wave output device %i has %i sub devices.\n", device_number,  total_subdevices);
					}

					const size_t requested_bytes = sizeof(audio_device) + sizeof(alsa_audio_device);
					char* memory = static_cast<char*>(MEMORY_ALLOC(requested_bytes, get_platform_allocator()));
					audio_device* device = new (memory) audio_device();
					alsa_audio_device* audio_device = new (memory + sizeof(audio_device)) alsa_audio_device();

					core::str::sprintf(card_name, 64, "hw:%i,%i", device_number, sub_device_index);
					device->name = card_name;
					audio_device->device_number = device_number;
					audio_device->sub_device_number = sub_device_index;
					audio_state->devices.push_back(device);
				}

				++total_devices;
			}

			// LOGV("Found %i devices on card %i\n", total_devices, card_index);

			// Close the card's control interface.
			snd_ctl_close(card_handle);
		}

		snd_config_update_free_global();

		return platform::Result::success();
	} // audio_startup

	void audio_shutdown()
	{
		for (size_t index = 0; index < audio_state->devices.size(); ++index)
		{
			audio_device* device = audio_state->devices[index];
			MEMORY_DEALLOC(device, get_platform_allocator());
		}

		MEMORY_DELETE(audio_state, get_platform_allocator());
	} // audio_shutdown

	void audio_set_callback(audio_sound_callback callback, void* context)
	{
		assert(audio_state);

		audio_state->audio_pull_callback = callback;
		audio_state->audio_pull_context = context;
	} // audio_set_callback

	uint64_t audio_frame_position()
	{
		// NOTE: This requires tstamp mode be enabled in ALSA.
		// look at snd_pcm_status_get_tstamp()
		// snd_pcm_uframes_t available_frames;
		// snd_htimestamp_t timestamp;
		// snd_pcm_htimestamp(audio_state->handle,
		// 	&available_frames,
		// 	&timestamp);
		return audio_state->frames_written - audio_state->frame_delay;
	} // audio_frame_position
} // namespace platform

#endif // GEMINI_ENABLE_AUDIO
