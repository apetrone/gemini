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
	// initialize memory handling
	void startup();
	
	// shutdown services and optionally perform any metrics, leak detection, etc
	void shutdown();
	
	class IAllocator
	{
	public:
		virtual ~IAllocator() {}

		virtual void * allocate( size_t bytes ) = 0;
		virtual void deallocate( void * memory ) = 0;
		
		virtual size_t activeBytes() const = 0;
		virtual size_t activeAllocations() const = 0;
		virtual size_t totalAllocations() const = 0;
		virtual size_t totalBytes() const = 0;
	}; // IAllocator
	
	
	IAllocator & allocator();

	
	#define ALLOC(Type, ...)	new (memory::allocator().allocate(sizeof(Type))) Type(__VA_ARGS__)
	#define DEALLOC(Type, pointer) { pointer->~Type(); memory::allocator().deallocate(pointer); }
	
}; // namespace memory