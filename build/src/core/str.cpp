// -------------------------------------------------------------
// Copyright (C) 2015- Adam Petrone

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

#include <platform/config.h>
#include "str.h"

#include <string.h>
#include <stdarg.h>

namespace gemini
{
	namespace core
	{
		namespace str
		{
			static const int STRING_MAX_BUFFERS = 2;
			static const int STRING_BUFFER_SIZE = 8192;
			
			static unsigned int _current_format_buffer = 0;
			static char _buffers[STRING_MAX_BUFFERS][STRING_BUFFER_SIZE];
			
			char* format(const char* format, ...)
			{
				va_list arg;
				
				char* tmp = &_buffers[_current_format_buffer][0];
				
				va_start(arg, format);
				vsnprintf(tmp, STRING_BUFFER_SIZE, format, arg);
				va_end(arg);
				
				++_current_format_buffer;
				_current_format_buffer = (_current_format_buffer % STRING_MAX_BUFFERS);
				
				return tmp;
			} // format

			int sprintf(char* destination, size_t destination_max_size, const char* format, ...)
			{
				va_list arg;
				int result;
				
				va_start(arg, format);
				result = vsnprintf(destination, destination_max_size, format, arg);
				va_end(arg);
				
				return result;
			} // sprintf
			
			int vsnprintf(char* destination, size_t destination_max_size, const char* format, va_list arg)
			{
				return ::vsnprintf(destination, destination_max_size, format, arg);
			} // vsnprintf
			
			char* copy(char* destination, const char* source, size_t source_bytes)
			{
				if (source_bytes == 0)
				{
					source_bytes = len(source);
				}
				
				return strncpy(destination, source, source_bytes);
			} // copy
			
			size_t len(const char* str)
			{
				return strlen(str);
			} // len
			
			char* cat(char* destination, const char* source)
			{
				return strcat(destination, source);
			} // cat
			
			char* ncat(char* destination, const char* source, size_t destination_size)
			{
				size_t dst_len = len(destination);
				size_t src_len = len(source);
				
				// we cannot possibly fit this string in the destination array
				if (dst_len + src_len >= destination_size)
				{
					return 0;
				}
				
				return cat(destination, source);
			} // ncat
			
			int case_insensitive_compare(const char* s1, const char* s2, size_t count)
			{
				size_t s1_len;
				size_t s2_len;
				
				if (!s1 || !s2)
				{
					if (s1)
					{
						return 1;
					}
					else if (s2)
					{
						return -1;
					}
					else
					{
						return 0;
					}
				}
				
				// size_t is implemented as 'unsigned int' on some platforms (Windows...)
				// use zero to mean: pick the smallest string
				if (count == 0)
				{
					s1_len = 0;
					if (s1 != 0)
					{
						s1_len = len(s1);
					}
					
					s2_len = 0;
					if (s2 != 0)
					{
						s2_len = len(s2);
					}
					
					if (s1_len < s2_len)
						return -1;
					else if (s1_len > s2_len)
						return 1;
					
					count = s1_len;
				}
#if PLATFORM_LINUX || PLATFORM_APPLE || PLATFORM_ANDROID
				return ::strncasecmp(s1, s2, count);
#else
				return ::strnicmp(s1, s2, count);
#endif
			} // case_insensitive_compare
			
			const char* strstr(const char* s1, const char* s2)
			{
				return ::strstr(s1, s2);
			} // strstr
		} // namespace str
	} // namespace core
} // namespace gemini
