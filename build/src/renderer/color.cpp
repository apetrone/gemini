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

#include <platform/typedefs.h>
#include <slim/xlog.h>

#include "color.h"

// unsigned char rgba[3];
// unsigned int mycolor = RGBToUInt( 255, 128, 75 );
// UIntToRGB( mycolor, rgb );

static inline float ubTof32( unsigned char c )
{
	return float(( c / 255.0f ));
}

static inline unsigned int f32Toub( float f )
{
	return (f * 255.0f);
}


Color::Color( unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a )
{
	set( _r, _g, _b, _a );
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

void Color::set( unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a )
{
	r = _r;
	g = _g;
	b = _b;
	a = _a;
}