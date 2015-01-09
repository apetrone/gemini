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
#include "interpolation.h"

#ifdef RGB
#undef RGB
#endif

#define RGBToUInt( r, g, b ) (((r&255)<<24) | ((g&255) <<16) | ((b&255)<<8) | 255)
#define UIntToRGB( i, c ) c[0] = ((i>>24)&255); c[1] = ((i>>16)&255); c[2] = ((i>>8)&255)
#define RGBAToUInt(r, g, b, a) ((r) | (g << 8) | (b << 16) | (a << 24))

// unsigned char rgba[3];
// unsigned int mycolor = RGBToUInt( 255, 128, 75 );
// UIntToRGB( mycolor, rgb );

namespace gemini
{
	struct Color
	{
		unsigned char r, g, b, a;
		
		Color( unsigned char _r = 255, unsigned char _g = 255, unsigned char _b = 255, unsigned char _a = 255 );
		static Color fromFloatPointer( const float * fl, int num_elements );
		void set( unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a = 255 );
	}; // Color


	template <>
	struct Interpolator<Color>
	{
		Color operator()( const Color & start, const Color & end, float t )
		{
			return Color( lerp( start.r, end.r, t ),
						 lerp( start.g, end.g, t ),
						 lerp( start.b, end.b, t ),
						 lerp( start.a, end.a, t ) );
		}
	}; // Interpolator
} // namespace gemini
