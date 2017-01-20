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

#include <runtime/filesystem.h>
#include <runtime/texture_library.h>

#include <core/array.h>
#include <core/logging.h>
#include <core/mem.h>

#include <renderer/image.h>

namespace gemini
{
	TextureLibrary::TextureLibrary(Allocator& allocator, render2::Device* render_device)
		: AssetLibrary2(allocator)
		, device(render_device)
	{
	}

	AssetLoadStatus TextureLibrary::create(LoadState& state, platform::PathString& fullpath)
	{
		LOGV("loading texture \"%s\"\n", fullpath());

		platform::PathString asset_uri = fullpath;

		// TODO: upgrade this when we want to support other formats; for now,
		// png is default.
		asset_uri.append(".png");

		bool is_new_asset = state.asset == nullptr;
		if (is_new_asset)
		{
			state.asset = MEMORY2_NEW(*state.allocator, gemini::Texture)(*state.allocator);
		}

		Array<unsigned char> buffer(*state.allocator);
		::renderer::Texture* render_texture = nullptr;
		core::filesystem::instance()->virtual_load_file(buffer, asset_uri());
		if (!buffer.empty())
		{
			image::Image& image = state.asset->image;
			unsigned char* pixels = image::load_image_from_memory(&buffer[0], buffer.size(), &image.width, &image.height, &image.channels);
			if (pixels)
			{
				//				may need to actually flip the image vertically here
				//				flip_image_vertically( width, height, components, pixels );

				image.pixels = pixels;
				//image.filter = parameters.filter_type;
				image.filter = image::FILTER_LINEAR;

				assert(0); // TODO: 01-19-17: fix this (textures)
				//render_texture = ::renderer::driver()->texture_create(image);

				LOGV("Loaded texture \"%s\"; (%i x %i @ %ibpp)\n", asset_uri(), image.width, image.height, image.channels);

				image::free_image(pixels);
				image.pixels = 0;
				return AssetLoad_Success;
			}
			else
			{
				LOGE("Unable to load image %s\n", asset_uri());
			}
		}
		else
		{
			LOGE("Couldn't load file: %s\n", asset_uri());
		}

		if (is_new_asset)
		{
			MEMORY2_DELETE(*state.allocator, state.asset);
		}

		return AssetLoad_Failure;
	}

	void TextureLibrary::destroy(LoadState& state)
	{
		MEMORY2_DELETE(*state.allocator, state.asset);
	}
} // namespace gemini
