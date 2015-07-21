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

#include "mem.h"

#ifndef memset
	#include <string.h>
#endif

#include <assert.h>

template <class Type>
class FixedArray
{
	Type *elements;
	size_t total_elements;

	FixedArray<Type> & operator=(const FixedArray<Type>& other) {}
	
private:
	void assert_valid_index(size_t index) const
	{
		assert(index >= 0 && index < total_elements);
	}
	
public:
	
	const Type* begin() const
	{
		return &elements[0];
	}
	
	const Type* end() const
	{
		return &elements[total_elements];
	}
	
	Type* begin()
	{
		return &elements[0];
	}
	
	Type* end()
	{
		return &elements[total_elements];
	}
	
	FixedArray()
	{
		elements = 0;
		total_elements = 0;
	} // FixedArray
	
	FixedArray(const FixedArray<Type>& other)
	{
		allocate(other.total_elements);
		memcpy(elements, other.elements, sizeof(Type) * total_elements);
	}
	
	~FixedArray()
	{
		clear();
	} // ~FixedArray
	
	void operator=(Type* other)
	{
		elements = other;
		total_elements = elements ? 1 : 0;
	}
	
	size_t size() const
	{
		return total_elements;
	} // size
	
	bool empty() const
	{
		return (total_elements == 0) && (elements == 0);
	}
	
	void clear()
	{
		if (elements && total_elements > 0)
		{
			MEMORY_DELETE_ARRAY(elements, core::memory::global_allocator());
			total_elements = 0;
		}
	} // clear
	
	void allocate(size_t element_total, bool zero_memory = false)
	{
		clear();
		total_elements = element_total;
		if (element_total > 0)
		{
			// allocate space for the pointers
			elements = MEMORY_NEW_ARRAY(Type, total_elements, core::memory::global_allocator());

			// optionally, zero the new memory
			if (zero_memory)
			{
				memset(elements, 0, sizeof(Type) * total_elements);
			}
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
}; // FixedArray


template <class T, size_t MaxSize>
struct CircularBuffer
{
	typedef T value_type;
	typedef FixedArray<value_type> container_type;
	container_type container;
	size_t index;
	
	CircularBuffer()
	{
		container.allocate(MaxSize);
		reset();
	}
	
	void reset()
	{
		index = 0;
	}
	
	T& next()
	{
		index = index % MaxSize;
		return container[index++];
	}
}; // CircularBuffer
