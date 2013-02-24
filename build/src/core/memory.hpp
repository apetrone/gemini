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
#pragma once

#include <string.h> // for size_t
#include <memory> // for placement new

namespace memory
{
	class IAllocator
	{
	public:
		virtual ~IAllocator() {}
		
		virtual void * allocate( size_t bytes ) = 0;
		virtual void deallocate( void * memory ) = 0;
		
		virtual size_t active_bytes() const = 0;
		virtual size_t active_allocations() const = 0;
		virtual size_t total_allocations() const = 0;
		virtual size_t total_bytes() const = 0;
	}; // IAllocator
	
	// initialize memory handling
	void startup();
	
	// shutdown services and optionally perform any metrics, leak detection, etc
	void shutdown();
	
	// instance of the active allocator
	IAllocator & allocator();

	// raw memory alloc/dealloc
	#define ALLOC(byte_count)	memory::allocator().allocate(byte_count)
	#define DEALLOC(pointer) { memory::allocator().deallocate(pointer); pointer = 0; }
	
	// helper macros for alloc and dealloc on classes and structures	
	#define CREATE(Type, ...)	new (memory::allocator().allocate(sizeof(Type))) Type(__VA_ARGS__)
	#define DESTROY(Type, pointer) { pointer->~Type(); memory::allocator().deallocate(pointer); pointer = 0; }
	
	// at the moment: this only works if the Type has a default constructor
	#define CREATE_ARRAY(Type, num_elements, ...)		new (memory::allocator().allocate(sizeof(Type)*num_elements)) Type[ num_elements ]
	#define DESTROY_ARRAY(Type, pointer, num_elements) if ( pointer ) { for( size_t i = 0; i < num_elements; ++i ) { (&pointer[i])->~Type(); } memory::allocator().deallocate(pointer); pointer = 0;  }

	
}; // namespace memory

#include "memory_stl_allocator.hpp"

#define GeminiAllocator memory::DebugAllocator