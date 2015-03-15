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

#include <assert.h>

#include "argumentparser.h"
#include <core/logging.h>

#include <platform/mem.h>

#include <regex>

namespace core
{
	namespace argparse
	{
		// ---------------------------------------------------------------------
		// utils
		// ---------------------------------------------------------------------
		
		bool starts_with(const std::string& prefix, const std::string& str)
		{
			std::string s2 = str.substr(0, prefix.length());
			return (prefix == s2);
		}
		
		bool ends_with(const std::string& postfix, const std::string& str)
		{
			std::string s2 = str.substr(str.length() - postfix.length(), postfix.length());
			return (postfix == s2);
		}
		
		// ---------------------------------------------------------------------
		// Pattern
		// ---------------------------------------------------------------------
		
		// ...
		
		// ---------------------------------------------------------------------
		// Leaf Pattern
		// ---------------------------------------------------------------------
		

		bool LeafPattern::matches(PatternWrapper& patterns)
		{
			bool matched = false;
			if (this->get_type() == PT_Argument || this->get_type() == PT_Command || this->get_type() == PT_Option)
			{
				LeafPattern* leaf = this->cast<LeafPattern>();
				matched = leaf->single_match(patterns);
			}
			
			return matched;
		}
		
		bool LeafPattern::single_match(PatternWrapper& patterns)
		{
			// should not get here.
			assert(0);
			return false;
		}
		
		// ---------------------------------------------------------------------
		// Argument
		// ---------------------------------------------------------------------
		
		
		Argument::Argument(const std::string& input_name, const std::string& input_value)
		{
			name = input_name;
			value = input_value;
		}
		
		bool Argument::single_match(PatternWrapper& patterns)
		{
			for (Pattern* p : patterns)
			{
				if (p->get_type() == PT_Argument)
				{
					patterns.pop();
					return true;
				}
			}
			return false;
		}
		
		
		// ---------------------------------------------------------------------
		// Option
		// ---------------------------------------------------------------------
		
		bool Option::single_match(PatternWrapper& patterns)
		{
			bool matched = false;
			for (PatternWrapper::Iterator it = std::begin(patterns); it != std::end(patterns); ++it)
			{
				Pattern* p = (*it);
				if (p->get_type() == PT_Option)
				{
					Option* option = p->cast<Option>();
					if (this->longname == option->longname)
					{
						LOGV("matched: %s -> '%s'\n", option->longname.c_str(), option->get_value().c_str());

						// erase this item
						it = patterns.erase(it);
						return true;
					}
				}
			}
			return matched;
		} // single_match
		
		
		// ---------------------------------------------------------------------
		// BranchPattern
		// ---------------------------------------------------------------------
		
		BranchPattern::~BranchPattern()
		{
			for (Pattern* child : children)
			{
				DESTROY(Pattern, child);
			}
			children.clear();
		}
		
		// ---------------------------------------------------------------------
		// ArgumentParser
		// ---------------------------------------------------------------------
		
		
		ArgumentParser::ArgumentParser()
		{
			state = ParsingUsage;
		}
		
		ArgumentParser::~ArgumentParser()
		{
			for (Required* p : usage_patterns)
			{
				DESTROY(Required, p);
			}
			usage_patterns.clear();
			
			for (Pattern* p : options_registry)
			{
				DESTROY(Pattern, p);
			}
			options_registry.clear();
		}
		
		
		void ArgumentParser::parse_patterns_from_tokens(PatternList& patterns, TokenWrapper& tokens)
		{
			state = ParsingInput;
		
		
			int argument_index = 0;
			int pattern_index = 0;
			
			
			// first, we build a list of the input tokens and categorize these
			// into a series of patterns.
			size_t total_tokens = tokens.size();
			while (true)
			{
				const std::string& arg = tokens.current();
				
				if (arg == "")
				{
					LOGV("end of arguments\n");
					break;
				}
				
				if (arg == "--")
				{
					// the following arguments are positional
//					LOGV("The following arguments will be treated as positional\n");
					tokens.pop();
					break;
				}
				else if (starts_with("--", arg))
				{
					// parse a long option
					LOGV("\"%s\" is a long option\n", arg.c_str());
					parse_long(patterns, tokens, options_registry);
				}
				else if (starts_with("-", arg))
				{
					// parse a short option
					LOGV("\"%s\" is a short option (or group of)\n", arg.c_str());
					parse_short(patterns, tokens, options_registry);
				}
				else
				{
					// parse an argument
					LOGV("\"%s\" is an argument\n", arg.c_str());
					Argument* newargument = CREATE(Argument, "", arg);
					tokens.pop();
					patterns.push_back(newargument);
				}
			}
			
			// now that info.input is populated, we can determine how to map
			// them to the usage branches.
			
			
		}
		
