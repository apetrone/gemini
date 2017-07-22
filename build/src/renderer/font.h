// -------------------------------------------------------------
// Copyright (C) 2004- Adam Petrone
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

//#include "shaderprogram.h"
#include <renderer/color.h>

#include <core/typedefs.h>
#include <core/array.h>

namespace gemini
{
	struct Allocator;
}

namespace render2
{
	class Device;
	struct Texture;
}

namespace font
{
	// the font system can handle two types of fonts:
	// - classic bitmap fonts
	// - signed distance field fonts
	enum Type
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

	struct Metrics
	{
		// scaled font metrics in pixels
		int32_t height;
		int32_t ascender;
		int32_t descender;
		int32_t max_height;
	};

	struct Handle
	{
		int ref;

		Handle(int reference = -1) :
			ref(reference)
		{
		}

		bool is_valid() const;

		operator int() const
		{
			return ref;
		}
	};


	// setup resources the font library might need
	void startup(gemini::Allocator& allocator, render2::Device* device);

	// cleanup any used resources
	void shutdown();

	// load font from memory with the desired pixel size
	Handle load_from_memory(const void* data, size_t data_size, size_t pixel_size, Type target_type = FONT_TYPE_BITMAP);

	// called to purge memory used by handle's font
	void destroy_font(Handle& handle);

	// return the point size for the font
	unsigned int get_pixel_size(Handle handle);

	// retrieve metrics for this font
	void get_font_metrics(Handle handle, Metrics& out_metrics);

	// fetch metrics for a string
	int get_string_metrics(Handle handle, const char* utf8, size_t string_length, glm::vec2& mins, glm::vec2& maxs);

	// populate vertices with the transformed vertices for drawing a string to the screen
	// returns the number of vertices used
	size_t draw_string(Handle handle, FontVertex* vertices, const char* utf8, size_t string_length, const gemini::Color& color);

	// retrieve the font texture used by a font
	render2::Texture* get_font_texture(Handle handle);

	/// @returns The number of vertices required to render string with
	/// string_length in characters.
	size_t count_vertices(Handle handle, size_t string_length);
} // namespace font