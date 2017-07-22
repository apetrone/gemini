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
#include <core/str.h>

#ifndef memset
	#include <string.h>
#endif

namespace gemini
{
	ZoneStats* _tracking_stats = nullptr;
	gemini::StaticMemory<gemini::ZoneStats, gemini::MEMORY_ZONE_MAX> zone_stat_memory;

	ZoneStats* memory_zone_tracking_stats()
	{
		return _tracking_stats;
	} // memory_zone_tracking_stats

	size_t memory_per_allocation_overhead()
	{
#if defined(ENABLE_MEMORY_TRACKING)
		return sizeof(MemoryDebugHeader) + sizeof(MemoryZoneHeader);
#else
		return sizeof(MemoryZoneHeader);
#endif
	} // memory_per_allocation_overhead

	void memory_leak_report(bool assert_on_active_allocations)
	{
		// Now we try to iterate over zone stats.
		ZoneStats* stats = memory_zone_tracking_stats();
#if defined(ENABLE_MEMORY_TRACKING)
		size_t leaked_allocations = 0;
#endif

		for (size_t index = 0; index < MEMORY_ZONE_MAX; ++index)
		{
			const ZoneStats& zone_stat = stats[index];
#if defined(ENABLE_MEMORY_TRACKING)
			if (zone_stat.tail != nullptr)
#else
			if (zone_stat.active_bytes > 0)
#endif
			{
				const char* zone_name = memory_zone_name(static_cast<MemoryZone>(index));
				LOGV("[memory] Zone Report for '%s' (%i)...\n", zone_name, index);
				LOGV("\t* total_allocations = %lu, total_bytes = %lu\n",
					(unsigned long)zone_stat.total_allocations,
					(unsigned long)zone_stat.total_bytes);

				// could use %zu on C99, but fallback to %lu and casts for C89.
				LOGV("\t* active allocations = %lu, active_bytes = %lu, high_watermark = %lu\n",
					(unsigned long)zone_stat.active_allocations,
					(unsigned long)zone_stat.active_bytes,
					(unsigned long)zone_stat.high_watermark
				);

				LOGV("\t* smallest_allocation = %lu, largest_allocation = %lu\n",
					(unsigned long)zone_stat.smallest_allocation,
					(unsigned long)zone_stat.largest_allocation);

				//LOGV("*** MEMORY LEAK [addr=%x] [file=%s] [line=%i] [size=%lu] [alloc_num=%lu]\n",
				//	(((char*)block) + sizeof(MemoryHeader)),
				//	block->filename,
				//	block->line,
				//	(unsigned long)block->allocation_size,
				//	(unsigned long)block->allocation_index);

#if defined(ENABLE_MEMORY_TRACKING)
				if (assert_on_active_allocations)
				{

					MemoryDebugHeader* debug = zone_stat.tail;
					while (debug)
					{
						unsigned char* block = reinterpret_cast<unsigned char*>(debug);
						MemoryZoneHeader* zone_header = reinterpret_cast<MemoryZoneHeader*>(block + sizeof(MemoryDebugHeader));
						LOGV("\t%s(%i): MEMORY LEAK allocation_id = %i, requested_size = %i, actual_size = %i\n",
							debug->filename,
							debug->line,
							debug->allocation_index,
							zone_header->requested_size,
							zone_header->allocation_size);
						debug = debug->next;
						++leaked_allocations;
					}
				}
#endif
			}
		}

#if defined(ENABLE_MEMORY_TRACKING)
		if (leaked_allocations)
		{
			LOGV("\t %i leaked allocations!\n", leaked_allocations);

			// if you hit this, there may be a memory leak!
			assert(leaked_allocations == 0);
		}
#endif


	} // memory_leak_report


	void* memory_force_alignment(void* mem, uint32_t alignment)
	{
		unsigned char* block = static_cast<unsigned char*>(mem);

		// this assumes alignment is a power of two.
		const size_t one_minus_alignment = (alignment - 1);

		return reinterpret_cast<void*>(((size_t)(block + one_minus_alignment)) & ~one_minus_alignment);
	} // memory_force_alignment

	bool memory_is_aligned(void* mem, uint32_t alignment)
	{
		return (reinterpret_cast<uintptr_t>(mem) & (alignment - 1)) == 0;
	} // memory_is_aligned

	void memory_zone_install_stats(ZoneStats* other)
	{
		_tracking_stats = other;
	} // memory_zone_install_stats

