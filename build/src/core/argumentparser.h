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


#include <string>
#include <vector>

#include <core/dictionary.h>

namespace core
{
	namespace argparse
	{
		// Proof of concept inspired by docopt library for Python by Vladimir Keleshev
		// Must be C++
		// Must not use exceptions
		// Must compile with gcc4.8, clang, vs2010/vs2013

		struct TokenInfo;
		struct Pattern;

		typedef std::vector<std::string> TokenList;
		typedef std::vector<Pattern*> PatternList;

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
			
			T& at(size_t index) const
			{
				return const_cast<T&>(items[index]);
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
					//this->container = other.container;
					this->index = other.index;
					return *this;
				}
				
				size_t get_index() const { return index; }
			};
			
			Iterator erase(Iterator& it)
			{
				items.erase(items.begin()+it.get_index());
				--items_length;
				return Iterator(*this, index);
			}
			
			Iterator begin() const { return Iterator(*this, index); }
			Iterator end() const { return Iterator(*this, items_length); }
		}; // Wrapper

		
		typedef Wrapper<std::string> TokenWrapper;
		typedef Wrapper<Pattern*> PatternWrapper;
		
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
			
			
			virtual bool is_leaf() const { return false; }
			virtual bool is_branch() const { return false; }
			virtual bool matches(PatternWrapper& patterns) { return 0; }
			virtual const char* get_classname() const { return "Pattern"; }
			virtual Type get_type() const { return PT_Pattern; }
			
			template <class T>
			T* cast() { return static_cast<T*>(this); }
		};
		
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
			arguments(input_arguments),
			argument_index(0),
			pattern_index(0)
			{
			}
			
			~TokenInfo()
			{
				for (Pattern* p : input)
				{
					delete p;
				}
				input.clear();
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
		protected:
			std::string name;
			std::string value;
			
		public:
			virtual bool is_leaf() const { return true; }
			
			virtual const std::string& get_name() const { return name; }
			virtual const std::string& get_value() const { return value; }
			
			virtual const char* get_classname() const { return "LeafPattern"; }
			virtual Type get_type() const { return PT_LeafPattern; }
			
			virtual bool matches(PatternWrapper& patterns);
			virtual bool single_match(PatternWrapper& patterns);
		};
		
		
		// ---------------------------------------------------------------------
		// Argument
		// ---------------------------------------------------------------------
		
		struct Argument : public LeafPattern
		{
			Argument(const std::string& input_name, const std::string& input_value);
			virtual bool single_match(PatternWrapper& patterns);
			
			virtual const char* get_classname() const { return "Argument"; }
			virtual Type get_type() const { return PT_Argument; }
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
			
			virtual bool single_match(PatternWrapper& patterns);
		};
		
		struct Command : public Argument
		{
			Command(const std::string& input_name, const std::string& input_value) : Argument(input_name, input_value)
			{
			}
			
			virtual const char* get_classname() const { return "Command"; }
			virtual Type get_type() const { return PT_Command; }
			
			virtual bool single_match(PatternWrapper& patterns)
			{
				for (Pattern* p : patterns)
				{
					if (p->get_type() == PT_Argument)
					{
						Argument* argument = p->cast<Argument>();
						if (argument->get_value() == this->name)
						{
							patterns.pop();
							return true;
						}
					}
				}
				return false;
			}
		};
		
		
		struct BranchPattern : public Pattern
		{
			PatternList children;
			
			BranchPattern(const PatternList& child_list) : children(child_list)
			{
			}
			
			virtual bool is_branch() const { return true; }
			virtual const char* get_classname() const { return "BranchPattern"; }
			virtual Type get_type() const { return PT_BranchPattern; }
		};
		
		struct Required : public BranchPattern
		{
			Required(const PatternList& child_list) : BranchPattern(child_list)
			{
			}
			
			virtual bool matches(PatternWrapper& patterns)
			{
				bool matched = false;
				for(Pattern* child : children)
				{
					matched = child->matches(patterns);
					if (!matched)
					{
						return false;
					}
				}
				
				return matched;
			}
			
			virtual const char* get_classname() const { return "Required"; }
		};
		
		struct Optional : public BranchPattern
		{
			Optional(const PatternList& child_list) : BranchPattern(child_list)
			{
			}
			
			
			virtual bool matches(PatternWrapper& patterns)
			{
				for (Pattern* child : children)
				{
					child->matches(patterns);
				}
				return true;
			}
			
			virtual const char* get_classname() const { return "Optional"; }
		};
		
		struct OneOrMore : public BranchPattern
		{
			virtual const char* get_classname() const { return "OneOrMore"; }
		};
		
		struct Either : public BranchPattern
		{
			Pattern* left;
			Pattern* right;
			
			virtual const char* get_classname() const { return "Either"; }
		};
		
		
		
		// ---------------------------------------------------------------------
		// ArgumentParser
		// ---------------------------------------------------------------------
		
		class ArgumentParser
		{
		public:
			const char* docstring;
			std::vector<Required*> usage_patterns;
			PatternList options_registry;
			
			ArgumentParser();
			~ArgumentParser();
			
			
			void parse_patterns_from_tokens(PatternList& patterns, TokenWrapper& tokens);
			
			Option* find_option(PatternList& patterns,
								const std::string& shortname,
								const std::string& longname,
								int& found_options);
			
			Option* parse_long(TokenWrapper& tokens, PatternList& options);
			
			void parse_atom(TokenWrapper& tokens, PatternList& results);
			
			void parse_sequence(TokenWrapper& tokens, PatternList& results);
			
			void parse_expr(TokenWrapper& tokens, PatternList& results);
			
			
			void parse_usage(const std::string& formal_usage, const char* help_string);
			
			
			std::string get_section_regex(const std::string& name);
			std::vector<std::string> split(const std::string& input, const std::string& substring);
			std::string trim_left(const std::string& input, const std::string& chars = "\t ");
			std::vector<std::string> find_section(const char* docstring, const std::string& section_name, bool& section_was_found);
			
			
			void parse_options(std::vector<std::string> lines);
			
			void parse_usage(std::vector<std::string> lines);
			
			bool check_extra(bool enable_automatic_help, const char* version_string, PatternList& patterns);
			void print_docstring() const;
			core::Dictionary<std::string> parse(const char* docstring, int argc, char** argv, const char* version_string = "");
			
		}; // ArgumentParser
		
		
	} // namespace argparse
} // namespace core

