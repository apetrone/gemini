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
	size_t total_elements;
	
	FixedArray(const FixedArray<Type>& other) {}
	FixedArray<Type> & operator=(const FixedArray<Type>& other) {}
	
private:
	void assert_valid_index(size_t index)
	{
		assert( elements != 0 );
		assert( index >= 0 && index < total_elements );
	}
	
public:
	
	FixedArray()
	{
		elements = 0;
		total_elements = 0;
	} // FixedArray
	
	~FixedArray()
	{
		clear();
	} // ~FixedArray
	
	size_t size() const
	{
		return total_elements;
	} // size
	
	bool empty() const
	{
		return (total_elements == 0);
	}
	
	void clear()
	{
		if ( elements )
		{
			DESTROY_ARRAY(Type, elements, total_elements);
			total_elements = 0;
		}
	} // clear
	
	void allocate(size_t element_total, bool zero_memory = false)
	{
		clear();
		total_elements = element_total;
		
		// allocate space for the pointers
		elements = CREATE_ARRAY(Type, total_elements);

		// optionally, zero the new memory
		if (zero_memory)
		{
			memset(elements, 0, sizeof(Type) * total_elements);
		}
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