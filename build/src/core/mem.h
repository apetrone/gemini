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

// TODO: replace this with ENABLE_MEMORY_TRACKING when refactor is done.
#define DEBUG_MEMORY 1

namespace gemini
{
	// Anything that allocates memory should accept an allocator.
	// Anything that deletes memory should accept an allocator.
	// Any container should be able to accept an allocator.

	// * Debug memory tracking should be a compile flag.
	// * Leak detection that aids in resolution.
	// * Different allocation strategies can be specified with the same interface.
	// * All memory should be tracked via zones. This is helpful to monitor
	//	 memory statistics for an application.
	// * Only OS allocations should be debug tracked and reported as leaks.

	enum AllocatorType
	{
		// Standard system allocator; malloc/free from the operating system.
		ALLOCATOR_SYSTEM,

		ALLOCATOR_LINEAR,

		ALLOCATOR_TYPE_MAX
	}; // AllocatorType

	enum MemoryZone
	{
		// All static memory
		MEMORY_ZONE_DEFAULT,

		// Memory allocated by the platform layer
		MEMORY_ZONE_PLATFORM,

		// Memory allocated by the audio subsystem
		MEMORY_ZONE_AUDIO,

		// Memory allocated by the renderer
		MEMORY_ZONE_RENDERER,

		// gui subsystem
		MEMORY_ZONE_GUI,

		MEMORY_ZONE_ASSETS,

		MEMORY_ZONE_MAX
	}; // MemoryZone

	struct Allocator
	{
#if defined(DEBUG_MEMORY)
		void* (*allocate)(Allocator& allocator, size_t requested_size, uint32_t alignment, const char* filename, int line);
		void (*deallocate)(Allocator& allocator, void* pointer, const char* filename, int line);
#else
		void* (*allocate)(Allocator& allocator, size_t requested_size, uint32_t alignment);
		void (*deallocate)(Allocator& allocator, void* pointer);
#endif
		// internal allocator type
		AllocatorType type;

		// zone from which any allocations from this allocator will be tagged
		MemoryZone zone;

		size_t bytes_used;
		size_t memory_size;
		void* memory;
	}; // Allocator


#if defined(DEBUG_MEMORY)
#pragma pack(push, 8)
	struct MemoryDebugHeader
	{
		uint32_t alignment;
		size_t allocation_index;
		const char* filename;
		int line;

		// in-place doubly-linked list
		struct MemoryDebugHeader* next;
		struct MemoryDebugHeader* prev;
	}; // MemoryDebugHeader
#pragma pack(pop)
#endif


	struct PLATFORM_ALIGN(16) MemoryZoneHeader
	{
		// target zone to which this allocation belongs
		MemoryZone zone;

		// total allocation size in bytes
		// (includes zone header and optional debug header)
		uint32_t allocation_size;

		// requested allocation size in bytes
		uint32_t requested_size;

		// alignment offset in bytes (preceding this zone header)
		uint32_t alignment_offset;
	}; // MemoryZoneHeader

	// The static memory reserved by this class is not supposed to be
	// placed into an allocator.

	// Allocate memory from this with MEMORY2_STATIC_NEW.
	template <class T, size_t count = 1>
	struct StaticMemory
	{
		enum
		{
			size = sizeof(T) * count
		};
		unsigned char memory[size];
	}; // StaticMemory

	// Allocator functions used with StaticMemory
	template <class T>
	T* memory_static_allocate(StaticMemory<T, 1>& mem)
	{
		return new (mem.memory) T;
	} // memory_static_allocate

	template <class T, class ... Types>
	T* memory_static_allocate(StaticMemory<T, 1>& mem, Types && ... tail)
	{
		return new (mem.memory) T(tail ...);
	} // memory_static_allocate

	template <class T, size_t count>
	T* memory_static_allocate(StaticMemory<T, count>& mem)
	{
		return new (mem.memory) T[count];
	} // memory_static_allocate



	struct ZoneStats
	{
		size_t total_allocations;
		size_t total_bytes;

		size_t active_allocations;
		size_t active_bytes;

		size_t high_watermark;
		size_t smallest_allocation;
		size_t largest_allocation;

#if defined(DEBUG_MEMORY)
		// list of active allocations for this zone
		MemoryDebugHeader* tail;
#endif
	}; // ZoneStats

	// fetch current zone stats
	ZoneStats* memory_zone_tracking_stats();

	// Returns the amount of overhead in bytes per allocation.
	size_t memory_per_allocation_overhead();

	void memory_leak_report();

	// Ensures memory is aligned to alignment.
	// Assumes alignment is a power of two.
	void* memory_force_alignment(void* memory, uint32_t alignment);
	bool memory_is_aligned(void* mem, uint32_t alignment);


	// share zone stats
	void memory_zone_install_stats(ZoneStats* other);
	void memory_zone_track(MemoryZone zone, size_t allocation_size);
	void memory_zone_untrack(MemoryZone zone, size_t allocation_size);
	const char* memory_zone_name(MemoryZone zone);
	MemoryZoneHeader* memory_zone_header_from_pointer(void* pointer);

	// Allocator factory functions
	Allocator memory_allocator_default(MemoryZone zone);
	Allocator memory_allocator_linear(MemoryZone zone, void* memory, size_t memory_size);



#if defined(DEBUG_MEMORY)
	#define MEMORY2_ALLOC(allocator, size) (allocator).allocate((allocator), size, alignof(void*), __FILE__, __LINE__)
	#define MEMORY2_DEALLOC(allocator, pointer) (allocator).deallocate((allocator), pointer, __FILE__, __LINE__)

