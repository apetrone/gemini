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

namespace font
{
	typedef unsigned int Handle;

	void startup();
	void shutdown();
	
	// draw a string
	void draw_string( font::Handle fontid, int x, int y, const char * utf8, const Color & color );
	
	// query the height of the font in pixels
	unsigned int measure_height( font::Handle fontid );
	
	// measure the width of the string in a given font in pixels
	unsigned int measure_width( font::Handle fontid, const char * str );
	
	font::Handle load_font_from_memory( const void * data, unsigned int data_size, unsigned int point_size, bool antialiased, unsigned int hdpi, unsigned int vdpi );
}; // namespace font
