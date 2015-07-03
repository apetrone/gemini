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

// The debug tracker keeps detailed records regarding allocations.
// This can be used to help track down memory leaks.
struct debug_tracking_policy
{
#pragma pack(push, 4)
	struct MemoryHeader
	{
		size_t allocation_size;
		size_t allocation_index;
		size_t alignment;
		const char* filename;
		int line;
	};
#pragma pack(pop)
	
	typedef std::list<MemoryHeader*> MemoryBlockList;
	MemoryBlockList allocated_blocks;
	
	~debug_tracking_policy()
	{
		print_leaks();
	}
	
	/// Adjust the size of the allocation for this policy.
	/// @returns the final size in bytes which should be allocated
	size_t request_size(size_t requested_size, size_t /*alignment*/)
	{
		return requested_size + sizeof(MemoryHeader);
	}
	
	
	/// Track an allocation
	/// @param pointer The pointer allocated
	/// @param requested_size The size in bytes of this allocation
	/// @param alignment The alignment of this allocation
	/// @param filename A null terminated filename where this was allocated
	/// @param line The line number of this allocation
	void* track_allocation(void* pointer, size_t requested_size, size_t alignment, const char* filename, int line)
	{
		MemoryHeader* header = reinterpret_cast<MemoryHeader*>(pointer);
		header->allocation_size = requested_size;
		header->allocation_index = 0;
		header->alignment = alignment;
		header->filename = filename;
		header->line = line;
		allocated_blocks.push_back(header);
		return (header+1);
	}
	
	/// Deallocates are VERY slow because we hunt the entire block list
	/// to remove. this could probably use a free list instead.
	/// @param allocation_size out param of this allocation's size in bytes
	/// @returns The pointer passed into track_allocation (should match)
	void* untrack_allocation(void* pointer, size_t& allocation_size)
	{
		MemoryHeader* header = static_cast<MemoryHeader*>(pointer);
		header--;
		assert(header);
		
		allocation_size = header->allocation_size;
		
		// remove from allocated block list
		MemoryBlockList::iterator it = allocated_blocks.begin();
		for (; it != allocated_blocks.end(); ++it)
		{
			if (*it == header)
			{
				allocated_blocks.erase(it);
				break;
			}
		}
		
		return header;
	}
private:
	void print_leaks()
	{
		MemoryBlockList::iterator it = allocated_blocks.begin();
		for (; it != allocated_blocks.end(); ++it)
		{
			MemoryHeader* block = (*it);
			fprintf(stdout, "*** MEMORY LEAK [addr=%p] [file=%s] [line=%i] [size=%lu] [alloc_num=%lu]\n",
					(((char*)block)+sizeof(MemoryHeader)),
					block->filename,
					block->line,
					(unsigned long)block->allocation_size,
					(unsigned long)block->allocation_index);
		}
	}
}; // debug_tracking_policy