	#define MEMORY2_NEW(allocator, type) new ((allocator).allocate((allocator), sizeof(type), alignof(type), __FILE__, __LINE__)) type
	#define MEMORY2_DELETE(allocator, pointer) memory_destroy((allocator), pointer, __FILE__, __LINE__)

	#define MEMORY2_NEW_ARRAY(allocator, type, size) gemini::memory_array_allocate< type >((allocator), size, __FILE__, __LINE__)
	#define MEMORY2_DELETE_ARRAY(allocator, pointer) gemini::memory_array_deallocate((allocator), pointer, __FILE__, __LINE__), pointer = 0

	void* memory_allocate(MemoryZone zone, size_t requested_size, uint32_t alignment, const char* filename, int line);
	void memory_deallocate(void* pointer, const char* filename, int line);

	template <class T>
	void memory_destroy(Allocator& allocator, T* pointer, const char* filename, int line)
	{
		// it is entirely legal to call delete on a null pointer,
		// but we don't need to do anything.
		if (pointer)
		{
			pointer->~T();
			allocator.deallocate(allocator, pointer, filename, line);
		}
	}
#else

	#define MEMORY2_ALLOC(allocator, size) (allocator).allocate((allocator), size, alignof(void*))
	#define MEMORY2_DEALLOC(allocator, pointer) (allocator).deallocate((allocator), pointer)

	#define MEMORY2_NEW(allocator, type) new ((allocator).allocate((allocator), sizeof(type), alignof(type))) type
	#define MEMORY2_DELETE(allocator, pointer) (allocator).deallocate((allocator), pointer)

	#define MEMORY2_NEW_ARRAY(allocator, type, size) gemini::memory_array_allocate< type >((allocator), size)
	#define MEMORY2_DELETE_ARRAY(allocator, pointer) gemini::memory_array_deallocate((allocator), pointer), pointer = 0

	void* memory_allocate(MemoryZone zone, size_t requested_size, uint32_t alignment);
	void memory_deallocate(void* pointer);

	template <class T>
	void memory_destroy(Allocator& allocator, T* pointer)
	{
		// it is entirely legal to call delete on a null pointer,
		// but we don't need to do anything.
		if (pointer)
		{
			pointer->~T();
			allocator.deallocate(allocator, pointer);
		}
	} // memory_destroy
#endif

#if defined(PLATFORM_COMPILER_MSVC)
	// msvc doesn't think using a variable in a term with a constructor
	// or cast is 'using' the variable, so it throws up this warning.
	// Let's disable it for construct_array/destruct_array.
	#pragma warning(push)
	#pragma warning(disable: 4189) // 'p': local variable is initialized but not referenced
#endif

	template <class _Type>
#if defined(DEBUG_MEMORY)
	_Type* memory_array_allocate(Allocator& allocator, size_t array_size, const char* filename, int line)
#else
	_Type* memory_array_allocate(Allocator& allocator, size_t array_size)
#endif
	{
		// If you hit this assert, someone wanted to allocate zero items of
		// an array. Sad panda.
		assert(array_size > 0);

		// As part of the allocation for arrays store the requested
		// array_size and the size of the _Type.
		size_t total_size = sizeof(_Type) * array_size + (sizeof(size_t) + sizeof(size_t));

#if defined(DEBUG_MEMORY)
		void* mem = allocator.allocate(allocator, total_size, alignof(_Type), filename, line);
		assert(mem != nullptr);
#else
		void* mem = allocator.allocate(allocator, total_size, alignof(_Type));
#endif
		size_t* block = reinterpret_cast<size_t*>(mem);
		*block = array_size;

		block++;
		*block = sizeof(_Type);

		block++;

		_Type* values = reinterpret_cast<_Type*>(block);

		for (size_t index = 0; index < array_size; ++index)
		{
			new (&values[index]) _Type;
		}

		return values;
	} // memory_array_allocate


	template <class _Type>
#if defined(DEBUG_MEMORY)
	void memory_array_deallocate(Allocator& allocator, _Type* pointer, const char* filename, int line)
#else
	void memory_array_deallocate(Allocator& allocator, _Type* pointer)
#endif
	{
		if (pointer)
		{
			// fetch the array_size we embedded during allocation.
			size_t* block = reinterpret_cast<size_t*>(pointer);
			block--;

			size_t type_size = *block;

			// If you hit this assert; there is a double-delete on this pointer!
			assert(type_size > 0);
			*block = 0;

			block--;
			size_t array_size = *block;

			char* mem = reinterpret_cast<char*>(pointer);

			// per the spec; we must delete the elements in reverse order
			for (size_t index = array_size; index > 0; --index)
			{
				// for non-POD types, we have to make sure we offset
				// into the array by the correct offset of the allocated type.
				size_t offset = (index - 1)*type_size;
				_Type* p = reinterpret_cast<_Type*>(mem + offset);
				p->~_Type();
			}

			// deallocate the block
#if defined(DEBUG_MEMORY)
			allocator.deallocate(allocator, block, filename, line);
#else
			allocator.deallocate(allocator, block);
#endif
		}
	} // memory_array_deallocate

#if defined(PLATFORM_COMPILER_MSVC)
	#pragma warning(pop)
#endif

} // namespace gemini

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
		// utility functions
		// ---------------------------------------------------------------------
		void* aligned_malloc(size_t bytes, size_t alignment);
		void aligned_free(void* pointer);

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
		void startup();
		void shutdown();


		typedef SystemAllocator<DefaultTrackingPolicy> SystemAllocatorType;
		SystemAllocatorType& system_allocator();

		typedef HeapAllocator<DefaultTrackingPolicy> GlobalAllocatorType;
		GlobalAllocatorType& global_allocator();
		void global_allocator(GlobalAllocatorType& allocator);

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
