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
#pragma once

#if defined(GEMINI_ENABLE_AUDIO)

#include "config.h"
#include "platform.h"

#include <core/mem.h>

namespace platform
{
	enum class AudioDeviceType
	{
		Output,	// speakers, headphones, etc.
		Input	// microphones, capture, etc.
	}; // AudioDeviceType

	struct audio_device
	{
		core::StackString<128> name;
		AudioDeviceType type;
	}; // audio_device

	typedef void (*audio_sound_callback)(void* buffer_frames, size_t buffer_frame_count, size_t sample_rate_hz, void* context);

	// This should list the default device first
	platform::Result audio_enumerate_devices(Array<audio_device*>& devices);

	// open audio output device
	platform::Result audio_open_output_device(audio_device* device);

	// close active output device
	void audio_close_output_device();

	// lifetime control for the audio engine
	platform::Result audio_startup();
	void audio_shutdown();

	// set a sound callback for an audio device
	// This will be called by the audio engine when the primary buffer should be filled.
	void audio_set_callback(audio_sound_callback callback, void* context);

	// Get the current frame count from the device.
	/// @returns The number of frames played since the device started playing.
	uint64_t audio_frame_position();
} // namespace platform

#endif // GEMINI_ENABLE_AUDIO