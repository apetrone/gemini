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
#pragma once

#include "typedefs.h"

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace core
{
	namespace argparse
	{
		// ArgumentParser heavily inspired by docopt library for Python
		// by Vladimir Keleshev.

		// My goals are the following:
		// - Must be C++
		// - Must not use exceptions or rtti
		// - Must compile with gcc4.8, clang, vs2013+
		// - Should follow docopt's naming and logic as closely as possible
		//   except for:

		//	 I'm requiring upper case for 'Usage' and 'Options'.
		//   There is no support for your program name at the beginning of usage.

		struct TokenInfo;
		struct Pattern;
		struct Option;

		typedef std::vector<std::string> TokenList;

		typedef std::shared_ptr<Pattern> PatternPtr;
		typedef std::shared_ptr<Option> OptionPtr;

		typedef std::vector<PatternPtr> PatternList;

		typedef std::unordered_map<std::string, std::string> VariableMap;
		typedef VariableMap::value_type VariableMapEntry;

		template <class T>
		struct Wrapper
		{
			typedef std::vector<T> ContainerType;
			ContainerType items;
			size_t index;
			const T default_value;
			size_t items_length;

			Wrapper(ContainerType& input_array, const T& default_value) :
				items(input_array),
				index(0),
				default_value(default_value)
			{
				items_length = items.size();
			}

			~Wrapper()
			{
			}

			Wrapper& operator=(const Wrapper& other)
			{
				items = other.items;
				index = other.index;
				default_value = other.default_value;
				items_length = other.items_length;
				return *this;
			}

			T current() const
			{
				if (items.empty())
				{
					return default_value;
				}

				if (index < items.size())
				{
					return items[index];
				}

				return default_value;
			}

			T pop()
			{
				if (items.empty())
				{
					return default_value;
				}

				return items[index++];
			}

			T& at(size_t requested_index) const
			{
				return const_cast<T&>(items[requested_index]);
			}

			size_t size() const
			{
				return items_length - index;
			}

			class Iterator
			{
			private:
				const Wrapper<T>& container;
				size_t index;

			public:
				Iterator(const Wrapper<T>& container, size_t index = 0) :
				container(container),
				index(index)
				{
				}

				Iterator(const Iterator& other) :
					container(other.container),
					index(other.index)
				{
				}

				bool operator!=(const Iterator& other)
				{
					return (index != other.index);
				}

				const Iterator& operator++()
				{
					++index;
					return *this;
				}

				T& operator*() const
				{
					return container.at(index);
				}

				Iterator& operator=(const Iterator& other)
				{
					this->index = other.index;
					return *this;
				}

				size_t get_index() const { return index; }
			};

			Iterator erase(Iterator& it)
			{
				items.erase(items.begin() + static_cast<intmax_t>(it.get_index()));
				--items_length;
				return Iterator(*this, index);
			}

			Iterator begin() const { return Iterator(*this, index); }
			Iterator end() const { return Iterator(*this, items_length); }
		}; // Wrapper


		typedef Wrapper<std::string> TokenWrapper;
		typedef Wrapper<PatternPtr> PatternWrapper;

		struct Pattern
		{
			enum Type
			{
				PT_Pattern,
				PT_Argument,
				PT_Command,
				PT_Option,

				PT_LeafPattern,
				PT_BranchPattern
			};

			virtual ~Pattern();

			virtual bool is_leaf() const { return false; }
			virtual bool is_branch() const { return false; }

			// pattern_start: The index of patterns to begin matching
			// returns true or false if it found a match
			virtual bool matches(uint32_t&, PatternWrapper&, VariableMap&) { return 0; }
			virtual const char* get_classname() const { return "Pattern"; }
			virtual Type get_type() const { return PT_Pattern; }

			template <class T>
			T* cast() { return static_cast<T*>(this); }
		}; // Pattern

		// ---------------------------------------------------------------------
		// TokenInfo
		// ---------------------------------------------------------------------

		struct TokenInfo
		{
			int argument_index;
			int pattern_index;
			std::vector<std::string>& arguments;
			PatternList input;

			TokenInfo(std::vector<std::string>& input_arguments) :
				argument_index(0),
				pattern_index(0),
				arguments(input_arguments)
			{
			}

			~TokenInfo()
			{
				input.clear();
			}

			TokenInfo& operator=(const TokenInfo& other)
			{
				argument_index = other.argument_index;
				pattern_index = other.pattern_index;
				arguments = other.arguments;
				input = other.input;
				return *this;
			}
		};


		// ---------------------------------------------------------------------
		// utils
		// ---------------------------------------------------------------------

		bool starts_with(const std::string& prefix, const std::string& str);
		bool ends_with(const std::string& postfix, const std::string& str);

		// ---------------------------------------------------------------------
		// LeafPattern
		// ---------------------------------------------------------------------

		struct LeafPattern : public Pattern
		{
			std::string name;
			std::string value;

			virtual bool is_leaf() const { return true; }

			virtual const std::string& get_name() const { return name; }
			virtual const std::string& get_value() const { return value; }

			virtual const char* get_classname() const { return "LeafPattern"; }
			virtual Type get_type() const { return PT_LeafPattern; }

			virtual bool matches(uint32_t& pattern_start, PatternWrapper& patterns, VariableMap& vars);

			// pattern_start is both input and output
			// Input, it indexes into patterns as a location to start matching.
			// If it finds a match (return true) and pattern_start should reference
			// the next index of patterns to begin matching.
			// This may not increment, for example, in the case of Optional arguments.
			virtual bool single_match(uint32_t& pattern_start, PatternWrapper& patterns, VariableMap& vars);
		};


		// ---------------------------------------------------------------------
		// Argument
		// ---------------------------------------------------------------------
		struct Argument : public LeafPattern
		{
			Argument(const std::string& input_name, const std::string& input_value);
			virtual bool single_match(uint32_t& pattern_start, PatternWrapper& patterns, VariableMap& vars);

			virtual const char* get_classname() const { return "Argument"; }
			virtual Type get_type() const { return PT_Argument; }

			virtual std::string get_sanitized_name() const;
		};

		// ---------------------------------------------------------------------
		// Option
		// ---------------------------------------------------------------------
		struct Option : public LeafPattern
		{
			std::string longname;
			int total_arguments;

			Option(const std::string& shortname,
				   const std::string& longname = "",
				   int argument_count = 0,
				   const std::string& value = "")
			{
				this->name = shortname;
				this->longname = longname;
				this->value = value;
				this->total_arguments = argument_count;
			}

			virtual const char* get_classname() const { return "Option"; }
			virtual Type get_type() const { return PT_Option; }
			virtual bool single_match(uint32_t& pattern_start, PatternWrapper& patterns, VariableMap& vars);
		};

		// ---------------------------------------------------------------------
		// Command
		// ---------------------------------------------------------------------

		struct Command : public Argument
		{
			Command(const std::string& input_name, const std::string& input_value) : Argument(input_name, input_value)
			{
			}

			virtual const char* get_classname() const { return "Command"; }
			virtual Type get_type() const { return PT_Command; }

			virtual bool single_match(uint32_t& pattern_start, PatternWrapper& patterns, VariableMap& vars);
		};

		// ---------------------------------------------------------------------
		// BranchPattern
		// ---------------------------------------------------------------------

		struct BranchPattern : public Pattern
		{
			PatternList children;

			BranchPattern(const PatternList& child_list) : children(child_list)
			{
			}

			~BranchPattern();

			virtual bool is_branch() const { return true; }
			virtual const char* get_classname() const { return "BranchPattern"; }
			virtual Type get_type() const { return PT_BranchPattern; }
		};

		// ---------------------------------------------------------------------
		// Required
		// ---------------------------------------------------------------------

		struct Required : public BranchPattern
		{
			Required(const PatternList& child_list) : BranchPattern(child_list)
			{
			}

			virtual bool matches(uint32_t& pattern_start, PatternWrapper& patterns, VariableMap& vars);
			virtual const char* get_classname() const { return "Required"; }
		};

		// ---------------------------------------------------------------------
		// Optional
		// ---------------------------------------------------------------------

		struct Optional : public BranchPattern
		{
			Optional(const PatternList& child_list) : BranchPattern(child_list)
			{
			}

			virtual bool matches(uint32_t& pattern_start, PatternWrapper& patterns, VariableMap& vars);
			virtual const char* get_classname() const { return "Optional"; }
		};

		// ---------------------------------------------------------------------
		// OneOrMore
		// ---------------------------------------------------------------------

		struct OneOrMore : public BranchPattern
		{
			OneOrMore(const PatternList& child_list) : BranchPattern(child_list)
			{
			}

			virtual ~OneOrMore() {}

			OneOrMore& operator=(const OneOrMore& other)
			{
				children = other.children;
				return *this;
			}

			virtual const char* get_classname() const { return "OneOrMore"; }
		};

		// ---------------------------------------------------------------------
		// Either
		// ---------------------------------------------------------------------

		struct Either : public BranchPattern
		{
			Either(const PatternList& child_list) : BranchPattern(child_list)
			{
			}

			virtual ~Either() {}

			Either& operator=(const Either& other)
			{
				left = other.left;
				right = other.right;
				children = other.children;
				return *this;
			}

			virtual const char* get_classname() const { return "Either"; }

			Pattern* left;
			Pattern* right;
		};

		// ---------------------------------------------------------------------
		// ArgumentParser
		// ---------------------------------------------------------------------

		class ArgumentParser
		{
			// In docopt, tokens.error is set to an exception type
			// when either parsing the Usage or parsing user input. This is
			// used as internal state when reading long and short options.
			// We equate ParsingUsage with DocoptLanguageError and
			// ParsingInput with DocoptExit.

			enum State
			{
				ParsingUsage,	// parsing the usage/doc string
				ParsingInput	// parsing user input passed on commandline
			};


			std::vector<PatternPtr> usage_patterns;
			PatternList options_registry;
			const char* docstring;
			State state;

			void parse_patterns_from_tokens(PatternList& patterns, TokenWrapper& tokens);

			PatternPtr find_option(PatternList& patterns,
				const std::string& shortname,
				const std::string& longname,
				int& found_options);

			void parse_long(PatternList& results, TokenWrapper& tokens, PatternList& options);
			void parse_short(PatternList& results, TokenWrapper& tokens, PatternList& options);
			void parse_atom(TokenWrapper& tokens, PatternList& results);
			void parse_sequence(TokenWrapper& tokens, PatternList& results);
			void parse_expr(TokenWrapper& tokens, PatternList& results);
			void parse_usage(const std::string& formal_usage, const char* help_string);


			std::string get_section_regex(const std::string& name);

			std::vector<std::string> find_section(const char* docstring_source, const std::string& section_name, bool& section_was_found);

			void parse_options(std::vector<std::string> lines);
			void parse_usage(std::vector<std::string> lines);
			bool check_extra(bool enable_automatic_help, const char* version_string, PatternList& patterns);

			void set_error(const char*, ...) {}

			/// @brief Split a space-delimited string up into argc/argv
			std::vector<std::string> split_commandline(const char* commandline);
		public:


			ArgumentParser();
			~ArgumentParser();

			void print_docstring() const;

			std::vector<std::string> split_tokens(int argc, char** argv);
			std::vector<std::string> split_tokens(const char* commandline);

			bool parse(const char* docstring_source, std::vector<std::string> tokens, VariableMap& vm, const char* version_string = "");
		}; // ArgumentParser


	} // namespace argparse
} // namespace core

