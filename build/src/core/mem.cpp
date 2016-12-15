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
#include "mem.h"

#include <core/logging.h>

#ifndef memset
	#include <string.h>
#endif

namespace gemini
{
	ZoneStats* _tracking_stats = nullptr;

	ZoneStats* memory_zone_tracking_stats()
	{
		return _tracking_stats;
	} // memory_zone_tracking_stats

	void* memory_force_alignment(void* mem, uint32_t alignment)
	{
		unsigned char* block = static_cast<unsigned char*>(mem);

		// this assumes alignment is a power of two.
		const uint32_t one_minus_alignment = (alignment - 1);

		return reinterpret_cast<void*>(((size_t)(block + one_minus_alignment)) & ~one_minus_alignment);
	} // memory_force_alignment

	void memory_zone_install_stats(ZoneStats* other)
	{
		_tracking_stats = other;
	} // memory_zone_install_stats

	void memory_zone_track(MemoryZone zone, size_t bytes)
	{
		ZoneStats& stats = _tracking_stats[zone];

		stats.active_allocations++;
		stats.active_bytes += bytes;

		stats.total_allocations++;
		stats.total_bytes += bytes;
	} // memory_zone_track

	void memory_zone_untrack(MemoryZone zone, size_t bytes)
	{
		ZoneStats& stats = _tracking_stats[zone];
		stats.active_allocations--;
		stats.active_bytes -= bytes;
	} // memory_zone_untrack

	// ---------------------------------------------------------------------
	// common allocation functions
	// ---------------------------------------------------------------------
#if defined(DEBUG_MEMORY)
	void* memory_allocate(Allocator* allocator, size_t bytes, size_t alignment, const char* filename, int line)
	{
		//const size_t required_size = (bytes + sizeof(MemoryDebugHeader));

		void* memory = allocator->allocate(allocator, bytes, alignment);
		//LOGV("[+] %p %i @ %i | '%s':%i\n", memory, bytes, alignment, filename, line);
		return memory;
	}

	void memory_deallocate(Allocator* allocator, void* pointer, const char* filename, int line)
	{
		allocator->deallocate(allocator, pointer);
		//LOGV("[-] %p | '%s':%i\n", pointer, filename, line);
	}
#else
	void* memory_allocate(Allocator* allocator, size_t bytes, size_t alignment)
	{
		return allocator->allocate(allocator, bytes, alignment);
	}

	void memory_deallocate(Allocator* allocator, void* pointer)
	{
		allocator->deallocate(allocator, pointer);
	}
#endif





	// ---------------------------------------------------------------------
	// default allocator: Standard operating system-level heap allocator
	// ---------------------------------------------------------------------
	void* default_allocate(Allocator*, size_t bytes, size_t alignment)
	{
		return core::memory::aligned_malloc(bytes, alignment);
	}

	void default_deallocate(Allocator*, void* pointer)
	{
		core::memory::aligned_free(pointer);
	}

	Allocator memory_allocator_default()
	{
		Allocator allocator;
		memset(&allocator, 0, sizeof(Allocator));
		allocator.allocate = default_allocate;
		allocator.deallocate = default_deallocate;
		allocator.type = ALLOCATOR_SYSTEM;
		return allocator;
	} // memory_allocator_default

	// ---------------------------------------------------------------------
	// linear allocator: Basic linear allocator
	// ---------------------------------------------------------------------
	void* linear_allocate(Allocator* allocator, size_t bytes, size_t /*alignment*/)
	{
		if (allocator->bytes_used + bytes <= allocator->memory_size)
		{
			unsigned char* block = reinterpret_cast<unsigned char*>(allocator->memory) + allocator->bytes_used;
			allocator->bytes_used += bytes;
			return block;
		}

		return nullptr;
	} // linear_allocate

	void linear_deallocate(Allocator* /*allocator*/, void* /*pointer*/)
	{
		// no-op
	}

	Allocator memory_allocator_linear(void* memory, size_t memory_size)
	{
		Allocator allocator;
		memset(&allocator, 0, sizeof(Allocator));
		allocator.allocate = linear_allocate;
		allocator.deallocate = linear_deallocate;
		allocator.memory = memory;
		allocator.memory_size = memory_size;
		allocator.type = ALLOCATOR_LINEAR;
		return allocator;
	} // memory_allocator_linear

} // namespace gemini

