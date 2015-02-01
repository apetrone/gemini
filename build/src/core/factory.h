// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone
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

#include <vector>

#include <platform/mem.h>

#include <core/str.h>

#define DECLARE_FACTORY_CLASS( class_name, abstract_class )\
	public:\
		static abstract_class* creator() { return CREATE(class_name); }\
		static const char* get_classname() { return #class_name; }

namespace core
{
	template <class Type>
	class Factory
	{
	public:
		typedef Type * (*TypeCreator)();
		
		struct Record
		{
			TypeCreator creator;
			const char * class_name;
			unsigned int category_type;
		};
		
		typedef std::vector<Record> RecordContainer;
		RecordContainer records;
		
		//
		// Register a class creator with this factory.
		// It will associated by name, a create function.
		// Category is optional
		void register_class( TypeCreator creator, const char * name, unsigned int category = 0 )
		{
			Record record;
			record.creator = creator;
			record.class_name = name;
			record.category_type = category;
			records.push_back(record);
		} // register a class
		
		//
		// Find a class record by name or
		// if category is specified, by category
		Record* find_class( const char * name, unsigned int category = 0 )
		{
			for(auto& record : records)
			{
				// if this category matches the record, or if category is not-specified (0)
				// and the name matches
				if ( (category != 0 && record.category_type == category) || (category == 0 && name != 0 && (str::case_insensitive_compare(record.class_name, name, 0) == 0)) )
				{
					return &record;
				}
			}
			
			return 0;
		} // find a class
	}; // class Factory
} // namespace core