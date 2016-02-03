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

template <class tracking_policy = DefaultTrackingPolicy>
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

		void* pointer = core::memory::aligned_malloc(size, alignment);

//		fprintf(stdout, "HeapAllocator allocate: %x, %lu bytes, %s:%i\n", pointer, (unsigned long)size, filename, line);

		pointer = tracker.track_allocation(dependent_name::memory_zone, pointer, size, alignment, filename, line);

		if (dependent_name::memory_zone->add_allocation(size) == 0)
		{
			return pointer;
		}
		return 0;
	}

	void deallocate(void* pointer)
	{
		size_t allocation_size;
		pointer = tracker.untrack_allocation(dependent_name::memory_zone, pointer, allocation_size);

//		fprintf(stdout, "HeapAllocator deallocate %x\n", pointer);
		dependent_name::memory_zone->remove_allocation(allocation_size);

		core::memory::aligned_free(pointer);
	}
};
