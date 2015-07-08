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

#ifndef memset
	#include <string.h>
#endif

namespace core
{
	namespace memory
	{
		// ---------------------------------------------------------------------
		// interface
		// ---------------------------------------------------------------------
		Zone* _global_zone = nullptr;
		global_allocator_type* _global_allocator = nullptr;
		
		void startup()
		{
			// If you hit this assert, there's a double memory startup
			assert(_global_zone == nullptr && _global_allocator == nullptr);

			static Zone global_memory_zone("global");
			_global_zone = &global_memory_zone;
			
			static global_allocator_type global_allocator_instance(&global_memory_zone);
			_global_allocator = &global_allocator_instance;
		}
		
		void shutdown()
		{
			// If you hit this assert, there's a double memory shutdown
			assert(_global_zone && _global_allocator);

			// sanity test; these are no longer valid!
			_global_zone = nullptr;
			_global_allocator = nullptr;
		}
		
		global_allocator_type& global_allocator()
		{
			return *_global_allocator;
		}
		
		void global_allocator(global_allocator_type& allocator)
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
			fprintf(stdout, "[zone: '%s'] total_allocations = %lu, total_bytes = %lu, budget_bytes = %lu\n", zone_name,
					(unsigned long)total_allocations,
					(unsigned long)total_bytes,
					(unsigned long)budget_bytes);
			
			fprintf(stdout, "[zone: '%s'] active_allocations = %lu, active_bytes = %lu, high_watermark = %lu\n", zone_name,
					(unsigned long)active_allocations,
					(unsigned long)active_bytes,
					(unsigned long)high_watermark);
			
			fprintf(stdout, "[zone: '%s'] smallest_allocation = %lu, largest_allocation = %lu\n", zone_name,
					(unsigned long)smallest_allocation,
					(unsigned long)largest_allocation);
			
			// if you hit this, there may be a memory leak!
			assert(active_allocations == 0 && active_bytes == 0);
		}
	} // namespace memory
} // namespace core
