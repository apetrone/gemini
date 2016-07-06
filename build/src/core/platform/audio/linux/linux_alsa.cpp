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
#include <platform/platform.h>

#include <core/atomic.h>
#include <core/logging.h>

#if defined(GEMINI_ENABLE_AUDIO)

namespace platform
{
	platform::Result audio_enumerate_devices(Array<audio_device*>& devices)
	{
		return platform::Result::failure("Not implemented");
	} // audio_enumerate_devices

	platform::Result audio_open_output_device(audio_device* device)
	{
		return platform::Result::failure("Not implemented");
	} // audio_open_output_device

	void audio_close_output_device()
	{
	} // audio_close_output_device

	platform::Result audio_startup()
	{
		return platform::Result::failure("Not implemented");
	} // audio_startup

	void audio_shutdown()
	{
	} // audio_shutdown

	void audio_set_callback(audio_sound_callback callback, void* context)
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
