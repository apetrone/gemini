// -------------------------------------------------------------
// Copyright (C) 2015- Adam Petrone
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

#include "str.h"
#include "config.h"


// TODO: replace this with a non-C-runtime function.
#include <platform/platform.h>

#include <string.h>
#include <stdarg.h>

namespace core
{
	namespace str
	{
		static const int STRING_MAX_BUFFERS = 2;
		static const int STRING_BUFFER_SIZE = 8192;

		static unsigned int _current_format_buffer = 0;
		static PLATFORM_THREAD_LOCAL char _buffers[STRING_MAX_BUFFERS][STRING_BUFFER_SIZE];

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
#if defined(PLATFORM_LINUX) || defined(PLATFORM_APPLE) || defined(PLATFORM_ANDROID)
			return ::strncasecmp(s1, s2, count);
#elif defined(PLATFORM_WINDOWS)
			return ::strnicmp(s1, s2, count);
#else
	#error Unknown platform
#endif
		} // case_insensitive_compare

		const char* strstr(const char* s1, const char* s2)
		{
			return ::strstr(s1, s2);
		} // strstr

		intptr_t find_last_slash(const char* cstring)
		{
			// check backslash and forward slash
			const char* pos = strrchr(cstring, '\\'); // look for backward slash

			if (pos)
				return pos - cstring;

			pos = strrchr(cstring, '/'); // look for fwd slash

			return pos - cstring;
		} // find_last_slash

		const char* basename(const char* cstring)
		{
			// return the basename of a filepath. (just the filename)
			// this replicates the behavior in python
			intptr_t last_index = find_last_slash(cstring);
			if (last_index != -1)
			{
				return cstring + last_index + 1;
			}

			return cstring;
		} // basename

		char* directory_up(char* source)
		{
			assert(source);

			const char* last_slash = source;
			// seek to the end of the string
			while(*last_slash)
			{
				++last_slash;
			}

			// back up from the NULL terminator
			--last_slash;

			while(last_slash > source)
			{
				if (*last_slash == '/' || *last_slash == '\\')
				{
					// ignore trailing slashes
					if (last_slash[1] != '\0')
					{
						break;
					}
				}

				--last_slash;
			}

			source[last_slash - source] = '\0';
			return source;
		} // directory_up

		std::vector<std::string> split(const std::string& input, const std::string& substring)
		{
			std::vector<std::string> elements;

			std::string::size_type pos = 0, last = 0;

			using value_type = std::vector<std::string>::value_type;
			using size_type = std::vector<std::string>::size_type;

			bool end_of_string = false;
			while(true)
			{
				// locate the substring
				pos = input.find(substring, last);
				if (pos == std::string::npos)
				{
					pos = input.length();
					end_of_string = true;
				}

				if (pos != last)
				{
					elements.push_back(value_type(input.data()+last, (size_type)pos-last));
				}

				if (end_of_string)
				{
					break;
				}

				last = pos+1;
			}

			return elements;
		}

		std::string trim_left(const std::string& input, const std::string& chars)
		{
			std::string out;

			std::size_t start = input.find_first_not_of(chars);
			if (start != std::string::npos)
			{
				out = input.substr(start);
			}

			return out;
		}

	} // namespace str
} // namespace core
