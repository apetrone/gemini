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
// IMPLIED WARRANTIES OF MERCHANATABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//		 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------
#include <core/logging.h>
#include <core/memory/zone.h>
#include <core/memory/debug_tracking_policy.h>

using namespace core::memory;

DebugTrackingPolicy::DebugTrackingPolicy()
{
}

DebugTrackingPolicy::~DebugTrackingPolicy()
{
	print_leaks();
}

size_t DebugTrackingPolicy::request_size(size_t requested_size, size_t /*alignment*/)
{
	return requested_size + sizeof(MemoryHeader);
}

void* DebugTrackingPolicy::track_allocation(Zone* zone, void* pointer, size_t requested_size, size_t alignment, const char* filename, int line)
{
	MemoryHeader* header = reinterpret_cast<MemoryHeader*>(pointer);
	header->allocation_size = requested_size;
	header->allocation_index = zone ? zone->next_allocation_id() : 0;
	header->alignment = alignment;
	header->filename = filename;
	header->line = line;
	allocated_blocks.push_back(header);

	LOGV("[+] '%s' %x size=%lu, align=%lu, line=%i, alloc_num=%zu, file='%s'\n",
		zone ? zone->name() : "",
		(header+1),
		requested_size,
		alignment,
		line,
		header->allocation_index,
		filename);
	return (header+1);
}

void* DebugTrackingPolicy::untrack_allocation(Zone* zone, void* pointer, size_t& allocation_size)
{
	MemoryHeader* header = static_cast<MemoryHeader*>(pointer);
	header--;
	assert(header);

	allocation_size = header->allocation_size;

	LOGV("[-] '%s' %x size=%lu, align=%lu, line=%i, alloc_num=%zu\n",
		zone ? zone->name() : "",
		pointer,
		header->allocation_size,
		header->alignment,
		header->line,
		header->allocation_index);

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

void DebugTrackingPolicy::print_leaks()
{
	MemoryBlockList::iterator it = allocated_blocks.begin();
	for (; it != allocated_blocks.end(); ++it)
	{
		MemoryHeader* block = (*it);
		LOGV("*** MEMORY LEAK [addr=%x] [file=%s] [line=%i] [size=%lu] [alloc_num=%lu]\n",
				(((char*)block)+sizeof(MemoryHeader)),
				block->filename,
				block->line,
				(unsigned long)block->allocation_size,
				(unsigned long)block->allocation_index);
	}
}
