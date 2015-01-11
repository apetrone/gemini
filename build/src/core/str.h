// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone

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

#include <platform/mem.h>
#include <string>

typedef std::basic_string<char, std::char_traits<char>, CustomPlatformAllocator<char> > String;


namespace gemini
{
	namespace core
	{
		namespace str
		{
			// ANSI string utils
			
			// returns a mutable buffer of characters; works in the style
			// of printf.
			char* format(const char* format, ...);

			// cross platform functions
			int sprintf(char* destination, size_t destination_max_size, const char* format, ...);
			int vsnprintf(char* destination, size_t destination_max_size, const char* format, va_list arg);
			
			// pass 0 for source_bytes in order to count the source string; (runs len on it)
			char* copy(char* destination, const char* source, size_t source_bytes);
			size_t len(const char* str);
			char* cat(char* destination, const char* source);
			char* ncat(char* destination, const char* source, size_t destination_size);
			int case_insensitive_compare(const char* s1, const char* s2, size_t count);
			const char* strstr(const char* s1, const char* s2);
		} // namespace str
	} // namespace core
} // namespace gemini
