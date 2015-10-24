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

#ifndef memcpy
	#include <string.h> // for memcpy
#endif

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
	public:
		typedef Array<value_type> container_type;

		iterator(container_type* container = nullptr, size_t index = 0) :
			container(container),
			index(index)
		{
		}
		
		iterator(const iterator& other) :
			container(other.container),
			index(other.index)
		{
		}
		
		iterator& operator= (const iterator& other)
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

		iterator operator++(int)
		{
			iterator out(*this);
			++out.index;
			return out;
		}

		value_type& operator* ()
		{
			return (*container)[index];
		}

		iterator operator+(int advance) const
		{
			iterator out(*this);
			out.index = (index + advance);
			return out;
		}

		size_t operator-(const iterator& other) const
		{
			assert(index >= other.index);
			return (index - other.index);
		}

		iterator operator-(const int& decrement) const
		{
			iterator out(*this);
			out.index = index - decrement;
			return out;
		}

		bool operator< (const iterator& other) const
		{
			return (index < other.index);
		}

		void swap(const iterator& other)
		{
			if (index != other.index)
			{
				container->swap(index, other.index);
			}
		}

	private:
		container_type* container;
		size_t index;
	};

	class reverse_iterator
	{
		typedef Array<value_type> container_type;

	private:
		const container_type* container;
		size_t index;

	public:
		reverse_iterator(const container_type* container = nullptr, size_t index = 0) :
			container(container),
			index(index)
		{
		}

		reverse_iterator(const reverse_iterator& other) :
			container(other.container),
			index(other.index)
		{
		}

		reverse_iterator& operator= (const reverse_iterator& other)
		{
			this->index = other.index;
			this->container = other.container;
			return *this;
		}

		bool operator== (const reverse_iterator& other) const
		{
			return (index == other.index);
		}

		bool operator!= (const reverse_iterator& other) const
		{
			return !(*this == other);
		}

		const reverse_iterator& operator++()
		{
			index--;
			return *this;
		}

		reverse_iterator operator++(int)
		{
			reverse_iterator out(*this);
			--out.index;
			return *this;
		}

		const value_type& operator* () const
		{
			return (*container)[index];
		}
	};


	Array(size_t capacity = 0)
	{
		data = nullptr;
		if (capacity > 0)
		{
			data = allocate(capacity);
		}

		total_elements = 0;
		max_capacity = capacity;
	}
	
	~Array()
	{
		clear();
	}

	void swap(size_t index, size_t other)
	{
		value_type temp = data[index];
		data[index] = data[other];
		data[other] = temp;
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
			if (max_capacity == 0)
				max_capacity = 8;
				
			grow(max_capacity * 2);
		}
		
		data[total_elements++] = item;
	}

	// pop the last element
	value_type pop_back()
	{
		assert(total_elements > 0);

		value_type item = data[total_elements-1];
		--total_elements;
		return item;
	}

	// reset capacity and total elements
	// optionally purges allocated data
	void clear(bool purge = true)
	{
		if (data && purge)
		{
			deallocate(data);
			data = 0;
			max_capacity = 0;
		}
		
		total_elements = 0;
	}
	

	void erase(const value_type& element)
	{
		if (empty())
		{
			return;
		}

		for (size_t index = total_elements; index > 0; --index)
		{
			if (data[index-1] == element)
			{
				value_pointer item = &data[index-1];
				item->~value_type();

				--total_elements;

				// shift elements
				for (size_t i = 0; i < total_elements; ++i)
				{
					data[(index-1) + i] = data[index+i];
				}
			}
		}
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
	
	iterator begin()
	{
		return iterator(this, 0);
	}
	
	iterator end()
	{
		return iterator(this, total_elements);
	}

	iterator begin() const
	{
		return iterator(this, 0);
	}

	iterator end() const
	{
		return iterator(this, total_elements);
	}

	reverse_iterator rbegin() const
	{
		return reverse_iterator(this, total_elements);
	}

	reverse_iterator rend() const
	{
		return reverse_iterator(this, 0);
	}
}; // Array
