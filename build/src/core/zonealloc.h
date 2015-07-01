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

#include <platform/typedefs.h>

//#define CORE_MEMORY_TRACKING_ENABLED 1

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
		// Zones (aka arenas, heaps, whatever) are a means of categorizing and
		// tagging allocations.
		
		// GOALS:
		// I. Allow arbitrary zones to be created
		// II. Zones should be able to report statistics about usage
		// III. Zones can optionally take a maximum budget size. 0 == no budget
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
			
		public:
			Zone(const char* zone_name, size_t max_budget_bytes = 0);
			~Zone();
			
			/// The name of this zone
			const char* name() const { return zone_name; }
			
			/// Track an allocation in this zone.
			/// @param size The size of the allocation in bytes
			/// @returns A value of 0 on success. Non-zero indicates failure.
			int add_allocation(size_t size);
			
			/// Untrack an allocation in this zone
			/// @param size The size of the allocation in bytes
			void remove_allocation(size_t size);
			
			/// Print out a general report of stats for this zone
			void report();
		}; // class Zone
		
		
		
		// ---------------------------------------------------------------------
		// allocator
		// ---------------------------------------------------------------------
		template <class Type>
		struct Allocator
		{
			Zone* zone;
			Allocator(Zone* memory_zone)
			{
				zone = memory_zone;
			}
		}; // struct Allocator

	} // namespace memory
} // namespace core