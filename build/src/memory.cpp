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
#include "memory.hpp"
#include <string.h> // for memset

#include <memory> // for malloc, free

#include <assert.h>

namespace memory
{
	const size_t MemoryHeaderSize = sizeof(size_t);
	IAllocator * _allocator = 0;
	
	class SimpleAllocator : public virtual IAllocator
	{
		size_t bytes_allocated;
		size_t active_allocations;
		size_t total_allocations;
		size_t total_bytes;
	public:
		SimpleAllocator() : bytes_allocated(0), active_allocations(0), total_allocations(0), total_bytes(0) {}
		
		virtual void * allocate( size_t bytes )
		{
			char * block = (char*)malloc( bytes+MemoryHeaderSize );
			if ( block )
			{
				// increment totals
				bytes_allocated += bytes+MemoryHeaderSize;
				++total_allocations;
				total_bytes += bytes+MemoryHeaderSize;
				++active_allocations;
				
				// set the leading bytes of the block to the allocation size
				size_t * memory_size = (size_t*)block;
				*memory_size = bytes;
				
				// advance the block pointer
				block += MemoryHeaderSize;
			}
			else
			{
				// oh noes, out of memory error!
			}
						
			return block;
		} // allocate
		
		virtual void deallocate( void * memory )
		{
			// it is entirely legal to delete on a null pointer,
			// but we don't need to do anything.
			if ( memory )
			{
				// get a header to the block pointer
				char * block = ((char*)memory) - MemoryHeaderSize;
				size_t * memory_size = (size_t*)block;

				// subtract allocations and sizes
				bytes_allocated -= (*memory_size + MemoryHeaderSize);
				--active_allocations;
				
				free( block );
			}
		} // deallocate
		
		virtual size_t activeBytes() const { return bytes_allocated; }
		virtual size_t activeAllocations() const { return active_allocations; }
		virtual size_t totalAllocations() const { return total_allocations; }
		virtual size_t totalBytes() const { return total_bytes; }
	}; // SimpleAllocator
	
	
		
	void startup()
	{
		static SimpleAllocator simple_allocator;
		_allocator = &simple_allocator;
	} // startup
	
	void shutdown()
	{
		// if you hit this, there may be a memory leak with the allocator
		assert( _allocator->totalAllocations() == 0 && _allocator->totalBytes() == 0 );
		_allocator = 0;
	} // shutdown
	
	
	IAllocator & allocator()
	{
		return *_allocator;
	}
	
}; // namespace memory