		Option* ArgumentParser::find_option(PatternList& patterns,
							const std::string& shortname,
							const std::string& longname,
							int& found_options)
		{
			Option* option = 0;
			
			bool test_shortname = !shortname.empty();
			bool test_longname = !longname.empty();
			
			for (Pattern* p : patterns)
			{
				if (p->get_type() == Pattern::PT_Option)
				{
					Option* o = p->cast<Option>();

					if (test_shortname && o->get_name() == shortname)
					{
						option = o;
						++found_options;
					}
					else if (test_longname && o->longname == longname)
					{
						option = o;
						++found_options;
					}
				}
			}
			return option;
		}
		
		void ArgumentParser::parse_long(PatternList& results, TokenWrapper& tokens, PatternList& options)
		{
			// long ::= '--' chars [ ( ' ' | '=' ) chars ] ;
			// split at the '=', if one exists.
			std::string long_arg = tokens.pop();
			
			// partition: '--longname=<argument>'
			// to: ('--longname', '=', '<argument>')
			
			
			Option* option = 0;
			int found_options = 0;
			std::string longname;
			std::string value;
			int total_arguments = 0;
			
			// get our first match; if one exists
			Option* first = find_option(options, "", longname, found_options);
			
			if (state == ParsingInput && found_options == 0)
			{
				// No exact match; try fuzzy searching?
			}
			
			size_t eq_pos = long_arg.find('=');
			if (eq_pos != std::string::npos)
			{
				longname = long_arg.substr(0, eq_pos);
				total_arguments = 1;
				value = long_arg.substr(eq_pos+1);
			}
			else
			{
				longname = long_arg;
			}
			
			
			if (found_options > 1)
			{
				// specified ambiguously 2+ times?
				set_error("%s is not a unique prefix!", longname.c_str());
			}
			else if (found_options < 1)
			{
				// TODO: grab the existing option
				// if it accepts an argument, then consume one (if we can)
				if (eq_pos != std::string::npos)
				{
					total_arguments = 1;
				}

				option = CREATE(Option, "", longname, total_arguments);
				options.push_back(option);

				if (state == ParsingInput)
				{
					option = CREATE(Option, "", longname, total_arguments, value);
				}
			}
			else // exactly one match
			{
				option = CREATE(Option, first->name,
									first->longname,
									first->total_arguments,
									first->value);
				
				if (option->total_arguments == 0)
				{
					if (!value.empty())
					{
						set_error("%s must not have an argument", option->longname.c_str());
					}
				}
				else
				{
					if (value.empty())
					{
						if (starts_with(tokens.current(), "--") || tokens.current().empty())
						{
							set_error("%s requires argument", option->longname.c_str());
						}
						value = tokens.pop();
					}
					
				}
				
				if (state == ParsingInput)
				{
					// update the option's value
					if (!value.empty())
					{
						option->value = value;
					}
				}
			}
			
			results.push_back(option);
		}
		
