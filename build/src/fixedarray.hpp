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

#include "memory.hpp"

template <class Type>
class FixedArray
{
	Type **elements;
	size_t element_count;
	
	FixedArray(const FixedArray<Type>& other) {}
	FixedArray<Type> & operator=(const FixedArray<Type>& other) {}
	
public:
	
	FixedArray()
	{
		elements = 0;
		element_count = 0;
	} // FixedArray
	
	~FixedArray()
	{
		purge();
	} // ~FixedArray
	
	size_t size() const
	{
		return element_count;
	} // size
	
	void purge()
	{
		if ( elements )
		{
			Type * element;
			for( size_t index = 0; index < element_count; ++index )
			{
				element = elements[index];
				if ( element )
				{
					DESTROY(Type, element);
				}
			}
			
			clear();
		}
	} // purge
	
	void clear()
	{
		if ( elements )
		{
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
		elements = (Type**)ALLOC( sizeof(Type*) * element_count );
		memset(elements, 0, sizeof(Type*) * element_count);
	} // allocate
	
	Type *& operator[](size_t index)
	{
		assert( elements != 0 );
		assert( index >= 0 && index < element_count );
		return elements[ index ];
	} // operator[]
}; // class FixedArray