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

namespace core
{
	namespace memory
	{
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
	} // namespace memory
} // namespace core