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
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------
#pragma once

#include <core/mem.h>

template <class T>
class Array
{
public:
	typedef T value_type;
	typedef T* value_pointer;
	
	value_pointer data;
	size_t max_capacity;
	size_t total_elements;
	
	
private:
	value_pointer allocate(size_t count)
	{
		return MEMORY_NEW_ARRAY(value_type, count, core::memory::global_allocator());
	}
	
	void deallocate(value_pointer pointer)
	{
		MEMORY_DELETE_ARRAY(pointer, core::memory::global_allocator());
	}
	
	// grow to capacity
	void grow(size_t capacity)
	{
		if (capacity > max_capacity)
		{
			value_pointer expanded_data = allocate(capacity);
			if (data)
			{
				memcpy(expanded_data, data, sizeof(value_type) * total_elements);
				deallocate(data);
			}
			
			data = expanded_data;
			max_capacity = capacity;
		}
	}
public:
	
	class iterator
	{
		typedef Array<value_type> container_type;
		
	private:
		const container_type& container;
		size_t index;
		
	public:
		iterator(const container_type& container, size_t index) :
		container(container),
		index(index)
		{
		}
		
		iterator(const iterator& other) :
		container(other.container),
		index(other.index)
		{
		}
		
		iterator& operator= (const iterator& other) const
		{
			this->index = other.index;
			this->container = other.container;
			return *this;
		}
		
		bool operator== (const iterator& other) const
		{
			return (index == other.index);
		}
		
		bool operator!= (const iterator& other) const
		{
			return !(*this == other);
		}
		
		const iterator& operator++()
		{
			index++;
			return *this;
		}
		
		const value_type& operator* () const
		{
			return container[index];
		}
	};
	
	Array(size_t capacity = 16)
	{
		data = allocate(capacity);
		total_elements = 0;
		max_capacity = capacity;
	}
	
	~Array()
	{
		clear();
	}
	
	void resize(size_t count)
	{
		resize(count, value_type());
	}
	
	// resizes the container to contain count elements
	// unlike std::vector, we aren't going to remove + destroy elements
	// if count < total_elements.
	void resize(size_t count, const value_type& default_value)
	{
		grow(count);
		
		if (count > total_elements)
		{
			size_t offset = total_elements;
			for (size_t index = offset; index < count; ++index)
			{
				data[index] = default_value;
			}
		}
		
		total_elements = count;
	}

	void push_back(const value_type& item)
	{
		// could also use a ratio to detect 70% full
		if (total_elements >= max_capacity)
		{
			grow(max_capacity * 2);
		}
		
		data[total_elements++] = item;
	}
	
	
	void clear()
	{
		if (data)
		{
			deallocate(data);
			data = 0;
		}
		
		max_capacity = 0;
		total_elements = 0;
	}
	
	
	size_t size() const
	{
		return total_elements;
	}
	
	bool empty() const
	{
		return total_elements == 0;
	}
	
	T& operator[](int index)
	{
		return data[index];
	}
	
	const T& operator[](int index) const
	{
		return data[index];
	}
	
	T& back()
	{
		assert(total_elements > 0);
		return data[total_elements-1];
	}
	
	iterator begin() const
	{
		return iterator(*this, 0);
	}
	
	iterator end() const
	{
		return iterator(*this, total_elements);
	}
}; // Array
