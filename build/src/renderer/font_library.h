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

#include <renderer/font_library.h>
#include <runtime/asset_library.h>

namespace render2
{
	class Device;
	struct Texture;
}

namespace gemini
{
	// the font system can handle two types of fonts:
	// - classic bitmap fonts
	// - signed distance field fonts
	enum FontType
	{
		FONT_TYPE_INVALID,
		FONT_TYPE_BITMAP,	// standard bitmap font
		FONT_TYPE_SDF		// signed distance field font
	};

	struct FontVertex
	{
		glm::vec2 position;
		gemini::Color color;
		glm::vec2 uv;
	};

	struct FontMetrics
	{
		// scaled font metrics in pixels
		int32_t height;
		int32_t ascender;
		int32_t descender;
		int32_t max_height;
	};

	struct FontCreateParameters
	{
		// font size in pixels
		uint32_t size_pixels;
	};

	struct FontLibraryData;
	struct FontData;









	class FontLibrary : public AssetLibrary2<FontData, FontLibrary>
	{
	public:

		FontLibrary(Allocator& allocator, render2::Device* render_device);
		virtual ~FontLibrary();

		void create_asset(LoadState& state, void* parameters);
		bool is_same_asset(AssetClass* asset, void* parameters);
		AssetLoadStatus load_asset(LoadState& state, platform::PathString& fullpath, void* parameters);
		void destroy_asset(LoadState& state);


	public:

		// additional font functionality

		// populate vertices with the transformed vertices for drawing a string to the screen
		// returns the number of vertices used
		size_t draw_string(AssetHandle handle, FontVertex* vertices, const char* utf8, size_t string_length, const Color& color);

		// retrieve metrics for this font
		void font_metrics(AssetHandle handle, FontMetrics& out_metrics);

		// retrieve the font texture used by a font
		render2::Texture* font_texture(AssetHandle handle);

		// fetch metrics for a string
		int32_t string_metrics(AssetHandle handle, const char* utf8, size_t string_length, glm::vec2& mins, glm::vec2& maxs);
	private:

		int32_t uvs_for_codepoint(FontData* font, int32_t codepoint, glm::vec2* uvs);

		FontLibraryData* private_state;
	}; // FontLibrary
} // namespace gemini
