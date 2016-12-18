// -------------------------------------------------------------
// Copyright (C) 2016- Adam Petrone
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

template <class T>
class LinearFreeList
{
private:
	struct Record
	{
		enum
		{
			Free,
			Used,
		};

		T* data;
		size_t state;
	};
	Array<Record> elements;

public:
	typedef T value_type;
	typedef T* value_pointer;
	typedef size_t Handle;
	gemini::Allocator& allocator;

	LinearFreeList(gemini::Allocator& allocator)
		: allocator(allocator)
	{
	}

	~LinearFreeList()
	{
		clear();
	}

	// acquire a new instance
	// This can fail if a size limit was imposed on the free list.
	Handle acquire()
	{
		// try to find a free slot
		const size_t total_elements = elements.size();
		for (size_t index = 0; index < total_elements; ++index)
		{
			Record& record = elements[index];
			if (record.state == Record::Free)
			{
				record.state = Record::Used;
				return index;
			}
		}

		// Unable to find a free slot. Allocate a new one.
		Record item;
		item.state = Record::Used;
		item.data = MEMORY2_NEW(allocator, T);
		Handle reference = elements.size();
		elements.push_back(item);
		return reference;
	}

	// return a handle to the free list
	void release(Handle handle)
	{
		assert(is_valid(handle));

		elements[handle].state = Record::Free;
	}

	bool is_valid(Handle handle) const
	{
		// If you hit this, an out of range handle was passed in.
		assert(handle < elements.size());
		return elements[handle].state == Record::Used;
	}

	T* from_handle(Handle handle)
	{
		if (is_valid(handle))
		{
			return elements[handle].data;
		}
		return nullptr;
	}

	size_t size() const
	{
		return elements.size();
	} // size

	void clear()
	{
		for (Record& record : elements)
		{
			MEMORY2_DELETE(allocator, record.data);
		}
		elements.clear();
	} // clear
}; // LinearFreeList