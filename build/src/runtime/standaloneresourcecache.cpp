// -------------------------------------------------------------
// Copyright (C) 2015- Adam Petrone
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
#include "standaloneresourcecache.h"

#include <renderer/font_library.h>
#include <renderer/image.h>
#include <renderer/texture.h>

#include <runtime/assets.h>
#include <runtime/filesystem.h>

using namespace gemini;

namespace renderer
{
	StandaloneResourceCache::StandaloneResourceCache(gemini::Allocator& _allocator)
		: allocator(_allocator)
		, textures(allocator, 0)
		, handle_by_texture(allocator)
		, font_handle_by_path(allocator, HASHSET_INITIAL_SIZE, HASHSET_GROWTH_FACTOR, Array<gui::FontHandle>(allocator))
	{
	}

	StandaloneResourceCache::~StandaloneResourceCache()
	{
		clear();
	}

	void StandaloneResourceCache::clear()
	{
		for (FontByPathSet::Iterator it = font_handle_by_path.begin(); it != font_handle_by_path.end(); ++it)
		{
			Array<gui::FontHandle>& items = it.value();
			items.clear();
		}

		textures.clear();

		handle_by_texture.clear();
		font_handle_by_path.clear();
	}

	gui::FontHandle StandaloneResourceCache::create_font(const char* filename, size_t pixel_size)
	{
		FontCreateParameters font_params;
		font_params.size_pixels = pixel_size;
		AssetHandle fonthandle = font_load(filename, false, &font_params);

		gui::FontHandle handle(fonthandle.index);

		// we need to track the texture for looking during rendering
		render2::Texture* texture = font_texture(fonthandle);
		if (!handle_by_texture.has_key(texture))
		{
			track_texture(texture);

			// insert the new handle into the array
			Array<gui::FontHandle>& items = font_handle_by_path[filename];
			items.push_back(handle);
		}

		return handle;
	}

	void StandaloneResourceCache::destroy_font(const gui::FontHandle& /*handle*/)
	{
		// ignore these for now; we take care of font tracking
	//	font::Handle fonthandle(handle);
	//	font::destroy_font(fonthandle);
	}

	gui::TextureHandle StandaloneResourceCache::texture_for_font(const gui::FontHandle& handle)
	{
		AssetHandle font_handle;
		font_handle.index = handle;

		render2::Texture* texture = font_texture(font_handle);
		if (handle_by_texture.has_key(texture))
		{
			return handle_by_texture[texture];
		}

		return gui::TextureHandle();
	}

	gui::TextureHandle StandaloneResourceCache::create_texture(const char* /*filename*/)
	{
		// generate a texture
//		image::Image checker_pattern;
//		checker_pattern.width = 32;
//		checker_pattern.height = 32;
//		checker_pattern.channels = 3;
//		image::generate_checker_pattern(checker_pattern, gemini::Color(1.0f, 0, 1.0f), gemini::Color(0, 1.0f, 0));
//		checker_texture = device->create_texture(checker_pattern);
//		assert(checker_texture);

		gui::TextureHandle handle;
		assert(0);
		return handle;
	}

	int StandaloneResourceCache::track_texture(render2::Texture *texture)
	{
		assert(!handle_by_texture.has_key(texture));


		int ref = static_cast<int>(textures.size());
		textures.push_back(texture);
		handle_by_texture[texture] = ref;

		return ref;
	}

	void StandaloneResourceCache::destroy_texture(const gui::TextureHandle& /*handle*/)
	{
		assert(0);
	}

	gui::TextureHandle StandaloneResourceCache::texture_to_handle(render2::Texture* texture)
	{
		assert(handle_by_texture.has_key(texture));
		gui::TextureHandle handle = handle_by_texture[texture];
		return handle;
	}

	render2::Texture* StandaloneResourceCache::handle_to_texture(const gui::TextureHandle& handle)
	{
		assert(handle.is_valid());
		int index = handle;
		return textures[static_cast<size_t>(index)];
	}

} // namespace renderer
