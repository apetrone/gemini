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
#pragma once

#include <runtime/assets.h>
#include <runtime/audio_mixer.h>

#include <core/stackstring.h>

namespace gemini
{
	namespace assets
	{
		// -------------------------------------------------------------
		// Sound

		// A frame is a single sample of PCM data
		// (including all channels which comprise that sample).

		struct Sound : public Asset
		{
			Array<gemini::audio::InMemorySampleType> pcmdata;
			size_t channels;

			// retrieve a frame from the underlying audio asset.
			// returns the number of frames read (0 on error)
			size_t (*get_frame_callback)(struct Sound* sound, size_t frame, float* buffer);

			Sound(gemini::Allocator& allocator);
			virtual ~Sound();
			virtual void release();
			size_t get_frame(size_t frame, float* destination);

			// Returns total frames for this sound.
			size_t get_total_frames() const;
		}; // Sound

		AssetLoadStatus sound_load_callback(const char* path, AssetLoadState<Sound>& load_state, const AssetParameters& parameters);
		void sound_construct_extension(core::StackString<MAX_PATH_SIZE>& extension);

		DECLARE_ASSET_LIBRARY_ACCESSOR(Sound, AssetParameters, sounds);
	} // namespace assets
} // namespace gemini
