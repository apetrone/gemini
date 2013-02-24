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
#include "typedefs.h"
#include "memory_stl_allocator.hpp"
#include <string.h> // for memset

#if __APPLE__
	#include <memory> // for malloc, free (on OSX)
#elif LINUX
	#include <stdlib.h>
	#include <stdio.h> // for fprintf
#elif _WIN32
	#include <memory> // we'll see if this compiles...
#endif


namespace memory
{
	const size_t MemoryHeaderSize = sizeof(size_t);
	IAllocator * _allocator = 0;
	
	class SimpleAllocator : public virtual IAllocator
	{
		size_t num_active_bytes;
		size_t num_active_allocations;
		size_t num_total_allocations;
		size_t num_total_bytes;
	public:
		SimpleAllocator() : num_active_bytes(0), num_active_allocations(0), num_total_allocations(0), num_total_bytes(0) {}
		
		virtual void * allocate( size_t bytes )
		{
			size_t total_size = bytes+MemoryHeaderSize;
			char * block = (char*)malloc( bytes+MemoryHeaderSize );
			if ( block )
			{
//				fprintf( stdout, "+ %i bytes\n", (unsigned long)total_size );
			
				// increment totals
				num_active_bytes += bytes+MemoryHeaderSize;
				++num_total_allocations;
				num_total_bytes += bytes+MemoryHeaderSize;
				++num_active_allocations;
				
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
				num_active_bytes -= (*memory_size + MemoryHeaderSize);
				--num_active_allocations;
				
				free( block );
			}
		} // deallocate
		
		virtual size_t active_bytes() const { return num_active_bytes; }
		virtual size_t active_allocations() const { return num_active_allocations; }
		virtual size_t total_allocations() const { return num_total_allocations; }
		virtual size_t total_bytes() const { return num_total_bytes; }
	}; // SimpleAllocator
	
	void startup()
	{
		static SimpleAllocator simple_allocator;
		_allocator = &simple_allocator;
	} // startup
	
	void shutdown()
	{
		fprintf( stdout, "[memory-status] total_allocations = %zu, total_bytes = %zu\n", _allocator->total_allocations(), _allocator->total_bytes() );
		fprintf( stdout, "[memory-status] active_allocations = %zu, active_bytes = %zu\n", _allocator->active_allocations(), _allocator->active_bytes() );
		
		// if you hit this, there may be a memory leak!
		assert( _allocator->active_allocations() == 0 && _allocator->active_bytes() == 0 );
		_allocator = 0;
	} // shutdown
	
	
	IAllocator & allocator()
	{
		return *_allocator;
	}
	
}; // namespace memory
