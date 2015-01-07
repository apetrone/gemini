// -------------------------------------------------------------
// Copyright (C) 2004- Adam Petrone

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM,OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
// -------------------------------------------------------------
#pragma once

#include <core/typedefs.h>

#include "shaderprogram.h"
#include "color.h"

namespace renderer
{
	typedef int FontHandle;
	struct Font
	{
		FontHandle handle;
		uint16_t point_size;
		
		inline bool is_valid() const { return (handle >= 0); }
	};
}

namespace font
{
	// provide the shader to use for rendering as well as the render width and height
	// of the target buffer or viewport
	void startup(renderer::ShaderProgram* fontshader, int width, int height);
	void shutdown();
	
	// draw string at (x, y) screen coordinates with the origin in the upper left of the screen
	void draw_string(const renderer::Font& font, int x, int y, const char* utf8, const Color& color );
	
	// set the viewport size for the future draw calls
	void set_viewport_size(int render_width, int render_height);
	
	// query the height of the font in pixels
	unsigned int measure_height(const renderer::Font& font, const char* utf8 );
	
	// measure the width of the string in a given font in pixels
	unsigned int measure_width(const renderer::Font& font, const char* utf8 );
	
	// load font from memory with the desired point size
	renderer::Font load_font_from_memory(const void* data, unsigned int data_size, unsigned short point_size);
}; // namespace font
