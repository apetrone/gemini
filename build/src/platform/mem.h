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

#include "config.h"

#include <string.h> // for size_t
#include <assert.h>
#include <list>

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

// set this to 0 to enable normal new/delete and malloc/free to narrow down problems
#define USE_DEBUG_ALLOCATOR 1

#define PLATFORM_MEMORY_TRACKING_ENABLED 1

namespace platform
{
	namespace memory
	{
		struct MemTagGlobal {};
		

		template <class T>
		struct MemoryCategoryTracking
		{
			typedef MemoryCategoryTracking<T> this_type;

			// lifetime values
			static size_t total_allocations;
			static size_t total_bytes;
			
			// active values
			static size_t active_allocations;
			static size_t active_bytes;
			
			static size_t high_watermark;
			static size_t smallest_allocation;
			static size_t largest_allocation;
						
			static void add_allocation(size_t size)
			{
				this_type::total_allocations++;
				this_type::total_bytes += size;
				
				this_type::active_allocations++;
				this_type::active_bytes += size;
				
				if (size > this_type::largest_allocation)
					this_type::largest_allocation = size;
					
				if (size < this_type::smallest_allocation || (this_type::smallest_allocation == 0))
					this_type::smallest_allocation = size;
					
				if (this_type::active_bytes > this_type::high_watermark)
					this_type::high_watermark = this_type::active_bytes;
			}
			
			static void remove_allocation(size_t size)
			{
				this_type::total_bytes -= size;
				
				this_type::active_allocations--;
				this_type::active_bytes -= size;
			}
			
			// print out a general report
			static void report(const char* name)
			{
				// could use %zu on C99, but fallback to %lu and casts for C89.
				fprintf(stdout, "[memory-tracking] '%s' total_allocations = %lu, total_bytes = %lu\n", name,
					(unsigned long)this_type::total_allocations,
					(unsigned long)this_type::total_bytes);
					
				fprintf(stdout, "[memory-tracking] '%s' active_allocations = %lu, active_bytes = %lu, high_watermark = %lu\n", name,
					(unsigned long)this_type::active_allocations,
					(unsigned long)this_type::active_bytes,
					(unsigned long)this_type::high_watermark);
					
				fprintf(stdout, "[memory-tracking] '%s' smallest_allocation = %lu, largest_allocation = %lu\n", name,
					(unsigned long)this_type::smallest_allocation,
					(unsigned long)this_type::largest_allocation);
					
				// if you hit this, there may be a memory leak!
				assert(this_type::active_allocations == 0 && this_type::active_bytes == 0);
			}
		};
		
		template <class T>
		size_t MemoryCategoryTracking<T>::total_allocations = 0;
		
		template <class T>
		size_t MemoryCategoryTracking<T>::total_bytes = 0;
		
		template <class T>
		size_t MemoryCategoryTracking<T>::active_allocations = 0;
		
		template <class T>
		size_t MemoryCategoryTracking<T>::active_bytes = 0;
		
		template <class T>
		size_t MemoryCategoryTracking<T>::high_watermark = 0;
		
		template <class T>
		size_t MemoryCategoryTracking<T>::smallest_allocation = 0;
		
		template <class T>
		size_t MemoryCategoryTracking<T>::largest_allocation = 0;

		
		template <class A, class T>
		struct Allocator
		{

			void tag_allocation(size_t size)
			{
				MemoryCategoryTracking<T>::add_allocation(size);
			}
			
			void untag_allocation(size_t size)
			{
				MemoryCategoryTracking<T>::remove_allocation(size);
			}
			
			void* allocate(size_t size, const char* filename, int line)
			{
				assert(0);
				return nullptr;
			}
			
			void deallocate(void* pointer)
			{
				assert(0);
			}
		}; // Allocator
		
		
		template <class T>
		class GlobalDebugAllocator : public Allocator< GlobalDebugAllocator<T>, T >
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

		public:
			void* allocate(size_t requested_size, const char* filename, int line)
			{
				size_t total_size = (requested_size + sizeof(MemoryHeader));
				MemoryHeader* header = (MemoryHeader*)malloc(total_size);
				assert(header);
				
				fprintf(stdout, "allocate: %p, %lu bytes, %s:%i\n", header+1, (unsigned long)requested_size, filename, line);
				
				header->allocation_size = requested_size;
				header->allocation_index = 0;
				header->alignment = sizeof(void*); // default alignment for now
				header->filename = filename;
				header->line = line;
				
				allocated_blocks.push_back(header);

				this->tag_allocation(requested_size);
				
				return (header+1);
			}
			
			// deallocates are VERY slow because we hunt the entire block list
			// to remove.
			void deallocate(void* pointer)
			{
				// it is entirely legal to delete on a null pointer,
				// but we don't need to do anything.
				if (!pointer)
				{
					return;
				}
			
				MemoryHeader* header = static_cast<MemoryHeader*>(pointer);
				header--;
				assert(header);
				
				// remove from allocated block list
				typename MemoryBlockList::iterator it = allocated_blocks.begin();
				for (; it != allocated_blocks.end(); ++it)
				{
					if (*it == header)
					{
						allocated_blocks.erase(it);
						break;
					}
				}

				this->untag_allocation(header->allocation_size);

				fprintf(stdout, "deallocate: %p\n", pointer);
				free(header);
			}
			
			void print_leaks()
			{
				typename MemoryBlockList::iterator it = allocated_blocks.begin();
				for (; it != allocated_blocks.end(); ++it)
				{
					MemoryHeader* block = (*it);
					fprintf(stdout, "[memory-leak] [addr=%p] [file=%s] [line=%i] [size=%lu] [alloc_num=%lu]\n",
							(((char*)block)+sizeof(MemoryHeader)),
							block->filename,
							block->line,
							(unsigned long)block->allocation_size,
							(unsigned long)block->allocation_index);
				}
			}
		}; // GlobalDebugAllocator



		typedef GlobalDebugAllocator<MemTagGlobal> GlobalAllocator;


		template <class _Type, class _Allocator>
		void deallocate_pointer(_Type* pointer, _Allocator& allocator)
		{
			// it's legal to delete a NULL pointer; just don't do anything.
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
			
			void* mem = allocator.allocate(total_size, filename, line);
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
		
		

		// initialize memory handling
		void startup();
		
		// shutdown services and optionally perform any metrics, leak detection, etc
		void shutdown();
		
		GlobalAllocator& global_allocator();
		void set_allocator(GlobalAllocator* allocator);

		#define MEMORY_ALLOC(size, allocator) allocator.allocate(size, __FILE__, __LINE__)
		#define MEMORY_DEALLOC(pointer, allocator) allocator.deallocate(pointer)

		#define MEMORY_NEW(type, allocator) new (allocator.allocate(sizeof(type), __FILE__, __LINE__)) type
		#define MEMORY_DELETE(pointer, allocator) ::platform::memory::deallocate_pointer(pointer, allocator), pointer = 0
				
		#define MEMORY_NEW_ARRAY(type, elements, allocator) ::platform::memory::construct_array< type >(elements, allocator, __FILE__, __LINE__)
		#define MEMORY_DELETE_ARRAY(pointer, allocator) ::platform::memory::destruct_array(pointer, allocator), pointer = 0
	} // namespace memory
} // namespace platform

#include "mem_stl_allocator.h"

#if USE_DEBUG_ALLOCATOR
	#define CustomPlatformAllocator platform::memory::DebugAllocator
#else
	#define CustomPlatformAllocator std::allocator
#endif