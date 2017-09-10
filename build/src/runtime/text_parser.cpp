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

#include <runtime/text_parser.h>
#include <core/datastream.h>
#include <core/serialization.h>
#include <runtime/filesystem.h>


namespace gemini
{
	TextFileContext::TextFileContext(gemini::Allocator& allocator)
		: file_data(allocator)
	{
	}

	void text_advance_character(TextFileContext* context)
	{
		uint32_t advance = 1;
		if (core::str::isnewline(context->current, &advance))
		{
			//LOGV("found newline!\n");
			++context->current_line;
			context->current_column = 0;
		}
		context->current += advance;
		context->current_column += advance;
	} // text_advance_character


	uint32_t text_stream_position(TextFileContext* context)
	{
		uint8_t* position = reinterpret_cast<uint8_t*>(context->current);
		uint8_t* stream = context->stream.get_data();
		return position - stream;
	} // text_stream_position

	bool text_eof(TextFileContext* context)
	{
		uint32_t stream_position = text_stream_position(context);
		return (stream_position >= context->stream.get_data_size() - 1);
	} // text_eof

	uint32_t text_eat_comments(TextFileContext* context)
	{
		char comment_char = '#';
		if (*context->current == comment_char)
		{
			for (;; )
			{
				uint32_t bail = 0;
				if (text_eof(context) || core::str::isnewline(context->current))
				{
					bail = 1;
				}

				text_advance_character(context);

				if (bail)
				{
					return 1;
				}
			}
		}

		return 0;
	} // text_eat_comments


	char* text_advance_newline(TextFileContext* context, uint32_t* advance)
	{
		uint32_t character_advance = 0;
		if (*context->current == '\n')
		{
			character_advance = 1;
			++context->current;
			context->current_column++;
		}
		else if (*context->current == '\r' && (*(context->current + 1) == '\n'))
		{
			character_advance = 2;
			context->current += 2;
			context->current_column += 2;
		}

		if (advance)
		{
			*advance = character_advance;
		}
		return context->current;
	} // text_advance_newlines


	void text_line_parse_keyvalues(TextFileContext* context, const gemini::string& line, void* user_data)
	{
		KeyValueArchive* archive = reinterpret_cast<KeyValueArchive*>(user_data);

		// Split string into pieces at the equals sign
		Array<gemini::string> pieces(*context->allocator);
		string_split(*context->allocator, pieces, line, "=");

		if (pieces.size() < 2)
		{
			return;
		}
		gemini::string& key = pieces[0];
		gemini::string& value = pieces[1];

		assert(archive);
		archive->set_item(key, value);
	} // text_line_parse_keyvalues

	uint32_t text_read_lines(TextFileContext* context, void* user_data)
	{
		// This expects the line_handler to be set.
		assert(context->line_handler);

		// initialize the context
		char* data = reinterpret_cast<char*>(context->stream.get_data());

		context->current_line = 0;
		context->current_column = 1;
		context->current = data;

		if (data == nullptr)
		{
			LOGW("No data to read.\n");
			return 0;
		}

		// declare some locals we'll use to parse lines.
		char* line_start = nullptr;
		char* last_character = nullptr;

		uint32_t reading_token = 0;

		while (!text_eof(context))
		{
			// TODO: Strip comments at end of lines

			if (!reading_token)
			{
				if (text_eat_comments(context))
				{
					// LOGV("skipping comment line at %i\n", context->current_line);
					continue;
				}

				if (isspace(*context->current))
				{
					text_advance_character(context);
					continue;
				}
			}

			// LOGV("found character %c [%i] at line %i, col %i\n",  *context->current, *context->current, context->current_line, context->current_column);
			if (!reading_token)
			{
				line_start = context->current;
				reading_token = 1;
			}
			else
			{
				char* prev_char = last_character + 1;
				if (text_eat_comments(context))
				{
					// Found comments while reading until EOL or EOF;
					// either way, we use the truncated line here.
					gemini::string token = string_substr(*context->allocator, line_start, 0, (prev_char - line_start));
					context->line_handler(context, token, user_data);
					reading_token = 0;
					line_start = nullptr;
					continue;
				}
			}

			// Found newline sequence (either '\r\n' or '\n')
			uint32_t advance;
			if (core::str::isnewline(context->current, &advance))
			{
				++context->current_line;
				context->current_column = 1;

				char* newline = context->current;

				uint32_t total_advance = 0;
				newline = text_advance_newline(context, &total_advance);

				uint32_t string_length = newline - line_start - advance;

				gemini::string token = string_substr(*context->allocator, line_start, 0, string_length);
				context->line_handler(context, token, user_data);
				reading_token = 0;
				line_start = nullptr;
				continue;
			}

			context->current += advance;
			if (isalnum(*context->current))
			{
				last_character = context->current;
			}
			context->current_column += advance;
		}

		if (reading_token)
		{
			// We hit this when the parser reaches the EOF, but is still reading a
			// token and there is no trailing newline in the file. We still must
			// handle the final line though.
			size_t string_length = context->current - line_start;
			gemini::string token = string_substr(*context->allocator, line_start, 0, string_length);
			context->line_handler(context, token, user_data);
		}

		// We increment this here to indicate we've read until the last line;
		// which at EOF, we have.
		context->current_line++;

		return 0;
	} // text_read_lines

	uint32_t text_context_from_file(TextFileContext* context, const char* path)
	{
		core::filesystem::IFileSystem* filesystem = core::filesystem::instance();
		if (!filesystem->virtual_file_exists(path))
		{
			return 1;
		}

		platform::Result result = filesystem->virtual_load_file(context->file_data, path);
		if (result.failed())
		{
			return 1;
		}

		if (!context->file_data.empty())
		{
			char* memory = reinterpret_cast<char*>(&context->file_data[0]);
			context->stream.init(memory, context->file_data.size());
		}

		return 0;
	} // text_context_from_file
} // namespace gemini