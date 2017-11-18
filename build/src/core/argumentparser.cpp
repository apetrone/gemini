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

#include <core/argumentparser.h>
#include <core/logging.h>
#include <core/str.h>

namespace gemini
{
	void argparse_create(gemini::Allocator& allocator, ArgumentParser* context)
	{
		context->root = nullptr;
		context->help = nullptr;
		context->allocator = &allocator;

		argparse_help(context, "Show this help description", "--help", "-h");
	}

	void argparse_destroy(gemini::Allocator& allocator, ArgumentParser* context)
	{
		for (ArgumentParser::Argument* current = context->root; current != nullptr; )
		{
			ArgumentParser::Argument* temp = current;
			if (current->string_value && !current->string_value->empty())
			{
				string_destroy(allocator, *current->string_value);
			}
			current = current->next;
			MEMORY2_DELETE(allocator, temp);
		}
	}

	void argparse_append(ArgumentParser* context, ArgumentParser::Argument* argument)
	{
		argument->next = context->root;
		context->root = argument;
	}

	void argparse_arg(ArgumentParser* context, const char* help_string, const char* long_name, const char* short_name, gemini::string* value, uint32_t flags)
	{
		ArgumentParser::Argument* argument = MEMORY2_NEW(*context->allocator, ArgumentParser::Argument);
		argument->help_string = help_string;
		argument->long_name = nullptr;
		argument->short_name = nullptr;
		argument->int_value = nullptr;
		argument->string_value = value;
		if (long_name)
		{
			argument->long_name = long_name + 2;
		}

		if (short_name)
		{
			argument->short_name = short_name + 1;
		}
		argument->flags = flags;
		argparse_append(context, argument);
	}

	void argparse_int(ArgumentParser* context, const char* help_string, const char* long_name, const char* short_name, int32_t* value, uint32_t flags)
	{
		ArgumentParser::Argument* argument = MEMORY2_NEW(*context->allocator, ArgumentParser::Argument);
		argument->help_string = help_string;
		argument->long_name = nullptr;
		argument->short_name = nullptr;
		argument->int_value = value;
		argument->string_value = nullptr;
		if (long_name)
		{
			argument->long_name = long_name + 2;
		}

		if (short_name)
		{
			argument->short_name = short_name + 1;
		}
		argument->flags = ArgumentParser::StoreBool | flags;
		argparse_append(context, argument);
	}

	void argparse_help(ArgumentParser* context, const char* help_string, const char* long_name, const char* short_name)
	{
		ArgumentParser::Argument* argument = MEMORY2_NEW(*context->allocator, ArgumentParser::Argument);
		argument->help_string = help_string;
		argument->long_name = nullptr;
		argument->short_name = nullptr;
		argument->int_value = nullptr;
		argument->string_value = nullptr;
		if (long_name)
		{
			argument->long_name = long_name + 2;
		}

		if (short_name)
		{
			argument->short_name = short_name + 1;
		}
		argument->flags = ArgumentParser::Help | ArgumentParser::Optional;
		argparse_append(context, argument);
	} // argparse_help

	void argparse_show_help(ArgumentParser* context)
	{
		// TODO: First pass, find the largest argument combination in characters.
		uint32_t widest_column = 8;
		ArgumentParser::Argument* current;

		for (current = context->root; current != nullptr; current = current->next)
		{
			if ((current->long_name && !current->short_name) || (!current->long_name && current->short_name))
			{
				// If there's only one...
				const char* argument;

				if (current->long_name)
				{
					argument = core::str::format("--%s", current->long_name);
				}
				else
				{
					argument = core::str::format("-%s", current->short_name);
				}

				const uint32_t column_length = static_cast<uint32_t>(core::str::len(argument));
				if (column_length > widest_column)
				{
					widest_column = column_length;
				}
			}
			else
			{
				const char* argument = core::str::format("--%s, -%s", current->long_name, current->short_name);
				uint32_t column_length = static_cast<uint32_t>(core::str::len(argument)) + 2;
				if (column_length > widest_column)
				{
					widest_column = column_length;
				}
			}
		}

		for (current = context->root; current != nullptr; current = current->next)
		{
			const char* argument;
			if ((current->long_name && !current->short_name) || (!current->long_name && current->short_name))
			{
				// If there's only one...
				if (current->long_name)
				{
					argument = core::str::format("--%s", current->long_name);
				}
				else
				{
					argument = core::str::format("-%s", current->short_name);
				}
			}
			else
			{
				argument = core::str::format("--%s, -%s", current->long_name, current->short_name);
			}
			uint32_t space_left = widest_column - static_cast<uint32_t>(core::str::len(argument));

			const char* prefix = "";
			const char* postfix = "";
			if (current->flags & ArgumentParser::Optional)
			{
				prefix = "[";
				postfix = "]";
			}
			else
			{
				space_left += 2;
			}
			LOGV("%s%s%s %*s: %s\n",
				prefix,
				argument,
				postfix,
				space_left,
				"",
				current->help_string
			);
		}
	} // argparse_show_help

