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

#include <gemini/mem.h>

template <class Type>
class FixedArray
{
	Type *elements;
	size_t element_count;
	
	FixedArray(const FixedArray<Type>& other) {}
	FixedArray<Type> & operator=(const FixedArray<Type>& other) {}
	
private:
	void assert_valid_index(size_t index)
	{
		assert( elements != 0 );
		assert( index >= 0 && index < element_count );
	}
	
public:
	
	FixedArray()
	{
		elements = 0;
		element_count = 0;
	} // FixedArray
	
	~FixedArray()
	{
		clear();
	} // ~FixedArray
	
	size_t size() const
	{
		return element_count;
	} // size
	
	bool empty() const
	{
		return (element_count == 0);
	}
	
	void clear()
	{
		if ( elements )
		{
// 			This doesn't work. I suck at memory code.
			DESTROY_ARRAY(Type, elements, element_count);
//			Type * element;
//			for( size_t index = 0; index < element_count; ++index )
//			{
//				element = elements[index];
//				if ( element )
//				{
//					DESTROY(Type, element);
//				}
//			}
			
			DEALLOC(elements);
			elements = 0;
			element_count = 0;
		}
	} // clear
	
	void allocate(size_t total_elements)
	{
		clear();
		element_count = total_elements;
		
		// allocate space for the pointers
		elements = (Type*)ALLOC( sizeof(Type) * element_count );
		memset(elements, 0, sizeof(Type) * element_count);
	} // allocate
	
	Type& operator[](size_t index)
	{
		assert_valid_index(index);
		return elements[index];
	} // operator[]
	
	const Type& operator[](size_t index) const
	{
		assert_valid_index(index);
		return elements[index];
	} // operator[] const
}; // class FixedArray