// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM,OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
// -------------------------------------------------------------
#pragma once

#include <vector>

#include <platform/mem.h>

#define DECLARE_FACTORY_CLASS( class_name, abstract_class )\
	public:\
		static abstract_class* creator() { return CREATE(class_name); }\
		static const char* get_classname() { return #class_name; }

namespace gemini
{
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
					if ( (category != 0 && record.category_type == category) || (category == 0 && name != 0 && (xstr_nicmp(record.class_name, name, 0) == 0)) )
					{
						return &record;
					}
				}
				
				return 0;
			} // find a class
		}; // class Factory
	} // namespace core
} // namespace gemini