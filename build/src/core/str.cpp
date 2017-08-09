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

#if 0
	str_t::str_t(gemini::Allocator& memory_allocator,
		const char* str)
		: allocator(memory_allocator)
	{
		// data_size is dirty.
		flags = 2;

		// data doesn't belong to us, but we can read it.
		data = const_cast<char*>(str);

		recalculate_size();
	}

	str_t::str_t(gemini::Allocator& memory_allocator,
		const char* str,
		size_t start,
		size_t length)
		: allocator(memory_allocator)
	{
		// This MUST make an explicit copy because the user asks for a subset of the string.
		// There's no easy way to do this right now. It's possible that we can keep a separate pointer
		// to a sub-index within the string -- but this seems like tbe best idea for now.
		// The biggest reason this must make a copy, is that we need to terminate it correctly
		// in order to achieve the correct subset of a larger string. If we don't, then a c_str()
		// operation would result in the original string (since we cannot mutate it without making a copy).
		flags = 1;
		reallocate(length);
		core::str::copy(data, &str[start], data_size);
		data[length] = '\0';
	}

	void str_t::reallocate(size_t new_size)
	{
		data_size = new_size;
		data = static_cast<char*>(MEMORY2_ALLOC(allocator, (data_size + 1) * sizeof(uint8_t)));
	}

	str_t::~str_t()
	{
		if (flags & 1)
		{
			MEMORY2_DEALLOC(allocator, data);
			data = nullptr;
			data_size = 0;
			flags &= ~1;
		}
	}

	void str_t::recalculate_size()
	{
		data_size = core::str::len(data);
		flags &= ~2;
	}

	bool str_t::operator==(const char* other)
	{
		return core::str::case_insensitive_compare(data, other, 0) == 0;
	}

	str_t& str_t::operator=(const str_t& other)
	{
		return *this;
	}

	size_t str_t::size() const
	{
		return data_size * sizeof(char);
	}

	size_t str_t::length() const
	{
		return data_size;
	}

	const char* str_t::c_str() const
	{
		return data;
	} // c_str

	char& str_t::operator[](int index)
	{
		if ((flags & 1) == 0)
		{
			// If you hit this conditional,
			// then this instance doesn't own the data.
			// User wants to modify the data -- so we need to
			// create a unique copy for this string so it is mutable.
			perform_copy_on_write();
		}
		assert(index <= data_size);
		return data[index];
	} // char& operator[]

	void str_t::perform_copy_on_write()
	{
		const char* original_string = data;
		reallocate(core::str::len(original_string));

		// this instance now owns data.
		flags = 1;

		core::str::copy(data, original_string, data_size);
	} // perform_copy_on_write

	str_t str_t::copy(gemini::Allocator& allocator, const char* source)
	{
		return str_t(allocator, source);
	} // static copy

	char str_t::operator[](int index) const
	{
		// If you hit this, indexing into data would cause a buffer overrun.
		assert(index <= data_size);
		return data[index];
	} // char operator[]

#endif

} // namespace core

namespace gemini
{


	char* string_allocate(gemini::Allocator& allocator, size_t length)
	{
		char* string_data = reinterpret_cast<char*>(MEMORY2_ALLOC(allocator, sizeof(char) * (length + 1)));
		memset(string_data, 0, sizeof(char) * length);
		string_data[length] = '\0';
		return string_data;
	}

	string string_create(gemini::Allocator& allocator, const char* data)
	{
		string value;
		value.string_data_size = core::str::len(data);

		char* string_data = string_allocate(allocator, value.string_data_size);
		memcpy(string_data, data, value.string_data_size);
		value.string_data = string_data;

		return value;
	} // string_create

	void string_destroy(gemini::Allocator& allocator, string& string)
	{
		MEMORY2_DEALLOC(allocator, const_cast<char*>(string.string_data));
	} // string_destroy

	string string_concat(gemini::Allocator& allocator, const string& first, const string& second)
	{
		string extended;
		extended.string_data_size = (first.string_data_size + second.string_data_size);

		char* string_data = string_allocate(allocator, extended.string_data_size);
		core::str::cat(string_data, first.string_data);
		core::str::cat(string_data, second.string_data);
		extended.string_data = string_data;

		return extended;
	} // string_concat

	string string_substr(gemini::Allocator& allocator, const char* source, uint32_t start, uint32_t length)
	{
		string substring;
		char* string_data = string_allocate(allocator, length);
		core::str::copy(string_data, &source[start], length);
		string_data[length] = '\0';
		substring.string_data_size = length;
		substring.string_data = string_data;
		return substring;
	} // string_substr
} // namespace gemini