	void memory_zone_track(MemoryZone zone, size_t allocation_size)
	{
		assert(_tracking_stats);
		ZoneStats& stats = _tracking_stats[zone];

		stats.active_allocations++;
		stats.active_bytes += allocation_size;

		stats.total_allocations++;
		stats.total_bytes += allocation_size;

		if (allocation_size > stats.largest_allocation)
		{
			stats.largest_allocation = allocation_size;
		}

		if (allocation_size < stats.smallest_allocation || (stats.smallest_allocation == 0))
		{
			stats.smallest_allocation = allocation_size;
		}

		if (stats.active_bytes > stats.high_watermark)
		{
			stats.high_watermark = stats.active_bytes;
		}
	} // memory_zone_track

	void memory_zone_untrack(MemoryZone zone, size_t allocation_size)
	{
		assert(_tracking_stats);
		ZoneStats& stats = _tracking_stats[zone];

		stats.active_allocations--;
		stats.active_bytes -= allocation_size;
	} // memory_zone_untrack

	const char* memory_zone_name(MemoryZone zone)
	{
		switch (zone)
		{
			case MEMORY_ZONE_DEFAULT:		return "default";
			case MEMORY_ZONE_PLATFORM:		return "platform";
			case MEMORY_ZONE_AUDIO:			return "audio";
			case MEMORY_ZONE_RENDERER:		return "renderer";
			case MEMORY_ZONE_GUI:			return "gui";
			case MEMORY_ZONE_ASSETS:		return "assets";
			case MEMORY_ZONE_DEBUGDRAW:		return "debugdraw";
			case MEMORY_ZONE_NAVIGATION:	return "navigation";
			case MEMORY_ZONE_RUNTIME:		return "runtime";
			case MEMORY_ZONE_FILESYSTEM:	return "filesystem";
			case MEMORY_ZONE_PHYSICS:		return "physics";
			case MEMORY_ZONE_GAME:			return "game";
			case MEMORY_ZONE_MAX:
			default:						return "unknown";
		}
	} // memory_zone_name

	MemoryZoneHeader* memory_zone_header_from_pointer(void* pointer)
	{
		unsigned char* memory = reinterpret_cast<unsigned char*>(pointer);
		memory -= sizeof(MemoryZoneHeader);
		return reinterpret_cast<MemoryZoneHeader*>(memory);
	} // memory_zone_header_from_pointer

	// ---------------------------------------------------------------------
	// common allocation functions
	// ---------------------------------------------------------------------
#if defined(ENABLE_MEMORY_TRACKING)
	void* memory_allocate(MemoryZone zone, size_t requested_size, uint32_t alignment, const char* filename, int line)
	{
		// Add header sizes onto the requested size.
		size_t allocation_size = requested_size + memory_per_allocation_overhead();

		// compute alignment offset if the allocation_size would spill
		// across boundaries.
		uintptr_t pointer_size = static_cast<uintptr_t>(allocation_size);
		pointer_size = reinterpret_cast<uintptr_t>(memory_force_alignment(reinterpret_cast<void*>(pointer_size), alignment));
		uintptr_t alignment_offset = pointer_size - allocation_size;
		allocation_size += alignment_offset;

		// request the memory from the OS
		void* memory = memory_aligned_malloc(allocation_size, alignment);

		ZoneStats* stats = memory_zone_tracking_stats();
		ZoneStats& target_stat = stats[zone];

		unsigned char* block = reinterpret_cast<unsigned char*>(memory);
		MemoryDebugHeader* debug = reinterpret_cast<MemoryDebugHeader*>(memory);
		memset(debug, 0, sizeof(MemoryDebugHeader));
		debug->alignment = alignment;
		debug->allocation_index = target_stat.total_allocations;
		debug->filename = filename;
		debug->line = line;
		debug->next = nullptr;
		debug->prev = nullptr;

		// insert the debug header into the list
		if (target_stat.tail != nullptr)
		{
			debug->next = target_stat.tail;
			target_stat.tail->prev = debug;
			target_stat.tail = debug;
		}
		else
		{
			target_stat.tail = debug;
		}

		// populate debug header.
		memory = block + sizeof(MemoryDebugHeader) + alignment_offset;

		MemoryZoneHeader* zone_header = reinterpret_cast<MemoryZoneHeader*>(memory);
		zone_header->zone = zone;
		zone_header->requested_size = static_cast<uint32_t>(requested_size);
		zone_header->allocation_size = static_cast<uint32_t>(allocation_size);
		zone_header->alignment_offset = static_cast<uint32_t>(alignment_offset);
		memory_zone_track(zone, allocation_size);

		// populate zone header
		memory = block + sizeof(MemoryDebugHeader) + alignment_offset + sizeof(MemoryZoneHeader);

		//LOGV("[+] '%s' %x size=%lu, align=%lu, line=%i, alloc_num=%zu, file='%s'\n",
		//	zone ? zone->name() : "",
		//	(header + 1),
		//	requested_size,
		//	alignment,
		//	line,
		//	header->allocation_index,
		//	filename);

		return memory;
	} // memory_allocate


