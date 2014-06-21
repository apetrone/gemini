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

#include "typedefs.h"
#include "color.hpp"
#include "assets/asset_font.hpp"

namespace font
{
	void startup();
	void shutdown();
	
	// draw a string
	// this accepts x and y coordinates with the origin in the upper left of the screen
	void draw_string( assets::Font * font, int x, int y, const char * utf8, const Color & color );
	
	// query the height of the font in pixels
	unsigned int measure_height( assets::Font * font, const char * utf8 );
	
	// measure the width of the string in a given font in pixels
	unsigned int measure_width( assets::Font * font, const char * utf8 );
	
	assets::FontHandle load_font_from_memory( const void * data, unsigned int data_size, unsigned short point_size );
	
	/// @param path relative path to the font file: "fonts/nokifc22.ttf"
	/// @param point_size font rendered with this size
	/// @param handle Output font handle
	/// @returns Font data as char *
	char * load_font_from_file( const char * path, unsigned short point_size, assets::FontHandle & handle );
}; // namespace font
