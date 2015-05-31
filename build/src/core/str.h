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

#include <platform/mem.h>

#include <string>
#include <vector>

typedef std::basic_string<char, std::char_traits<char>, CustomPlatformAllocator<char> > String;


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

		// use count = 0 to mean: pick the smallest string
		int case_insensitive_compare(const char* s1, const char* s2, size_t count);
		const char* strstr(const char* s1, const char* s2);
		
		// this accepts a path entered by the user (possibly on the commandline)
		// and returns an expanded absolute path for use.
		// this should take into account leading tilde (~), which denotes the
		// special $(HOME) environment variable.
		std::string make_absolute_path(const std::string& path);
		
		
		std::vector<std::string> split(const std::string& input, const std::string& substring);
		std::string trim_left(const std::string& input, const std::string& chars = "\t ");
	} // namespace str
} // namespace core
