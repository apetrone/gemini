// -------------------------------------------------------------
// Copyright (C) 2017- Adam Petrone
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

#include <core/typedefs.h>
#include <core/mem.h>
#include <core/str.h>

namespace gemini
{
	struct ArgumentParser
	{
		enum
		{
			// This argument is required
			Required = 1,

			// This argument is optional
			Optional = 2,

			// This is the help argument
			Help = 4,

			// This argument is stored as a string
			StoreString = 8,

			// This argument is stored as a boolean
			StoreBool = 16,

			// This argument was found during parsing.
			Found = 64,

			// This argument's value was found
			ValueFound = 128
		};

		struct Argument
		{
			const char* long_name;
			const char* short_name;
			const char* help_string;
			int32_t* int_value;
			gemini::string* string_value;
			uint32_t flags;
			struct Argument* next;
		};

		gemini::Allocator* allocator;
		Argument* root;
		Argument* help;
	};

	void argparse_create(gemini::Allocator& allocator, ArgumentParser* context);
	void argparse_destroy(gemini::Allocator& allocator, ArgumentParser* context);
	void argparse_append(ArgumentParser* context, ArgumentParser::Argument* argument);
	void argparse_arg(ArgumentParser* context, const char* help_string, const char* long_name, const char* short_name, gemini::string* value, uint32_t flags = ArgumentParser::Required);
	void argparse_int(ArgumentParser* context, const char* help_string, const char* long_name, const char* short_name, int32_t* value, uint32_t flags = ArgumentParser::Required);
	void argparse_help(ArgumentParser* context, const char* help_string, const char* long_name = "--help", const char* short_name = "-h");
	void argparse_show_help(ArgumentParser* context);
	ArgumentParser::Argument* argparse_find_argument(ArgumentParser* context, const gemini::string& token, uint32_t is_short_name);
	int32_t argparse_parse(ArgumentParser* context, const Array<gemini::string>& tokens);
} // namespace gemini



