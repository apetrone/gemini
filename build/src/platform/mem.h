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

#include <string.h> // for size_t

#include "config.h"

#if PLATFORM_APPLE
	#include <memory> // for malloc, free (on OSX)
	#include <stdlib.h>
#elif PLATFORM_LINUX || PLATFORM_ANDROID
	#include <stdlib.h>
	#include <stdio.h> // for fprintf
	#include <new> // for placement new
#elif PLATFORM_WINDOWS
	#include <memory> // we'll see if this compiles...
#else
	#error Unknown platform!
#endif

// set this to 0 to enable normal new/delete and malloc/free to narrow down problems
#define USE_DEBUG_ALLOCATOR 1

namespace platform
{
	namespace memory
	{
		class IAllocator
		{
		public:
			virtual ~IAllocator() {}
			
			virtual void * allocate( size_t bytes, const char * file, int line ) = 0;
			virtual void deallocate( void * memory ) = 0;
			virtual void print_report() = 0;

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

		// helper function for creating arrays from a contiguous block of memory
		// and then calling placement new on each one
		template <class Type>
		Type* create_array(size_t num_elements, const char* file, int line)
		{
			Type* block = (Type*)memory::allocator().allocate(sizeof(Type)*num_elements, file, line);
			for (size_t i = 0; i < num_elements; ++i)
			{
				new (&block[i]) Type;
			}

			return block;
		}

#if USE_DEBUG_ALLOCATOR
		// raw memory alloc/dealloc
		#define ALLOC(byte_count)	platform::memory::allocator().allocate(byte_count, __FILE__, __LINE__)
		#define DEALLOC(pointer) { platform::memory::allocator().deallocate(pointer); pointer = 0; }
		
		// helper macros for alloc and dealloc on classes and structures
		#define CREATE(Type, ...)	new (platform::memory::allocator().allocate(sizeof(Type), __FILE__, __LINE__)) Type(__VA_ARGS__)
		#define DESTROY(Type, pointer) { if (pointer) { pointer->~Type(); platform::memory::allocator().deallocate(pointer); pointer = 0; } }
		
		// NOTES:
		// 1. This only works if the Type has a default constructor
		// 2. In order to work around the ridiculous "standard" of placement new in 5.3.4.12,
		//	  This allocates a contiguous block of memory and then calls placement new on each element.
		#define CREATE_ARRAY(Type, num_elements) platform::memory::create_array<Type>(num_elements, __FILE__, __LINE__)
		#define DESTROY_ARRAY(Type, pointer, num_elements) if ( pointer ) { for( size_t i = 0; i < num_elements; ++i ) { (&pointer[i])->~Type(); } platform::memory::allocator().deallocate(pointer); pointer = 0;  }
#else
		#define ALLOC(byte_count)	malloc(byte_count)
		#define DEALLOC(pointer) { free(pointer); pointer = 0; }

		#define CREATE(Type, ...)	new Type(__VA_ARGS__)
		#define DESTROY(Type, pointer) { delete pointer; pointer = 0; }

		#define CREATE_ARRAY(Type, num_elements, ...)		new Type[ num_elements ]
		#define DESTROY_ARRAY(Type, pointer, num_elements) if ( pointer ) { delete [] pointer; pointer = 0;  }
#endif
	} // namespace memory
} // namespace platform

#include "mem_stl_allocator.h"

#if USE_DEBUG_ALLOCATOR
	#define CustomPlatformAllocator platform::memory::DebugAllocator
#else
	#define CustomPlatformAllocator std::allocator
#endif