		void ArgumentParser::parse_short(PatternList& results, TokenWrapper &tokens, PatternList &options)
		{
			// shorts ::= '-' ( chars )* [ [ ' ' ] chars ] ;
			std::string token = tokens.pop();
			
			assert(token[0] == '-');
			
			std::string left = trim_left(token, "-");
//			LOGV("left: %s\n", left.c_str());

			size_t total_size = left.length();
			for (size_t index = 0; index < total_size; ++index)
			{
				std::string shortname = "-";
				std::string longname;
				std::string value;
				Option* option = 0;

				int found_options = 0;
				shortname += left[index];
				char next = 0;
				if (index < total_size-1)
					next = left[index+1];
				

//				LOGV("searching for shortname: '%s'\n", shortname.c_str());
				Option* first = find_option(options, shortname, "", found_options);
				
				if (found_options > 1)
				{
					set_error("raise error: %s is specified ambiguously %i times", shortname.c_str(), found_options);
					break;
				}
				else if (found_options < 1)
				{
					option = CREATE(Option, shortname, "", 0);
					options.push_back(option);
					
					if (state == ParsingInput)
					{
						option = CREATE(Option, shortname, "", 0, "true");
					}
				}
				else
				{
					LOGV("found an option '%s', '%s'\n", first->name.c_str(), first->longname.c_str());
					option = CREATE(Option, first->name, first->longname, first->total_arguments, first->value);
					if (option->total_arguments)
					{
						if (next == 0)
						{
							const std::string& current_token = tokens.current();
							if (starts_with(current_token, "--") || current_token.empty())
							{
								LOGV("%s requires argument!\n", shortname.c_str());
							}
							value = tokens.pop();
						}
						else
						{
							value = left;
							index = total_size;
						}
					}
					
					// update value
					if (state == ParsingInput && !value.empty())
					{
						option->value = value;
					}
				}
				
				results.push_back(option);
			}
		}
		
		void ArgumentParser::parse_atom(TokenWrapper& tokens, PatternList& results)
		{
			// atom ::= '(' expr ')' | '[' expr ']' | 'options'
			//	| longname | shortnames | argument | command ;
			const std::string& token = tokens.current();
			
			if (token == "(" || token == "[")
			{
				tokens.pop();
				if (token == "(")
				{
					LOGV("TODO: implement parentheses!\n");
				}
				else if (token == "[")
				{
					PatternList option_results;
					parse_expr(tokens, option_results);
					if (tokens.current() != "]")
					{
						LOGV("Unmatched '%s'\n", "]");
					}
					tokens.pop();
					Optional* optional = CREATE(Optional, option_results);
					results.push_back(optional);
					return;
				}
			}
			else if (starts_with("--", token) && token != "--")
			{
				//LOGV("this is a long option\n");
				parse_long(results, tokens, options_registry);
			}
			else if (starts_with("-", token) && token != "-")
			{
				//LOGV("this is a short option\n");
				parse_short(results, tokens, options_registry);
			}
			else if (starts_with("<", token) && ends_with(">", token))
			{
				//LOGV("this is an argument\n");
				Argument* argument = CREATE(Argument, tokens.pop(), "");
				results.push_back(argument);
			}
			else
			{
				//LOGV("this is a command\n");
				Command* command = CREATE(Command, tokens.pop(), "");
				results.push_back(command);
			}
		}
		
		void ArgumentParser::parse_sequence(TokenWrapper& tokens, PatternList& results)
		{
			// sequence ::= ( atom [ '...' ] )* ;
			while (true)
			{
				std::string current = tokens.current();
				if (current == "" || current == "]" || current == ")" || current == "|")
				{
					//LOGV("detected end of sequence\n");
					break;
				}
				
				parse_atom(tokens, results);
			}
		}
		
