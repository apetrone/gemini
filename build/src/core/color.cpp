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

#include "color.h"

#include <platform/typedefs.h>

// unsigned char rgba[3];
// unsigned int mycolor = RGBToUInt( 255, 128, 75 );
// UIntToRGB( mycolor, rgb );

namespace core
{

	static inline float ubTof32( unsigned char c )
	{
		return float(( c / 255.0f ));
	}

	static inline unsigned int f32Toub( float f )
	{
		return (f * 255.0f);
	}
	
	Color Color::fromFloatPointer( const float * fl, int num_elements )
	{
		Color c;
		if ( num_elements == 4 )
		{
			c.set( f32Toub(fl[0]), f32Toub(fl[1]), f32Toub(fl[2]), f32Toub(fl[3]) );
		}
		else
		{
			c.set( f32Toub(fl[0]), f32Toub(fl[1]), f32Toub(fl[2]), 255 );
		}
		return c;
	}
	
	Color Color::from_int(unsigned int color)
	{
		Color out;
		out.a = ((color>>24) & 255);
		out.b = ((color>>16) & 255);
		out.g = ((color>>8) & 255);
		out.r = (color & 255);
		return out;
	}
	
	Color Color::from_ubyte(unsigned char* ubyte)
	{
		return Color(ubyte[0], ubyte[1], ubyte[2], ubyte[3]);
	}

	Color::Color( unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a )
	{
		set( _r, _g, _b, _a );
	}

	void Color::set( unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a )
	{
		r = _r;
		g = _g;
		b = _b;
		a = _a;
	}
	
	uint32_t Color::as_uint32() const
	{
		return (((a << 24) | (b << 16) | (g << 8) | r));
	}
} // namespace core