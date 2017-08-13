// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone
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

#include "typedefs.h"
#include "mem.h"
#include <core/array.h>

#include <string>
#include <vector>


typedef std::string String;

#define STRING_HASH32(string_value)\
	::core::str::hash_string32< ::core::str::const_length(string_value) >(string_value)

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

		// returns if newline is at string; returns true for \r\n or \n
		bool isnewline(char* string, uint32_t* advance = nullptr);

		// pass 0 for source_bytes in order to count the source string; (runs len on it)
		char* copy(char* destination, const char* source, size_t source_bytes);
		size_t len(const char* str);
		char* cat(char* destination, const char* source);
		char* ncat(char* destination, const char* source, size_t destination_size);

		// use count = 0 to mean: pick the smallest string
		int case_insensitive_compare(const char* s1, const char* s2, size_t count);
		const char* strstr(const char* s1, const char* s2);

		// Returns the index of the last slash in the string.
		// returns -1 if no slash is found.
		intptr_t find_last_slash(const char* cstring);

		// returns the basename of the string
		const char* basename(const char* cstring);

		// this accepts a path entered by the user (possibly on the commandline)
		// and returns an expanded absolute path for use.
		// this should take into account leading tilde (~), which denotes the
		// special $(HOME) environment variable.
		// std::string make_absolute_path(const std::string& path);

		// effectively appending "../" and resolving.
		char* directory_up(char* source);

		std::vector<std::string> split(const std::string& input, const std::string& substring);
		std::string trim_left(const std::string& input, const std::string& chars = "\t ");

		template <size_t N>
		constexpr size_t const_length(char const(&)[N])
		{
			return N-1;
		}

		// It would help if I made this a better hash function in the future;
		// but for now, let's see how many collisions this causes.
		template <size_t index>
		constexpr uint32_t hash_string32(const char* string)
		{
			return hash_string32<index - 1>(string) + ((string[index] * index) & 0x0000ff);
		}

		template <>
		constexpr uint32_t hash_string32<size_t(-1)>(const char*)
		{
			return 0x0;
		}


		template <class T>
		void parse_value_from_string(T* value, const char* token);
	} // namespace str

#if 0
	struct str_t
	{
		// Semi-optimized string class for performant use of string data.
		// Implements copy-on-write semantics.

		str_t(gemini::Allocator& memory_allocator,
			const char* str);
		str_t(gemini::Allocator& memory_allocator,
			const char* str,
			size_t start,
			size_t length);

		void reallocate(size_t new_size);
		~str_t();
		void recalculate_size();
		bool operator==(const char* other);
		str_t& operator=(const str_t& other);

		// returns the size of the data in bytes
		size_t size() const;

		// returns the length of the string in characters
		size_t length() const;

		// returns a c-style string
		const char* c_str() const;
		char& operator[](int index);
		void perform_copy_on_write();
		static str_t copy(gemini::Allocator& allocator, const char* source);
		char operator[](int index) const;

		char* data;

		// 1: This string instance owns allocated data that must be deallocated.
		// 2: data_size is stale
		uint32_t flags;
		uint32_t data_size;
		gemini::Allocator& allocator;
	}; // str_t
#endif

} // namespace core

namespace gemini
{

	// strings are immutable. You must use the string_* functions to operate
	// on them.
	struct string
	{
		typedef const char value_type;

		size_t string_data_size;
		const char* string_data;

		string()
		{
			string_data = nullptr;
			string_data_size = 0;
		} // string

		string(const string& other)
		{
			string_data = other.string_data;
			string_data_size = other.string_data_size;
		} // string

		size_t size() const
		{
			return string_data_size * sizeof(unsigned char);
		} // size

		size_t length() const
		{
			return string_data_size;
		} // length

		bool operator==(const string& other) const
		{
			if (other.string_data_size != string_data_size)
			{
				return false;
			}

			if (core::str::case_insensitive_compare(other.string_data, string_data, string_data_size) != 0)
			{
				return false;
			}

			return true;
		} // operator==

		bool operator==(const char* other)
		{
			return core::str::case_insensitive_compare(string_data, other, 0) == 0;
		} // operator==

		string& operator=(const string& other)
		{
			string_data = other.string_data;
			string_data_size = other.string_data_size;
			return *this;
		} // operator=

		const char* c_str() const
		{
			return string_data;
		} // c_str

		const char& operator[](int index) const;
	}; // string

	char* string_allocate(gemini::Allocator& allocator, size_t length);
	string string_create(gemini::Allocator& allocator, const char* data);
	string string_create(const char* data);
	void string_destroy(gemini::Allocator& allocator, string& string);
	string string_concat(gemini::Allocator& allocator, const string& first, const string& second);
	string string_substr(gemini::Allocator& allocator, const char* source, uint32_t start, uint32_t length);

	void string_split_lines(gemini::Allocator& allocator, Array<gemini::string>& pieces, const string& line, const char* delimiters = "\t ");
} // namespace gemini
