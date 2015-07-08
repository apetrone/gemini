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
//		 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------
#pragma once

template <class tracking_policy = default_tracking_policy>
struct HeapAllocator : public Allocator< HeapAllocator<tracking_policy> >
{
	typedef HeapAllocator<tracking_policy> this_type;
	typedef Allocator<this_type> dependent_name;
	tracking_policy tracker;
	
	HeapAllocator(Zone* memory_zone) :
		dependent_name(memory_zone)
	{
	}
	
	void* allocate(size_t size, size_t alignment, const char* filename, int line)
	{
		size = tracker.request_size(size, alignment);
		
		void* pointer = malloc(size);
		
		fprintf(stdout, "HeapAllocator [%s]: allocate: %p, %lu bytes, %s:%i\n", dependent_name::memory_zone->name(), pointer, (unsigned long)size, filename, line);
		
		pointer = tracker.track_allocation(pointer, size, alignment, filename, line);
		
		if (dependent_name::memory_zone->add_allocation(size) == 0)
		{
			return pointer;
		}
		return 0;
	}
	
	void deallocate(void* pointer)
	{
		// it is entirely legal to call delete on a null pointer,
		// but we don't need to do anything.
		if (!pointer)
		{
			return;
		}
		
		size_t allocation_size;
		pointer = tracker.untrack_allocation(pointer, allocation_size);
		
		fprintf(stdout, "HeapAllocator [%s]: deallocate %p\n", dependent_name::memory_zone->name(), pointer);
		dependent_name::memory_zone->remove_allocation(allocation_size);
		free(pointer);
	}
};