		void ArgumentParser::parse_expr(TokenWrapper& tokens, PatternList& results)
		{
			// expr ::= sequence ( '|' sequence )* ;
			
			parse_sequence(tokens, results);
			
			//while (tokens.current() == "|")
			//{
			//	tokens.pop();
			//	PatternList other_results;
			//	parse_sequence(tokens, other_results);
			//	results.push_back(CREATE(Required, other_results));
			//}
			
			
		}
		
		
		void ArgumentParser::parse_usage(const std::string& formal_usage, const char* help_string)
		{
			// the formal usage pattern needs to be split up into tokens
			// which can be fed into a regex.
			
			// first, we expand the usage string so we have extra whitespace
			// to separate elements.
			std::regex replacement("([\\[\\]\\(\\)\\|]|\\.\\.\\.)");
			std::string str;
			std::regex_replace(std::back_inserter(str), formal_usage.begin(), formal_usage.end(), replacement, " $1 ");
			
			TokenList usage;
			
			std::regex rgx("\\s+");
			std::sregex_token_iterator iter(str.begin(),
											str.end(),
											rgx,
											-1);
			std::sregex_token_iterator end;
			
			// next, we tokenize the expanded string
			for (; iter != end; ++iter)
			{
				usage.push_back( (*iter).str() );
			}
			
			// now we parse the expanded string
			TokenWrapper tw(usage, std::string(""));
			PatternList usage_pattern;
			parse_expr(tw, usage_pattern);
			
			usage_patterns.push_back(CREATE(Required, usage_pattern));
		}
		
		
		std::string ArgumentParser::get_section_regex(const std::string& name)
		{
			std::string regex = "(^\\s)*(";
			
			regex += name;
			
			regex += "[^\\n]*\\n?(?:[ \\t].*?(?:\\n|$))*)";
			
			return regex;
		}
		
		std::vector<std::string> ArgumentParser::split(const std::string& input, const std::string& substring)
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
		
		std::string ArgumentParser::trim_left(const std::string& input, const std::string& chars)
		{
			std::string out;
			
			std::size_t start = input.find_first_not_of(chars);
			if (start != std::string::npos)
			{
				out = input.substr(start);
			}
			
			return out;
		}
		
		std::vector<std::string> ArgumentParser::find_section(const char* docstring, const std::string& section_name, bool& section_was_found)
		{
			section_was_found = false;
			std::regex rgx(get_section_regex(section_name));
			std::cmatch result;
			std::regex_search(docstring, result, rgx);
			
			std::vector<std::string> output;
			
			if (result.size() > 0)
			{
				// found the section
				section_was_found = true;
				
				// split it at the colon ("Usage:")
				std::string match = result.str(0);
				std::size_t pos = match.find_first_of(':');
				if (pos != std::string::npos)
				{
					match = match.substr(pos+1);
				}
				
				// split section at newlines and trim leading whitespace
				std::vector<std::string> lines = split(match, "\n");
				for (std::string& line : lines)
				{
					std::string fixed_line = trim_left(line);
					if (!fixed_line.empty())
					{
						output.push_back(fixed_line);
					}
				}
			}
			
			return output;
		}
		
		
		void ArgumentParser::parse_options(std::vector<std::string> lines)
		{
			for (auto& raw_option_line : lines)
			{
				// the raw option line may also have a help string associated with it
				// so we need to split the string properly
//				LOGV("%s\n", raw_option_line.c_str());
				std::vector<std::string> items = split(raw_option_line, "  ");
				
				std::string specifier;
				std::string description;
				
				// handle the case where a description is not specified in the options
				// we still need to process the specifier (left side) and
				// description (right side)
				if (!items.empty())
				{
					specifier = items[0];
					if (items.size() > 1)
					{
						description = items[1];
					}
				}
				
				
				// parse an option
				
				// clean up the options a bit
				std::replace(specifier.begin(), specifier.end(), '=', ' ');
				std::string result;
				std::remove_copy(specifier.begin(), specifier.end(), std::back_inserter(result), ',');
				specifier = result;
//				LOGV("specifier: %s\n", specifier.c_str());
				
				
				std::string longname;
				std::string shortname;
				std::string value;
				int argument_count = 0;
				
				std::vector<std::string> elements = split(specifier, " ");
				for (const std::string& item : elements)
				{
					if (starts_with("--", item))
					{
						longname = item;
					}
					else if (starts_with("-", item))
					{
						shortname = item;
					}
					else
					{
						argument_count = 1;
					}
				}
				
				if (argument_count)
				{
					// try to read defaults from the description
					std::regex rgx("\\[default: (.*)\\]");
					std::cmatch matches;
					std::regex_search(description.c_str(), matches, rgx);
					if (matches.size() > 1)
					{
						value = matches.str(1);
//						LOGV("matched: %s\n", value.c_str());
					}
				}
				
//				LOGV("longname: %s\n", longname.c_str());
//				LOGV("shortname: %s\n", shortname.c_str());
//				LOGV("argument_count: %i\n", argument_count);
				
				Option* option = CREATE(Option, shortname,
											longname,
											argument_count,
											value);
											
				options_registry.push_back(option);
			}
		}
		
