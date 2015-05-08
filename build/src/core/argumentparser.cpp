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


#include "argumentparser.h"

#include <core/str.h>

#include <assert.h>
#include <stdio.h>
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
		

		bool LeafPattern::matches(int32_t& pattern_start, PatternWrapper& patterns, VariableMap& vars)
		{
			bool matched = false;
			if (this->get_type() == PT_Argument || this->get_type() == PT_Command || this->get_type() == PT_Option)
			{
				LeafPattern* leaf = this->cast<LeafPattern>();
				matched = leaf->single_match(pattern_start, patterns, vars);
			}
			
			return matched;
		}
		
		bool LeafPattern::single_match(int32_t& pattern_start, PatternWrapper& patterns, VariableMap& vars)
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
		
		bool Argument::single_match(int32_t& pattern_start, PatternWrapper& patterns, VariableMap& vars)
		{
			for (int32_t index = pattern_start; index < patterns.items_length; ++index)
			{
				PatternPtr p = patterns.at(index);
				if (p->get_type() == PT_Argument)
				{
					Argument* argument = p->cast<Argument>();

					vars[this->get_sanitized_name()] = argument->value;
					++pattern_start;
					return true;
				}
			}
			return false;
		}
		
		std::string Argument::get_sanitized_name() const
		{
			// strip off the leading and trailing brackets (<, >)
			std::string sanitized_name = this->get_name().substr(1, this->get_name().length()-2);
			return sanitized_name;
		}
		
		// ---------------------------------------------------------------------
		// Option
		// ---------------------------------------------------------------------
		
		bool Option::single_match(int32_t& pattern_start, PatternWrapper& patterns, VariableMap& vars)
		{
			for (PatternWrapper::Iterator it = std::begin(patterns); it != std::end(patterns); ++it)
			{
				PatternPtr p = (*it);
				if (p->get_type() == PT_Option)
				{
					Option* option = p->cast<Option>();
					if (this->longname == option->longname)
					{
						vars[option->longname] = option->get_value();

						// erase this item
						it = patterns.erase(it);
						
						if (pattern_start > 0)
						{
							--pattern_start;
						}
						return true;
					}
				}
			}
			return true;
		} // single_match
		
		// ---------------------------------------------------------------------
		// Command
		// ---------------------------------------------------------------------
		bool Command::single_match(int32_t& pattern_start, PatternWrapper &patterns, VariableMap &vars)
		{
			for (int32_t index = pattern_start; index < patterns.items_length; ++pattern_start)
			{
				PatternPtr p = patterns.at(index);
				if (p->get_type() == PT_Argument)
				{
					Argument* argument = p->cast<Argument>();
					if (argument->get_value() == this->name)
					{
						vars[this->get_name()] = argument->get_value();
						++pattern_start;
						return true;
					}
				}
			}
			return false;
		}
		
		// ---------------------------------------------------------------------
		// BranchPattern
		// ---------------------------------------------------------------------
		
		BranchPattern::~BranchPattern()
		{
			children.clear();
		}
		
		// ---------------------------------------------------------------------
		// Required
		// ---------------------------------------------------------------------
		bool Required::matches(int32_t &pattern_start, PatternWrapper &patterns, VariableMap &vars)
		{
			bool matched = false;
			for(PatternPtr child : children)
			{
				matched = child->matches(pattern_start, patterns, vars);
				if (!matched)
				{
					return false;
				}
			}
			
			return matched;
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
			usage_patterns.clear();
			options_registry.clear();
		}
		
		
		void ArgumentParser::parse_patterns_from_tokens(PatternList& patterns, TokenWrapper& tokens)
		{
			state = ParsingInput;
		
		
			int argument_index = 0;
			int pattern_index = 0;
			
			
			// first, we build a list of the input tokens and categorize these
			// into a series of patterns.
			while (true)
			{
				const std::string& arg = tokens.current();
				
				if (arg == "")
				{
					break;
				}
				
				if (arg == "--")
				{
					// TODO: the following arguments are positional
					tokens.pop();
					break;
				}
				else if (starts_with("--", arg))
				{
					// parse a long option
					parse_long(patterns, tokens, options_registry);
				}
				else if (starts_with("-", arg))
				{
					// parse a short option
					parse_short(patterns, tokens, options_registry);
				}
				else
				{
					// parse an argument
					PatternPtr newargument(new Argument("", arg));
					tokens.pop();
					patterns.push_back(newargument);
				}
			}
		}
		
		PatternPtr ArgumentParser::find_option(PatternList& patterns,
							const std::string& shortname,
							const std::string& longname,
							int& found_options)
		{
			PatternPtr option = 0;
			
			bool test_shortname = !shortname.empty();
			bool test_longname = !longname.empty();
			
			for (PatternPtr p : patterns)
			{
				if (p->get_type() == Pattern::PT_Option)
				{
					Option* o = p->cast<Option>();

					if (test_shortname && o->get_name() == shortname)
					{
						option = p;
						++found_options;
					}
					else if (test_longname && o->longname == longname)
					{
						option = p;
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
			
			
			OptionPtr option;
			int found_options = 0;
			std::string longname;
			std::string value;
			int total_arguments = 0;

			
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
			
			
			// get our first match; if one exists
			PatternPtr first_option = find_option(options, "", longname, found_options);
			
			if (state == ParsingInput && found_options == 0)
			{
				// No exact match; try fuzzy searching?
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

				option = OptionPtr(new Option("", longname, total_arguments));
				options.push_back(option);

				if (state == ParsingInput)
				{
					option = OptionPtr(new Option("", longname, total_arguments, value));
				}
			}
			else // exactly one match
			{
				Option* first = first_option->cast<Option>();
				option = OptionPtr(new Option(first->name,
									first->longname,
									first->total_arguments,
									first->value));
				
				if (option->total_arguments == 0)
				{
					if (!value.empty())
					{
						set_error("%s must not have an argument", option->longname.c_str());
					}
					
					value = "true";
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
			
			std::string left = core::str::trim_left(token, "-");


			size_t total_size = left.length();
			for (size_t index = 0; index < total_size; ++index)
			{
				std::string shortname = "-";
				std::string longname;
				std::string value;
				OptionPtr option;

				int found_options = 0;
				shortname += left[index];
				char next = 0;
				

				PatternPtr first_option = find_option(options, shortname, "", found_options);
				
				if (found_options > 1)
				{
					set_error("raise error: %s is specified ambiguously %i times", shortname.c_str(), found_options);
					break;
				}
				else if (found_options < 1)
				{
					option = OptionPtr(new Option(shortname, "", 0));
					options.push_back(option);
					
					if (state == ParsingInput)
					{
						option = OptionPtr(new Option(shortname, "", 0, "true"));
					}
				}
				else
				{
					Option* first = first_option->cast<Option>();
					option = OptionPtr(new Option(first->name, first->longname, first->total_arguments, first->value));
					if (option->total_arguments)
					{
						if (next == 0)
						{
							const std::string& current_token = tokens.current();
							if (starts_with(current_token, "--") || current_token.empty())
							{
								fprintf(stderr, "%s requires argument!\n", shortname.c_str());
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
					fprintf(stderr, "TODO: implement parentheses!\n");
				}
				else if (token == "[")
				{
					PatternList option_results;
					parse_expr(tokens, option_results);
					if (tokens.current() != "]")
					{
						fprintf(stderr, "Unmatched '%s'\n", "]");
					}
					tokens.pop();
					PatternPtr optional = PatternPtr(new Optional(option_results));
					results.push_back(optional);
					return;
				}
			}
			else if (starts_with("--", token) && token != "--")
			{
				parse_long(results, tokens, options_registry);
			}
			else if (starts_with("-", token) && token != "-")
			{
				parse_short(results, tokens, options_registry);
			}
			else if (starts_with("<", token) && ends_with(">", token))
			{
				results.push_back(PatternPtr(new Argument(tokens.pop(), "")));
			}
			else
			{
				results.push_back(PatternPtr(new Command(tokens.pop(), "")));
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
			//	results.push_back(PatternPtr(new Required(other_results)));
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
			
			usage_patterns.push_back(PatternPtr(new Required(usage_pattern)));
		}
		
		
		std::string ArgumentParser::get_section_regex(const std::string& name)
		{
			std::string regex = "(^\\s)*(";
			
			regex += name;
			
			regex += "[^\\n]*\\n?(?:[ \\t].*?(?:\\n|$))*)";
			
			return regex;
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
				std::vector<std::string> lines = core::str::split(match, "\n");
				for (std::string& line : lines)
				{
					std::string fixed_line = core::str::trim_left(line);
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
				std::vector<std::string> items = core::str::split(raw_option_line, "  ");
				
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
				
				
				std::string longname;
				std::string shortname;
				std::string value;
				int argument_count = 0;
				
				std::vector<std::string> elements = core::str::split(specifier, " ");
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

					}
				}

				OptionPtr option = OptionPtr(new Option(shortname,
											longname,
											argument_count,
											value));
											
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
				std::string replaced_string;
				std::regex replacement("([\\[\\]\\(\\)\\|]|\\.\\.\\.)");
				std::regex_replace(std::back_inserter(replaced_string),
					formal_usage.begin(),
					formal_usage.end(),
					replacement,
					" $1 ");
				
				// this needs to remove the commas from the usage pattern
				// otherwise it may incorrectly pickup bogus options: "-,"
				std::string str;
				std::remove_copy(replaced_string.begin(), replaced_string.end(), std::back_inserter(str), ',');
				
				// the above regex_replace will introduce a space when
				// options are specified first. Obviously, this is bad.
				// so we trim any whitespace before tokenizing it.
				str = core::str::trim_left(str);
				
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
				usage_patterns.push_back(PatternPtr(new Required(usage_pattern)));
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

		std::vector<std::string> ArgumentParser::split_tokens(int argc, char** argv)
		{
			std::vector<std::string> tokens;
			for (int index = 1; index < argc; ++index)
			{
				tokens.push_back(argv[index]);
			}

			return tokens;
		}

		std::vector<std::string> ArgumentParser::split_tokens(const char* commandline)
		{
			std::string source_commandline(commandline);
			std::regex rgx("(-*[\\\\:/a-zA-Z-]+)|(\"[\\\\:/a-zA-Z ]+\")");
			std::sregex_token_iterator iter(source_commandline.begin(),
				source_commandline.end(),
				rgx,
				1);
			std::sregex_token_iterator end;

			std::vector<std::string> tokens;
			for (; iter != end; ++iter)
			{
				tokens.push_back((*iter).str());
			}

			return tokens;
		}
			
		bool ArgumentParser::parse(const char* docstring, std::vector<std::string> tokens, VariableMap& vm, const char* version_string)
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
				fprintf(stderr, "Usage section not found!\n");
			}
			
			
			// build our token list
			TokenList tokenlist;
			for (const std::string& token : tokens)
			{
				tokenlist.push_back(token);
			}
			
			// create a wrapper pass this on
			TokenWrapper tokenwrapper(tokenlist, std::string(""));
			
			PatternList input_patterns;
			parse_patterns_from_tokens(input_patterns, tokenwrapper);
			
			
			bool automatic_help = true;
			bool should_exit = check_extra(automatic_help, version_string, input_patterns);
			if (should_exit)
			{
				// should bail here; displayed help or version string to user.
				return false;
			}
			
			
			
			// insert default option values for the items not in the dict
			for (PatternPtr option : options_registry)
			{
				Option* o = option->cast<Option>();
				std::string option_name;
				if (!o->get_name().empty())
				{
					option_name = o->get_name();
				}
				else if (!o->longname.empty())
				{
					option_name = o->longname;
				}
				std::string option_value = o->get_value();
				if (o->get_value().empty() && o->total_arguments == 0)
				{
					option_value = "false";
				}
				vm[option_name] = option_value;
			}
			
			PatternWrapper input(input_patterns, nullptr);

			bool success = false;
			for (PatternPtr usage : usage_patterns)
			{
				int32_t pattern_start = 0;
				success = usage->matches(pattern_start, input, vm);
				
				if (success)
				{
					break;
				}
				
				vm.clear();
			}
			
			if (!success)
			{
				print_docstring();
				return false;
			}
			
			return success;
		}
	} // namespace argparse
} // namespace core