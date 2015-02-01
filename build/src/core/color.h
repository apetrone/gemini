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

namespace core
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
} // namespace core