	ArgumentParser::Argument* argparse_find_argument(ArgumentParser* context, const gemini::string& token, uint32_t is_short_name)
	{
		for (ArgumentParser::Argument* current = context->root; current != nullptr; current = current->next)
		{
			if (is_short_name)
			{
				if (token == current->short_name)
				{
					return current;
				}
			}
			else
			{
				if (token == current->long_name)
				{
					return current;
				}
			}
		}

		return nullptr;
	}

	int32_t argparse_parse(ArgumentParser* context, const Array<gemini::string>& tokens)
	{
		ArgumentParser::Argument* last_argument = nullptr;

		const size_t memory_size = 1024;
		char memory[memory_size];
		gemini::Allocator string_allocator = gemini::memory_allocator_linear(MEMORY_ZONE_DEFAULT, memory, memory_size);

		for (size_t index = 0; index < tokens.size(); ++index)
		{
			const gemini::string& token_string = tokens[index];
			//LOGV("%i arg = %s\n", index, token_string.c_str());

			Array<gemini::string> pieces(*context->allocator);
			string_split(string_allocator, pieces, token_string, "=");

			for (size_t token_index = 0; token_index < pieces.size(); ++token_index)
			{
				const gemini::string line = pieces[token_index];

				// Categorize the argument first.
				const size_t len = line.length();
				gemini::string token;
				if (len > 2 && (line[0] == '-' && line[1] == '-'))
				{
					token = string_create(&line[2]);

					//LOGV("found long name [%s]\n", token.c_str());
					ArgumentParser::Argument* ap = argparse_find_argument(context, token, 0);
					if (ap)
					{
						ap->flags |= ArgumentParser::Found;
						//LOGV("found %s; %s; %s\n", ap->long_name, ap->short_name, ap->help_string);
						last_argument = ap;
					}
				}
				else if (len == 2 && line[0] == '-')
				{
					token = string_create(&line[1]);
					//LOGV("found a short name [%s]\n", token.c_str());
					ArgumentParser::Argument* ap = argparse_find_argument(context, token, 1);
					if (ap)
					{
						ap->flags |= ArgumentParser::Found;
						//LOGV("found %s; %s; %s\n", ap->long_name, ap->short_name, ap->help_string);
						last_argument = ap;

						if (ap->flags & ArgumentParser::StoreBool)
						{
							*ap->int_value = 1;
							ap->flags |= ArgumentParser::ValueFound;
						}
					}
					else
					{
						LOGV("Unrecognized argument: -%s\n", token.c_str());
						break;
					}
				}
				else
				{
					token = line;
					//LOGV("found positional argument [%s]\n", token.c_str());
					if (last_argument)
					{
						//LOGV("set key val '%s' [%s] -> %s\n", last_argument->long_name, last_argument->short_name, line.c_str());
						if (last_argument->int_value)
						{
							*last_argument->int_value = atoi(line.c_str());
							last_argument->flags |= ArgumentParser::ValueFound;
						}
						else if (last_argument->string_value)
						{
							// If token is quoted, remove the quotes here:
							if (token[0] == '\"' && token[static_cast<int32_t>(token.length()) - 1] == '\"')
							{
								*last_argument->string_value = string_substr(*context->allocator, token.c_str(), 1, static_cast<uint32_t>(token.length()) - 2);
							}
							else
							{
								*last_argument->string_value = string_create(*context->allocator, token);
								assert(!last_argument->string_value->empty());
							}

							last_argument->flags |= ArgumentParser::ValueFound;
						}
					}
					last_argument = nullptr;
				}
			}
		}

		// If the user wanted to view help; don't complain about missing
		// arguments, etc.
		ArgumentParser::Argument* current = context->root;
		for (; current != nullptr; current = current->next)
		{
			if (current->flags & ArgumentParser::Help && current->flags & ArgumentParser::Found)
			{
				// stop everything and display help.
				argparse_show_help(context);
				return 1;
			}
		}

		current = context->root;
		for (; current != nullptr; current = current->next)
		{
			if (current->flags & ArgumentParser::Required)
			{
				const char* argname = current->long_name ? current->long_name : current->short_name;

				if (current->flags & ArgumentParser::Found)
				{
					if (!(current->flags & ArgumentParser::ValueFound))
					{
						LOGE("ERROR: Argument '%s' found, but expecting parameter.\n", argname);
						argparse_show_help(context);
						return -1;
					}
				}
				else
				{
					LOGE("ERROR: Missing required argument: '%s'\n", argname);
					argparse_show_help(context);
					return -1;
				}
			}
		}

		return 0;
	} // argparse_parse
} // namespace gemini