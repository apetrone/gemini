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
						//patterns.pop();
						// we cannot pop, but we can erase.
						it = patterns.erase(it);
						return true;
					}
				}
				
				//LOGV("class: %s\n", p->get_classname());
			}
			return matched;
		}
	} // namespace argparse
} // namespace core