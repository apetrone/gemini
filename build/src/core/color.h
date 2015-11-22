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
#include "typedefs.h"

namespace core
{
	struct Color
	{
		static Color from_float_pointer(const float * fl, int num_elements);
		static Color from_int(unsigned int color);
		static Color from_ubyte(unsigned char* ubyte);
		static Color from_rgba(unsigned char r, unsigned char g, unsigned char b, unsigned char a);

		LIBRARY_EXPORT Color(float r = 1.0f, float = 1.0f, float b = 1.0f, float a = 1.0f);
		LIBRARY_EXPORT void set(float r, float g, float b, float a = 1.0f);
		LIBRARY_EXPORT uint32_t as_uint32() const;
		LIBRARY_EXPORT bool operator==(const Color& other) const;

		float red;
		float green;
		float blue;
		float alpha;
	}; // Color


	template <>
	struct Interpolator<Color>
	{
		Color operator()( const Color & start, const Color & end, float t )
		{
			return Color( lerp( start.red, end.red, t ),
						 lerp( start.green, end.green, t ),
						 lerp( start.blue, end.blue, t ),
						 lerp( start.alpha, end.alpha, t ) );
		}
	}; // Interpolator
} // namespace core