namespace core
{
	namespace memory
	{
		// ---------------------------------------------------------------------
		// interface
		// ---------------------------------------------------------------------
		Zone* _global_zone = nullptr;
		GlobalAllocatorType* _global_allocator = nullptr;

		gemini::StaticMemory<Zone> global_zone_memory;

		void startup()
		{
			// If you hit this assert, there's a double memory startup
			assert(_global_zone == nullptr && _global_allocator == nullptr);
			_global_zone = new (global_zone_memory.memory) Zone("global");
			static GlobalAllocatorType global_allocator_instance(_global_zone);
			_global_allocator = &global_allocator_instance;

			// create zone tracking stats
			using namespace gemini;
			gemini::Allocator allocator = gemini::memory_allocator_default();
			ZoneStats* zone_stats = MEMORY2_NEW_ARRAY(&allocator, MEMORY_ZONE_DEFAULT, ZoneStats, MEMORY_ZONE_MAX);
		}

		void shutdown()
		{
			// If you hit this assert, there's a double memory shutdown
			assert(_global_zone && _global_allocator);

			_global_zone->~Zone();

			// sanity test; these are no longer valid!
			_global_zone = nullptr;
			_global_allocator = nullptr;
		}

		SystemAllocatorType& system_allocator()
		{
			static SystemAllocatorType _system_allocator;
			return _system_allocator;
		}

		GlobalAllocatorType& global_allocator()
		{
			return *_global_allocator;
		}

		void global_allocator(GlobalAllocatorType& allocator)
		{
			_global_allocator = &allocator;
		}

		// ---------------------------------------------------------------------
		// zone
		// ---------------------------------------------------------------------
		Zone::Zone(const char* zone_name, size_t max_budget_bytes)
		{
			memset(this, 0, sizeof(Zone));
			this->zone_name = zone_name;
			budget_bytes = max_budget_bytes;
			current_allocation = 0;
		}

		Zone::~Zone()
		{
			report();
		}

		int Zone::add_allocation(size_t size)
		{
			if (budget_bytes > 0 && (active_bytes + size) > budget_bytes)
			{
				// over budget!
				return -1;
			}

			total_allocations++;
			total_bytes += size;

			active_allocations++;
			active_bytes += size;

			if (size > largest_allocation)
				largest_allocation = size;

			if (size < smallest_allocation || (smallest_allocation == 0))
				smallest_allocation = size;

			if (active_bytes > high_watermark)
				high_watermark = active_bytes;

			return 0;
		}

		void Zone::remove_allocation(size_t size)
		{
			// Attempted to free something that wasn't allocated.
			assert(active_allocations > 0 && active_bytes >= size);

			active_allocations--;
			active_bytes -= size;
		}

		void Zone::report()
		{
			// could use %zu on C99, but fallback to %lu and casts for C89.
			LOGV("[zone: '%s'] total_allocations = %lu, total_bytes = %lu, budget_bytes = %lu\n", zone_name,
					(unsigned long)total_allocations,
					(unsigned long)total_bytes,
					(unsigned long)budget_bytes);

			LOGV("[zone: '%s'] active_allocations = %lu, active_bytes = %lu, high_watermark = %lu\n", zone_name,
					(unsigned long)active_allocations,
					(unsigned long)active_bytes,
					(unsigned long)high_watermark);

			LOGV("[zone: '%s'] smallest_allocation = %lu, largest_allocation = %lu\n", zone_name,
					(unsigned long)smallest_allocation,
					(unsigned long)largest_allocation);

			// if you hit this, there may be a memory leak!
			assert(active_allocations == 0 && active_bytes == 0);
		}

		void* aligned_malloc(size_t bytes, size_t alignment)
		{
#if defined(PLATFORM_POSIX)
			void* mem;
			if (0 == posix_memalign(&mem, alignment, bytes))
			{
				return mem;
			}

			return nullptr;

#elif defined(PLATFORM_WINDOWS)
			return _aligned_malloc(bytes, alignment);
#else
	#error aligned_malloc not defined for this platform!
#endif
		}

		void aligned_free(void* pointer)
		{
#if defined(PLATFORM_POSIX)
			free(pointer);
#elif defined(PLATFORM_WINDOWS)
			_aligned_free(pointer);
#else
	#error aligned_free not defined for this platform!
#endif
		}
	} // namespace memory
} // namespace core
