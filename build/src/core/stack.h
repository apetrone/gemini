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

namespace gemini
{
	template <class T>
	class stack
	{
	public:
		typedef T value_type;
		typedef T* value_pointer;

		stack(gemini::Allocator& _allocator)
			: allocator(_allocator)
			, data(nullptr)
			, stack_pointer(0)
		{
			stack_pointer = ~0u;
			max_capacity = 0;
		}

		stack& operator=(const stack& other) = delete;

		void push(const T& item)
		{
			if (!data && max_capacity == 0)
			{
				grow(16);
			}

			if (stack_pointer+1 >= max_capacity)
			{
				grow(max_capacity << 1);
			}

			data[++stack_pointer] = item;
		}

		T pop()
		{
			validate_range();
			T item = data[stack_pointer--];
			return item;
		}

		T& top()
		{
			validate_range();
			return const_cast<T&>(data[stack_pointer]);
		}

		const T& top() const
		{
			validate_range();
			return data[stack_pointer];
		}

		size_t size() const
		{
			return stack_pointer + 1;
		}

		bool empty() const
		{
			return stack_pointer == ~0u;
		}

		void clear(bool purge = true)
		{
			stack_pointer = ~0u;
			max_capacity = 0;

			if (purge)
			{
				deallocate(data);
				data = nullptr;
			}
		}

	private:

		inline void validate_range()
		{
			assert(stack_pointer >= 0 && stack_pointer < ~0u);
		}

		value_pointer allocate(size_t count)
		{
			return MEMORY2_NEW_ARRAY(allocator, value_type, count);
		}

		void deallocate(value_pointer pointer)
		{
			MEMORY2_DELETE_ARRAY(allocator, pointer);
		}

		// grow to capacity
		void grow(size_t capacity)
		{
			if (capacity > max_capacity)
			{
				value_pointer expanded_data = allocate(capacity);
				assert(expanded_data != nullptr);
				if (data)
				{
					memcpy(expanded_data, data, sizeof(value_type) * max_capacity);
					deallocate(data);
				}

				data = expanded_data;
				max_capacity = static_cast<uint32_t>(capacity);
			}
		}

		gemini::Allocator& allocator;
		value_pointer data;
		uint32_t stack_pointer;
		uint32_t max_capacity;
	}; // class stack

} // namespace gemini