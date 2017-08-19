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

#include <core/array.h>
#include <core/mem.h>
#include <core/typedefs.h>
#include <core/str.h>
#include <core/datastream.h>


namespace gemini
{
	struct TextFileContext
	{
		gemini::Allocator* allocator;
		core::util::MemoryStream stream;
		Array<unsigned char> file_data;
		uint32_t current_line;
		uint32_t current_column;
		char* current;
		void(*line_handler)(struct TextFileContext* context, const gemini::string& line, void* user_data);

		TextFileContext(gemini::Allocator& allocator);
	}; // TextFileContext

	void text_advance_character(TextFileContext* context);
	uint32_t text_stream_position(TextFileContext* context);
	bool text_eof(TextFileContext* context);
	uint32_t text_eat_comments(TextFileContext* context);
	char* text_advance_newline(TextFileContext* context, uint32_t* advance = nullptr);
	uint32_t text_read_lines(TextFileContext* context, void* user_data);
} // namespace gemini