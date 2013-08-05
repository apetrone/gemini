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
#include <stdio.h> // for stdout

namespace memory
{
#pragma pack(push, 4)
	struct MemoryHeader
	{
		size_t alloc_size;
		size_t alloc_num;
		const char * file;
		int line;
	};
#pragma pack(pop)

	const size_t MemoryHeaderSize = sizeof(MemoryHeader);
	IAllocator * _allocator = 0;
	
	class SimpleAllocator : public IAllocator
	{
		/*
			What follows are the following known limitations / drawbacks to this allocator:
				- Multiple inheritance is not supported. Using public virtual will cause
					an implicit conversion which is offset and therefore breaks the internal tracking of the allocator.
		*/
		size_t num_active_bytes;
		size_t num_active_allocations;
		size_t num_total_allocations;
		size_t num_total_bytes;
	public:
		SimpleAllocator() : num_active_bytes(0), num_active_allocations(0), num_total_allocations(0), num_total_bytes(0) {}
		
		virtual void * allocate( size_t bytes, const char * file, int line )
		{
//			size_t total_size = bytes+MemoryHeaderSize;

			char * block = (char*)malloc( bytes+MemoryHeaderSize );
			assert(block != 0);
			if ( block )
			{
//				fprintf( stdout, "+ %zu (%zu) bytes\n", total_size, bytes );
			
				// increment totals
				num_active_bytes += bytes+MemoryHeaderSize;
				++num_total_allocations;
				num_total_bytes += bytes+MemoryHeaderSize;
				++num_active_allocations;
				
				// set the leading bytes of the block to the allocation size
				MemoryHeader * mheader = (MemoryHeader*)block;
				mheader->alloc_size = bytes;
				mheader->alloc_num = num_total_allocations-1;
				mheader->file = file;
				mheader->line = line;

				// advance the block pointer
				block += MemoryHeaderSize;
//				fprintf(stdout, "memory_header = %p, block = %p\n", mheader, block);
			}
			else
			{
				// oh noes, out of memory error!
				assert( 0 );
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
				MemoryHeader * mheader = (MemoryHeader*)block;

				// subtract allocations and sizes
				num_active_bytes -= (mheader->alloc_size + MemoryHeaderSize);
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
	} // allocator
}; // namespace memory
