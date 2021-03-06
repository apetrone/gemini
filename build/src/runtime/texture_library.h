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
#include <renderer/image.h>

namespace render2
{
	class Device;
	struct Texture;
} // namespace render2

namespace gemini
{
	struct TextureCreateParameters
	{
		render2::Device* device;
		image::FilterType filter;
		//uint32_t flags;
	};

	class TextureLibrary : public AssetLibrary2<render2::Texture, TextureLibrary>
	{
	public:
		TextureLibrary(Allocator& allocator, render2::Device* render_device);

		void create_asset(LoadState& state, void* parameters);
		inline bool is_same_asset(AssetClass*, void*) { return true; }
		AssetLoadStatus load_asset(LoadState& state, platform::PathString& fullpath, void* parameters);
		void destroy_asset(LoadState& state);

	protected:
		render2::Device* device;
	}; // TextureLibrary
} // namespace gemini
