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


namespace platform
{
	struct alsa_audio_device
	{

	}; // alsa_audio_state

	struct alsa_audio_state
	{
		uint32_t sample_rate_hz;
		snd_pcm_t* handle;
		audio_sound_callback audio_pull_calback;
		Array<audio_device*, platform::PlatformAllocatorType> devices;

		alsa_audio_state()
			: devices(16, get_platform_allocator())
		{
		}
	}; // alsa_audio_state

	alsa_audio_state* audio_state;

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

	static void create_device_list(alsa_audio_state* state)
	{

	}


	platform::Result audio_enumerate_devices(Array<audio_device*>& devices)
	{
		for (size_t index = 0; index < audio_state->devices.size(); ++index)
		{
			devices.push_back(audio_state->devices[index]);
		}

		return platform::Result::success();
	} // audio_enumerate_devices

	platform::Result audio_open_output_device(audio_device*)
	{
		return platform::Result::failure("Not implemented");
	} // audio_open_output_device

	void audio_close_output_device()
	{
	} // audio_close_output_device

	platform::Result audio_startup()
	{
		audio_state = MEMORY_NEW(alsa_audio_state, get_platform_allocator());
		create_device_list(audio_state);

		return platform::Result::success();
	} // audio_startup

	void audio_shutdown()
	{
		MEMORY_DELETE(audio_state, get_platform_allocator());

	} // audio_shutdown

	void audio_set_callback(audio_sound_callback, void*)
	{
		// audio_state.audio_pull_callback = callback;
		// audio_state.context = context;
	} // audio_set_callback

	uint64_t audio_frame_position()
	{
		return 0;
	} // audio_frame_position
} // namespace platform

#endif // GEMINI_ENABLE_AUDIO