	void memory_deallocate(void* pointer, const char* /*filename*/, int /*line*/)
	{
		if (!pointer)
		{
			// Nothing to do.
			return;
		}
		unsigned char* memory = reinterpret_cast<unsigned char*>(pointer);

		MemoryZoneHeader* zone_header = memory_zone_header_from_pointer(pointer);
		memory_zone_untrack(zone_header->zone, zone_header->allocation_size);

		memory = reinterpret_cast<unsigned char*>(zone_header);
		memory -= zone_header->alignment_offset;

		memory -= sizeof(MemoryDebugHeader);
		MemoryDebugHeader* debug = reinterpret_cast<MemoryDebugHeader*>(memory);

		ZoneStats* stats = memory_zone_tracking_stats();
		ZoneStats& target_stat = stats[zone_header->zone];

		// remove debug header from the linked list.
		MemoryDebugHeader* next = debug->next;
		if (debug->prev)
		{
			debug->prev->next = next;
		}
		if (next)
		{
			next->prev = debug->prev;
		}

		if (target_stat.tail == debug)
		{
			target_stat.tail = next;
		}

		//LOGV("[-] '%s' %x size=%lu, align=%lu, line=%i, alloc_num=%zu\n",
		//	zone ? zone->name() : "",
		//	pointer,
		//	header->allocation_size,
		//	header->alignment,
		//	header->line,
		//	header->allocation_index);

		memory_aligned_free(memory);
	} // memory_deallocate
#else
	void* memory_allocate(MemoryZone zone, size_t requested_size, size_t alignment)
	{
		// Add header sizes onto the requested size.
		size_t allocation_size = requested_size + memory_per_allocation_overhead();

		// compute alignment offset if the allocation_size would spill
		// across boundaries.
		void* pointer_size = reinterpret_cast<void*>(allocation_size);
		pointer_size = memory_force_alignment(pointer_size, alignment);
		uint32_t alignment_offset = static_cast<uint32_t>((size_t)pointer_size - (size_t)allocation_size);
		allocation_size += alignment_offset;

		// request the memory from the OS
		unsigned char* block = reinterpret_cast<unsigned char*>(memory_aligned_malloc(allocation_size, alignment));

		block += alignment_offset;
		MemoryZoneHeader* zone_header = reinterpret_cast<MemoryZoneHeader*>(block);
		zone_header->zone = zone;
		zone_header->requested_size = static_cast<uint32_t>(requested_size);
		zone_header->allocation_size = static_cast<uint32_t>(allocation_size);
		zone_header->alignment_offset = alignment_offset;

		block += sizeof(MemoryZoneHeader);

		memory_zone_track(zone, allocation_size);

		return block;
	} // memory_allocate

	void memory_deallocate(void* pointer)
	{
		MemoryZoneHeader* zone_header = memory_zone_header_from_pointer(pointer);
		memory_zone_untrack(zone_header->zone, zone_header->allocation_size);

		unsigned char* block = reinterpret_cast<unsigned char*>(zone_header);
		block -= zone_header->alignment_offset;

		memory_aligned_free(block);
	} // memory_deallocate
#endif





	// ---------------------------------------------------------------------
	// default allocator: Standard operating system-level heap allocator
	// ---------------------------------------------------------------------
#if defined(ENABLE_MEMORY_TRACKING)
	void* default_allocate(Allocator& allocator, size_t requested_size, uint32_t alignment, const char* filename, int line)
	{
		return memory_allocate(allocator.zone, requested_size, alignment, filename, line);
	}

	void default_deallocate(Allocator& /*allocator*/, void* pointer, const char* filename, int line)
	{
		memory_deallocate(pointer, filename, line);
	}
#else
	void* default_allocate(Allocator& /*allocator*/, size_t bytes, uint32_t alignment)
	{
		return memory_aligned_malloc(bytes, alignment);
	}

	void default_deallocate(Allocator& /*allocator*/, void* pointer)
	{
		memory_aligned_free(pointer);
	}
#endif

	Allocator memory_allocator_default(MemoryZone zone)
	{
		Allocator allocator;
		memset(&allocator, 0, sizeof(Allocator));
		allocator.zone = zone;
		allocator.allocate = default_allocate;
		allocator.deallocate = default_deallocate;
		allocator.type = ALLOCATOR_SYSTEM;
		return allocator;
	} // memory_allocator_default