		void ArgumentParser::parse_usage(std::vector<std::string> lines)
		{
			for (auto& formal_usage : lines)
			{
				// the formal usage pattern needs to be split up into tokens
				// which can be fed into a regex.
								
				// first, we expand the usage string so we have extra whitespace
				// to separate elements.
				std::string str;
				std::regex replacement("([\\[\\]\\(\\)\\|]|\\.\\.\\.)");
				std::regex_replace(std::back_inserter(str),
					formal_usage.begin(),
					formal_usage.end(),
					replacement,
					" $1 ");
				
				// next, we tokenize the expanded string and insert these
				// into a vector.
				TokenList usage;
				std::regex rgx("\\s+");
				std::sregex_token_iterator iter(str.begin(),
												str.end(),
												rgx,
												-1);
				std::sregex_token_iterator end;

				for (; iter != end; ++iter)
				{
					usage.push_back( (*iter).str() );
				}
				
				// now we parse the expanded string
				TokenWrapper tw(usage, std::string(""));
				PatternList usage_pattern;
				parse_expr(tw, usage_pattern);

				// save the usage patterns from this formal_usage line under
				// a new required branch
				usage_patterns.push_back(CREATE(Required, usage_pattern));
			}
		}
		
		bool ArgumentParser::check_extra(bool enable_automatic_help, const char* version_string, PatternList& patterns)
		{
			int found_options = 0;
			if (enable_automatic_help && find_option(patterns, "-h", "--help", found_options))
			{
				print_docstring();
				return true;
			}
			
			if (version_string && find_option(patterns, "", "--version", found_options))
			{
				fprintf(stdout, "%s\n", version_string);
				return true;
			}
			
			return false;
		}
		
		void ArgumentParser::print_docstring() const
		{
			// print out the doc string
			fprintf(stdout, "%s\n", docstring);
		}
			
		core::Dictionary<std::string> ArgumentParser::parse(const char* docstring, int argc, char** argv, const char* version_string)
		{
			this->docstring = docstring;
			bool found_options = false;
			bool found_usage = false;
			
			// run through the docstring and parse the options first.
			parse_options(find_section(docstring, "Options", found_options));
			
			// parse usage block
			parse_usage(find_section(docstring, "Usage", found_usage));
			
			if (!found_usage)
			{
				LOGV("Usage section not found!\n");
			}
			
			
			// build our token list
			TokenList tokens;
			for (int i = 1; i < argc; ++i)
			{
				tokens.push_back(argv[i]);
			}
			
			// create a wrapper pass this on
			TokenWrapper tokenwrapper(tokens, std::string(""));
			
			PatternList input_patterns;
			parse_patterns_from_tokens(input_patterns, tokenwrapper);
			
			
			bool automatic_help = true;
			bool should_exit = check_extra(automatic_help, version_string, input_patterns);
			if (should_exit)
			{
				// should bail here; displayed help or version string to user.
				exit(0);
			}
			
			PatternWrapper input(input_patterns, nullptr);
			core::Dictionary<std::string> dict;
			
			bool success = false;
			for (Required* usage : usage_patterns)
			{
				success = usage->matches(input);
				
				// too many arguments specified
				if (success && input.size() > 0)
				{
					fprintf(stdout, "ArgumentParser: ignoring unrecognized arguments\n");
				}
			}
			
			if (!success)
			{
				print_docstring();
			}
			
			return dict;
		}
	} // namespace argparse
} // namespace core