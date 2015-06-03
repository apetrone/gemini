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

#include "interpolation.h"

#include <platform/typedefs.h>

namespace core
{
	struct LIBRARY_EXPORT Color
	{
		static Color from_float_pointer( const float * fl, int num_elements );
		static Color from_int(unsigned int color);
		static Color from_ubyte(unsigned char* ubyte);
		
		unsigned char r, g, b, a;
		Color( unsigned char _r = 255, unsigned char _g = 255, unsigned char _b = 255, unsigned char _a = 255 );
		void set( unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a = 255 );
		uint32_t as_uint32() const;
		bool operator==(const Color& other) const;
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
