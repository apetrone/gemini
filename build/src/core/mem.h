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

#include "typedefs.h"
#include "config.h"

#if defined(PLATFORM_APPLE)
	#include <memory> // for malloc, free (on OSX)
	#include <stdlib.h>
#elif defined(PLATFORM_LINUX) || defined(PLATFORM_ANDROID)
	#include <stdlib.h>
	#include <new> // for placement new
#elif defined(PLATFORM_WINDOWS)
	#include <memory> // we'll see if this compiles...
#else
	#error Unknown platform!
#endif

// uncomment this to debug memory allocations (performance hit!)
//#define ENABLE_MEMORY_TRACKING 1

#include <assert.h>


#if defined(ENABLE_MEMORY_TRACKING)
#include <list>
#endif

#define MEMORY_ALLOC(size, allocator) (allocator).allocate(size, sizeof(void*), __FILE__, __LINE__)
#define MEMORY_DEALLOC(pointer, allocator) (allocator).deallocate(pointer)

#define MEMORY_NEW(type, allocator) new ((allocator).allocate(sizeof(type), sizeof(void*), __FILE__, __LINE__)) type
#define MEMORY_DELETE(pointer, allocator) ::core::memory::deallocate_pointer(pointer, (allocator)), pointer = 0

#define MEMORY_NEW_ARRAY(type, elements, allocator) ::core::memory::construct_array< type >(elements, allocator, __FILE__, __LINE__)
#define MEMORY_DELETE_ARRAY(pointer, allocator) ::core::memory::destruct_array(pointer, allocator), pointer = 0


#include "core/memory/zone.h"

// ---------------------------------------------------------------------
// tracking policies
// ---------------------------------------------------------------------
#include "memory/simple_tracking_policy.h"

#if defined(ENABLE_MEMORY_TRACKING)
#include "memory/debug_tracking_policy.h"
namespace core
{
	namespace memory
	{
		typedef DebugTrackingPolicy DefaultTrackingPolicy;
	} // namespace memory
} // namespace core
#else
namespace core
{
	namespace memory
	{
		typedef SimpleTrackingPolicy DefaultTrackingPolicy;
	} // namespace memory
} // namespace core
#endif

namespace core
{
	namespace memory
	{
		// ---------------------------------------------------------------------
		// constants
		// ---------------------------------------------------------------------
		const size_t Kilobyte = 1024;
		const size_t Megabyte = (Kilobyte*1024);
		const size_t Gigabyte = (Megabyte*1024);


		template <class T, size_t count = 1>
		struct static_memory
		{
			unsigned char memory[sizeof(T) * count];
		};

		// ---------------------------------------------------------------------
		// utility functions
		// ---------------------------------------------------------------------
		LIBRARY_EXPORT void* aligned_malloc(size_t bytes, size_t alignment);
		LIBRARY_EXPORT void aligned_free(void* pointer);

		// ---------------------------------------------------------------------
		// allocator
		// ---------------------------------------------------------------------
		// Allocators provide an interface between code and memory. Different
		// allocation strategies are provided by each new allocator type.

		// GOALS:
		// I. Allocators should work in tandem with the zones
		// II. When practical, Allocators should accept an upper limit for size.
		//     This allows greater control over the memory usage.
		// III. Allocators should accept a few different policies for control and tuning.
		//      Policies for tracking, or guarding memory can be specified.
		template <class Type>
		struct Allocator
		{
			Zone* memory_zone;
			Allocator(Zone* target_zone)
			{
				memory_zone = target_zone;
			}

			Zone* get_zone() const { return memory_zone; }
		}; // struct allocator





		// allocators
		#include "memory/system_allocator.h"
		#include "memory/heap_allocator.h"
//		#include "memory/linear_allocator.h"
//		#include "memory/pool_allocator.h"
//		#include "memory/stack_allocator.h"
//		#include "memory/page_allocator.h"

		// ---------------------------------------------------------------------
		// interface
		// ---------------------------------------------------------------------
		LIBRARY_EXPORT void startup();
		LIBRARY_EXPORT void shutdown();


		typedef SystemAllocator<DefaultTrackingPolicy> SystemAllocatorType;
		LIBRARY_EXPORT SystemAllocatorType& system_allocator();

		typedef HeapAllocator<DefaultTrackingPolicy> GlobalAllocatorType;
		LIBRARY_EXPORT GlobalAllocatorType& global_allocator();
		LIBRARY_EXPORT void global_allocator(GlobalAllocatorType& allocator);

		// ---------------------------------------------------------------------
		// template functions
		// ---------------------------------------------------------------------
		template <class _Type, class _Allocator>
		void deallocate_pointer(_Type* pointer, _Allocator& allocator)
		{
			// it is entirely legal to call delete on a null pointer,
			// but we don't need to do anything.
			if (pointer)
			{
				pointer->~_Type();
				allocator.deallocate(pointer);
			}
		}

#if defined(PLATFORM_COMPILER_MSVC)
	// msvc doesn't think using a variable in a term with a constructor
	// or cast is 'using' the variable, so it throws up this warning.
	// Let's disable it for construct_array/destruct_array.
	#pragma warning(push)
	#pragma warning(disable: 4189) // 'p': local variable is initialized but not referenced
#endif

		template <class _Type, class _Allocator>
		_Type* construct_array(size_t elements, _Allocator& allocator, const char* filename, int line)
		{
			// store number of elements
			// and for Non-POD types; the size of the _Type.

			size_t total_size = sizeof(_Type)*elements + (sizeof(size_t)+sizeof(size_t));

			void* mem = allocator.allocate(total_size, sizeof(void*), filename, line);
			size_t* block = reinterpret_cast<size_t*>(mem);
			*block = elements;

			block++;
			*block = sizeof(_Type);

			block++;

			_Type* values = reinterpret_cast<_Type*>(block);

			for (size_t index = 0; index < elements; ++index)
			{
				new (&values[index]) _Type;
			}

			return values;
		}

		template <class _Type, class _Allocator>
		void destruct_array(_Type* pointer, _Allocator& allocator)
		{
			if (pointer)
			{
				// fetch the number of elements
				size_t* block = reinterpret_cast<size_t*>(pointer);
				block--;

				size_t type_size = *block;

				// If you hit this assert; there is a double-delete on this pointer!
				assert(type_size > 0);
				*block = 0;

				block--;
				size_t total_elements = *block;

				char* mem = reinterpret_cast<char*>(pointer);

				// per the spec; we must delete the elements in reverse order
				for (size_t index = total_elements; index > 0; --index)
				{
					// for non-POD types, we have to make sure we offset
					// into the array by the correct offset of the allocated type.
					size_t offset = (index-1)*type_size;
					_Type* p = reinterpret_cast<_Type*>(mem+offset);
					p->~_Type();
				}

				// deallocate the block
				allocator.deallocate(block);
			}
		}

#if defined(PLATFORM_COMPILER_MSVC)
	#pragma warning(pop)
#endif
	} // namespace memory
} // namespace core