	// ---------------------------------------------------------------------
	// linear allocator: Basic linear allocator
	// ---------------------------------------------------------------------
	void* linear_allocate_common(Allocator& allocator, MemoryZone zone, size_t requested_size, uint32_t alignment)
	{
		// We purposely don't track memory leaks for the linear allocators.
		// That memory is the responsibility of the allocator's creator.
		size_t allocation_size = requested_size + sizeof(MemoryZoneHeader);

		void* pointer_size = reinterpret_cast<void*>(allocation_size);
		pointer_size = memory_force_alignment(pointer_size, alignment);
		uint32_t alignment_offset = static_cast<uint32_t>((size_t)pointer_size - (size_t)allocation_size);
		allocation_size += alignment_offset;

		if (allocator.bytes_used + allocation_size <= allocator.memory_size)
		{
			unsigned char* block = reinterpret_cast<unsigned char*>(allocator.memory) + allocator.bytes_used;

			block += alignment_offset;
			MemoryZoneHeader* zone_header = reinterpret_cast<MemoryZoneHeader*>(block);
			zone_header->zone = zone;
			zone_header->requested_size = static_cast<uint32_t>(requested_size);
			zone_header->allocation_size = static_cast<uint32_t>(allocation_size);
			zone_header->alignment_offset = alignment_offset;

			block += sizeof(MemoryZoneHeader);

			allocator.bytes_used += allocation_size;
			memory_zone_track(zone, allocation_size);
			return block;
		}

		return nullptr;
	} // linear_allocate_common

	void linear_deallocate_common(void* pointer)
	{
		MemoryZoneHeader* zone_header = memory_zone_header_from_pointer(pointer);
		memory_zone_untrack(zone_header->zone, zone_header->allocation_size);
	} // linear_deallocate_common

#if defined(ENABLE_MEMORY_TRACKING)
	void* linear_allocate(Allocator& allocator, size_t requested_size, uint32_t alignment, const char* /*filename*/, int /*line*/)
	{
		return linear_allocate_common(allocator, allocator.zone, requested_size, alignment);
	} // linear_allocate

	void linear_deallocate(Allocator& /*allocator*/, void* pointer, const char* /*filename*/, int /*line*/)
	{
		linear_deallocate_common(pointer);
	} // linear_deallocate

#else
	void* linear_allocate(Allocator& allocator, size_t requested_size, uint32_t alignment)
	{
		return linear_allocate_common(allocator, allocator.zone, requested_size, alignment);
	} // linear_allocate

	void linear_deallocate(Allocator& /*allocator*/, void* pointer)
	{
		linear_deallocate_common(pointer);
	} // linear_deallocate
#endif

	Allocator memory_allocator_linear(MemoryZone zone, void* memory, size_t memory_size)
	{
		Allocator allocator;
		memset(&allocator, 0, sizeof(Allocator));
		allocator.zone = zone;
		allocator.allocate = linear_allocate;
		allocator.deallocate = linear_deallocate;
		allocator.memory = memory;
		allocator.memory_size = memory_size;
		allocator.type = ALLOCATOR_LINEAR;
		return allocator;
	} // memory_allocator_linear


	// ---------------------------------------------------------------------
	// interface
	// ---------------------------------------------------------------------

	void memory_startup()
	{
		static_assert(sizeof(gemini::MemoryZoneHeader) == 16, "MemoryZoneHeader padding is incorrect.");

		// If you hit this assert, there's a double memory startup
		assert(gemini::_tracking_stats == nullptr);

		// create zone tracking stats
		gemini::ZoneStats* zone_stats = memory_static_allocate(zone_stat_memory);
		memset(zone_stats, 0, zone_stat_memory.size);
		gemini::memory_zone_install_stats(zone_stats);
	}

	void memory_shutdown()
	{
		gemini::memory_leak_report();

		// If you hit this assert, there's a double memory shutdown
		assert(gemini::_tracking_stats);
		gemini::_tracking_stats = nullptr;
	}

	void* memory_aligned_malloc(size_t bytes, size_t alignment)
	{
#if defined(PLATFORM_POSIX)
		void* mem = nullptr;

		// alignment must be a multiple of sizeof(void*)
		if (alignment < sizeof(void*))
		{
			alignment = sizeof(void*);
		}
		assert(alignment / sizeof(void*) > 0);

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
	} // memory_aligned_malloc

	void memory_aligned_free(void* pointer)
	{
#if defined(PLATFORM_POSIX)
		free(pointer);
#elif defined(PLATFORM_WINDOWS)
		_aligned_free(pointer);
#else
#error aligned_free not defined for this platform!
#endif
	} // memory_aligned_free
} // namespace gemini
