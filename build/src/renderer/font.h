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

#include "shaderprogram.h"
#include "color.h"

#include <core/typedefs.h>


namespace font
{
	typedef int Handle;
}

namespace renderer
{
	struct Font
	{
		font::Handle handle;
		uint16_t point_size;
		
		Font() : handle(-1), point_size(0) {}
		virtual ~Font() {}
		
		inline bool is_valid() const { return (handle >= 0); }
	}; // Font
} // namespace renderer

namespace render2
{
	class Device;
	struct Texture;
}

namespace font
{
	// provide the shader to use for rendering as well as the render width and height
	// of the target buffer or viewport
	void startup(renderer::ShaderProgram* fontshader, int width, int height);

	// must shut this down BEFORE the renderer; otherwise it cannot
	// allocate resources and will crash because it stores a pointer to the device.
	void shutdown();
	
	// draw string at (x, y) screen coordinates with the origin in the upper left of the screen
	void draw_string(Handle handle, int x, int y, const char* utf8, const core::Color& color);
	
	// set the viewport size for the future draw calls
	void set_viewport_size(int render_width, int render_height);

	// query the height of the font in pixels
	unsigned int measure_height(Handle handle, const char* utf8);
	
	// measure the width of the string in a given font in pixels
	unsigned int measure_width(Handle handle, const char* utf8);
	
	// load font from memory with the desired point size
	Handle load_font_from_memory(const void* data, unsigned int data_size, unsigned short point_size);
} // namespace font


template <class T>
class Array;

namespace render2
{
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
			core::Color color;
			glm::vec2 uv;
		};

		struct Handle
		{
			int ref;

			Handle() :
				ref(-1)
			{
			}

			bool is_valid() const;

			unsigned int point_size() const
			{
				return 0; //return get_point_size(*this);
			}
		};


		// setup resources the font library might need
		void startup(render2::Device* device);

		// cleanup any used resources
		void shutdown();

		// load font from memory with the desired point size
		Handle load_from_memory(const void* data, unsigned int data_size, unsigned int point_size, Type target_type = FONT_TYPE_BITMAP);

		// return the point size for the font
		unsigned int get_point_size(Handle handle);

		// measure the width of the string in a given font in pixels
		unsigned int measure_width(Handle handle, const char* utf8);

		// query the height of the font in pixels
		unsigned int measure_height(Handle handle, const char* utf8);

		// populate vertices with the transformed vertices for drawing a string to the screen
		void draw_string(Handle handle, Array<FontVertex>& vertices, const glm::mat2& transform, const char* utf8, const core::Color& color);

		render2::Texture* get_font_texture(Handle handle);
	} // namespace font
} // namespace render2





