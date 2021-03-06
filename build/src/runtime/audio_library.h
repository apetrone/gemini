// -------------------------------------------------------------
// Copyright (C) 2017- Adam Petrone
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
#pragma once

#include <runtime/asset_library.h>

namespace gemini
{
	// -------------------------------------------------------------
	// Sound

	// A frame is a single sample of PCM data
	// (including all channels which comprise that sample).

	struct Sound
	{
		Array<float> pcmdata;
		size_t channels;

		// retrieve a frame from the underlying audio asset.
		// returns the number of frames read (0 on error)
		size_t(*get_frame_callback)(struct Sound* sound, size_t frame, float* buffer);

		Sound(Allocator& allocator);
		virtual ~Sound();
		size_t get_frame(size_t frame, float* destination);

		// Returns total frames for this sound.
		size_t get_total_frames() const;
	}; // Sound

	class AudioLibrary : public AssetLibrary2<Sound, AudioLibrary>
	{
	public:
		AudioLibrary(Allocator& allocator);

		void create_asset(LoadState& state, void* parameters);
		inline bool is_same_asset(AssetClass*, void*) { return true; }
		AssetLoadStatus load_asset(LoadState& state, platform::PathString& fullpath, void* parameters);
		void destroy_asset(LoadState& state);
	}; // AudioLibrary
} // namespace gemini
