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

// used by the debug tracking policy
#include <list>
#include <assert.h>

//#define CORE_MEMORY_TRACKING_ENABLED 1


#define MEMORY_ALLOC(size, allocator) (allocator).allocate(size, sizeof(void*), __FILE__, __LINE__)
#define MEMORY_DEALLOC(pointer, allocator) (allocator).deallocate(pointer)

#define MEMORY_NEW(type, allocator) new ((allocator).allocate(sizeof(type), sizeof(void*), __FILE__, __LINE__)) type
#define MEMORY_DELETE(pointer, allocator) ::core::memory::deallocate_pointer(pointer, (allocator)), pointer = 0

#define MEMORY_NEW_ARRAY(type, elements, allocator) ::core::memory::construct_array< type >(elements, allocator, __FILE__, __LINE__)
#define MEMORY_DELETE_ARRAY(pointer, allocator) ::core::memory::destruct_array(pointer, allocator), pointer = 0


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

		// ---------------------------------------------------------------------
		// zone
		// ---------------------------------------------------------------------
		// zones (aka arenas, heaps, whatever) are a means of categorizing and
		// tagging allocations.

		// GOALS:
		// I. Allow arbitrary zones to be created
		// II. zones should be able to report statistics about usage
		// III. zones can optionally take a maximum budget size. 0 == no budget
		class Zone
		{
		private:
			// lifetime values
			size_t total_allocations;
			size_t total_bytes;

			// active values
			size_t active_allocations;
			size_t active_bytes;

			size_t high_watermark;
			size_t smallest_allocation;
			size_t largest_allocation;

			// budgeted memory size
			size_t budget_bytes;

			// zone name
			const char* zone_name;

			/// Print out a general report of stats for this zone
			void report();

		public:
			LIBRARY_EXPORT Zone(const char* zone_name, size_t max_budget_bytes = 0);
			LIBRARY_EXPORT ~Zone();

			/// The name of this zone
			LIBRARY_EXPORT const char* name() const { return zone_name; }

			/// Track an allocation in this zone.
			/// @param size The size of the allocation in bytes
			/// @returns A value of 0 on success. Non-zero indicates failure.
			LIBRARY_EXPORT int add_allocation(size_t size);

			/// Untrack an allocation in this zone
			/// @param size The size of the allocation in bytes
			LIBRARY_EXPORT void remove_allocation(size_t size);


			// public accessors
			LIBRARY_EXPORT size_t get_total_allocations() const { return total_allocations; }
			LIBRARY_EXPORT size_t get_total_bytes() const { return total_bytes; }
			LIBRARY_EXPORT size_t get_active_allocations() const { return active_allocations; }
			LIBRARY_EXPORT size_t get_active_bytes() const { return active_bytes; }
			LIBRARY_EXPORT size_t get_high_watermark() const { return high_watermark; }
			LIBRARY_EXPORT size_t get_smallest_allocation() const { return smallest_allocation; }
			LIBRARY_EXPORT size_t get_largest_allocation() const { return largest_allocation; }
			LIBRARY_EXPORT size_t get_budget_bytes() const { return budget_bytes; }
		}; // class Zone


		// ---------------------------------------------------------------------
		// utility functions
		// ---------------------------------------------------------------------
		LIBRARY_EXPORT void* aligned_malloc(size_t bytes, size_t alignment);
		LIBRARY_EXPORT void aligned_free(void* pointer);


		// ---------------------------------------------------------------------
		// tracking policies
		// ---------------------------------------------------------------------
		#include "memory/simple_tracking_policy.h"
		#include "memory/debug_tracking_policy.h"

		typedef SimpleTrackingPolicy DefaultTrackingPolicy;

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
			if (pointer)
			{
				pointer->~_Type();
				allocator.deallocate(pointer);
			}
		}

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
	} // namespace memory
} // namespace core
