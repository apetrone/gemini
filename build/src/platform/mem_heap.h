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

#include "mem.h"

namespace platform
{
	namespace memory
	{
		
		
		
		// - Heap: A chunk of memory
		// - Allocator: specifies a strategy for allocating from a heap
		
		
		struct SimpleAllocator
		{
			//	size_t max_size;
			//	void* memory;
			//
			//	SimpleAllocator() :
			//		max_size(0),
			//		memory(0)
			//	{
			//	}
			
			void set_memory(void* memory, size_t memory_size)
			{
				//		this->memory = memory;
				//		this->max_size = memory_size;
			}
			
			void* allocate(size_t bytes)
			{
				return malloc(bytes);
			}
			
			void deallocate(void* pointer)
			{
				free(pointer);
			}
		};
		
		template <class _Allocator>
		struct TrackingHeap
		{
			// This does not track
			
			typedef _Allocator allocator_type;
			
			allocator_type allocator;
			core::StackString<16> name;
			
			// currently active values
			size_t active_allocations;
			size_t active_footprint;
			
			// lifetime values
			size_t total_allocations;
			size_t total_footprint;
			
			size_t high_watermark;
			
			size_t smallest_allocation_size;
			size_t largest_allocation_size;
			
			// this is NOT an accurate value at the moment because
			// I use std::vector with the default allocator.
			// That is used to store tracking data, but I don't track that usage.
			size_t estimated_overhead;
			
			struct AllocationTrackingData
			{
				size_t allocation_index;
				size_t size;
				void* address; // only for debugging!!
				uint32_t line;
				const char* file;
			};
			
			std::vector<AllocationTrackingData> tracking_data;
			std::vector<size_t> freelist;
			
			TrackingHeap(const char* label) :
			name(label),
			active_allocations(0),
			active_footprint(0),
			total_allocations(0),
			total_footprint(0),
			high_watermark(0),
			smallest_allocation_size(SIZE_T_MAX),
			largest_allocation_size(0),
			estimated_overhead(0)
			{
			}
			
			void set_memory(void* memory, size_t memory_size)
			{
				allocator.set_memory(memory, memory_size);
			}
			
			size_t track_allocation(void* pointer, size_t size, const char* file, int line)
			{
				AllocationTrackingData* data;
				
				if (!freelist.empty())
				{
					size_t allocation_index = freelist.back();
					freelist.pop_back();
					
					data = &tracking_data[allocation_index];
				}
				else
				{
					AllocationTrackingData new_data;
					new_data.allocation_index = tracking_data.size();
					tracking_data.push_back(new_data);
					data = &tracking_data[new_data.allocation_index];
				}
				
				data->address = pointer;
				data->size = size;
				data->file = file;
				data->line = line;
				
				++active_allocations;
				active_footprint += data->size;
				++total_allocations;
				total_footprint += data->size;
				
				estimated_overhead += sizeof(size_t)+sizeof(AllocationTrackingData);
				
				if (size > largest_allocation_size)
					largest_allocation_size = size;
				
				if (size < smallest_allocation_size)
					smallest_allocation_size = size;
				
				if (active_footprint > high_watermark)
					high_watermark = active_footprint;
				
				fprintf(stdout, "allocate %zu [%p, size=%zu, file='%s', line=%i]\n", data->allocation_index, pointer, size, file, line);
				
				return data->allocation_index;
			}
			
			void untrack_allocation(size_t allocation_index)
			{
				const AllocationTrackingData& data = tracking_data[allocation_index];
				
				--active_allocations;
				active_footprint -= data.size;
				
				//		estimated_overhead -= sizeof(size_t);
				
				fprintf(stdout, "deallocate %zu [pointer=%p]\n", allocation_index, data.address);
				
				freelist.push_back(allocation_index);
			}
			
			
			AllocationTrackingData* find_allocation(void* pointer)
			{
				for (AllocationTrackingData& data : tracking_data)
				{
					if (data.address == pointer)
						return &data;
				}
				return nullptr;
			}
			
			void* allocate(size_t size, const char* filename, int line)
			{
				size_t tracking_size = size+sizeof(size_t);
				void* mem = allocator.allocate(tracking_size);
				size_t* allocation_index = reinterpret_cast<size_t*>(mem);
				char* block = static_cast<char*>(((char*)mem)+sizeof(size_t));
				
				*allocation_index = track_allocation(block, size+sizeof(size_t), filename, line);
				
				return block;
			}
			
			void deallocate(void* pointer)
			{
				AllocationTrackingData* data = find_allocation(pointer);
				if (data)
				{
					char* block = ((char*)pointer)-sizeof(size_t);
					size_t* allocation_index = reinterpret_cast<size_t*>(block);
					untrack_allocation(*allocation_index);
					
					pointer = block;
				}
				
				allocator.deallocate(pointer);
			}
			
			void report()
			{
				fprintf(stdout, "[memory-heap] %s, aa: %zu | af: %zu | ta: %zu | tf: %zu | hw: %zu | s: %zu | l: %zu | ov: %zu\n",
						name(),
						active_allocations,
						active_footprint,
						total_allocations,
						total_footprint,
						high_watermark,
						smallest_allocation_size,
						largest_allocation_size,
						estimated_overhead);
			}
		}; // TrackingHeap

	} // namespace memory
} // namespace platform