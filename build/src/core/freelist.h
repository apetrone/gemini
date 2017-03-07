// -------------------------------------------------------------
// Copyright (C) 2017- Adam Petrone
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

#include "typedefs.h"

#include <core/array.h>

#if 0
// USAGE
LinearFreeList<int> abc;
LinearFreeList<int>::Handle value = abc.acquire();
if (abc.is_valid(value))
{
	int* beh = abc.from_handle(value);
	*beh = 73;

	// done with that, return it to the free list.
	//abc.release(value);

	// try to acquire another (should be the same as beh was).
	value = abc.acquire();

	int* foo = abc.from_handle(value);
	*foo = 42;
}

for (size_t index = 0; index < abc.size(); ++index)
{
	if (abc.is_valid(index))
	{
		int* p = abc.from_handle(index);
		LOGV("index: %i, ptr: %p %i\n", index, p, *p);
	}
}
#endif


namespace gemini
{
	// ---------------------------------------------------------------------
	// Freelist
	// ---------------------------------------------------------------------
	const uint32_t InvalidHandle = { UINT32_MAX };

	template <class T>
	class Freelist
	{
	public:
		typedef T value_type;
		typedef T* value_pointer;
		typedef size_t Handle;
		uint32_t used_handles;

		Freelist(Allocator& in_allocator, const T& in_default_value = T())
			: allocator(in_allocator)
			, elements(in_allocator)
			, freelist(in_allocator)
			, handles(in_allocator)
			, default_value(in_default_value)
		{
			used_handles = 0;
		}

		Handle acquire()
		{
			Handle handle = InvalidHandle;
			if (freelist.empty())
			{
				handle = elements.size();
				elements.push_back(default_value);
				handles.push_back(handle);
			}
			else
			{
				handle = freelist.pop_back();
				handles[handle] = handle;
				elements[handle] = default_value;
			}
			++used_handles;
			return handle;
		}

		void release(Handle handle)
		{
			assert(handle >= 0 && handle < handles.size());
			if (handles[handle] != InvalidHandle)
			{
				handles[handle] = InvalidHandle;
				--used_handles;
			}
		}

		void set(Handle handle, const T& value)
		{
			assert(handles[handle] != InvalidHandle);
			elements[handle] = value;
		}

		T& from_handle(Handle handle)
		{
			assert(handles[handle] != InvalidHandle);
			return elements[handle];
		}

		size_t size() const
		{
			return used_handles;
		}

		struct Iterator
		{
			Freelist<T>* freelist;
			uint32_t last_index;
			Iterator(Freelist<T>* in_freelist = nullptr)
				: freelist(in_freelist)
				, last_index(0)
			{
			}

			Iterator(const Iterator& other)
				: freelist(other.freelist)
				, last_index(other.last_index)
			{
			}

			Iterator& operator++()
			{
				for (last_index += 1; last_index < freelist->handles.size(); ++last_index)
				{
					if (freelist->handles[last_index] != InvalidHandle)
					{
						break;
					}
				}
				return *this;
			}

			Iterator operator--()
			{
				Iterator iter(freelist);
				iter.last_index = last_index - 1;
				return iter;
			}

			bool operator==(const Iterator& other)
			{
				return freelist == other.freelist && \
					last_index == other.last_index;
			}

			bool operator!=(const Iterator& other)
			{
				return !operator==(other);
			}

			T& data()
			{
				return freelist->elements[last_index];
			}
		}; // Iterator

		friend struct Iterator;

		Iterator begin()
		{
			Iterator iter(this);
			iter.last_index = 0;
			return iter;
		}

		Iterator end()
		{
			Iterator iter(this);
			iter.last_index = handles.size();
			return iter;
		}

	private:
		gemini::Allocator& allocator;
		Array<T> elements;
		Array<size_t> freelist;
		Array<size_t> handles;

		T default_value;
		value_pointer data;
	}; // Freelist
} // namespace gemini