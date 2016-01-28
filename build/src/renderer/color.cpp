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
#include <core/typedefs.h>
#include <renderer/color.h>

namespace gemini
{
	static inline unsigned char float_to_ubyte(float f)
	{
		return (f * 255.0f);
	}

	Color Color::from_float_pointer(const float* fl, int num_elements)
	{
		Color c;
		if (num_elements == 4)
		{
			c.set(fl[0], fl[1], fl[2], fl[3]);
		}
		else
		{
			c.set(fl[0], fl[1], fl[2], 1.0f);
		}
		return c;
	}

	Color Color::from_int(unsigned int color)
	{
		Color out;
		out.alpha = ((color>>24) & 255) / 255.0f;
		out.blue = ((color>>16) & 255) / 255.0f;
		out.green = ((color>>8) & 255) / 255.0f;
		out.red = (color & 255) / 255.0f;
		return out;
	}

	Color Color::from_ubyte(unsigned char* ubyte)
	{
		return Color::from_rgba(ubyte[0], ubyte[1], ubyte[2], ubyte[3]);
	}

	Color Color::from_rgba(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
	{
		Color color;
		color.red = r / 255.0f;
		color.green = g / 255.0f;
		color.blue = b / 255.0f;
		color.alpha = a / 255.0f;

		return color;
	}

	Color::Color(float r, float g, float b, float a)
	{
		set(r, g, b, a);
	}

	void Color::set(float r, float g, float b, float a)
	{
		red = r;
		green = g;
		blue = b;
		alpha = a;

		// As I make the conversion to normalized floats for colors,
		// this will get hit whenever an unsigned byte is passed in
		// by mistake.
		assert(red >= 0.0f && red <= 1.0f);
		assert(green >= 0.0f && green <= 1.0f);
		assert(blue >= 0.0f && blue <= 1.0f);
		assert(alpha >= 0.0f && alpha <= 1.0f);
	}

	uint32_t Color::as_uint32() const
	{
		return static_cast<uint32_t>(((
			float_to_ubyte(alpha) << 24) |
			(float_to_ubyte(blue) << 16) |
			(float_to_ubyte(green) << 8) |
			float_to_ubyte(red)));
	}

	bool Color::operator==(const Color& other) const
	{
		return (red == other.red) && (green == other.green) && (blue == other.blue) && (alpha == other.alpha);
	}
} // namespace gemini
