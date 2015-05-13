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

namespace platform
{
	namespace memory
	{
		class IAllocator
		{
		public:
			virtual ~IAllocator() {}
			
			virtual void * allocate( size_t bytes, const char * file, int line ) = 0;
			virtual void deallocate( void * memory ) = 0;
			virtual void print_report() = 0;
			
			virtual void track_allocation(void* memory, size_t bytes, const char* file, int line) = 0;
			virtual void untrack_allocation(void* memory) = 0;

			virtual size_t active_bytes() const = 0;
			virtual size_t active_allocations() const = 0;
			virtual size_t total_allocations() const = 0;
			virtual size_t total_bytes() const = 0;
		}; // IAllocator
		
		// initialize memory handling
		void startup();
		
		// shutdown services and optionally perform any metrics, leak detection, etc
		void shutdown();
		
		// instance of the active allocator
		IAllocator & allocator();
		
		void set_allocator(IAllocator* allocator);

		// helper function for creating arrays from a contiguous block of memory
		// and then calling placement new on each one
		template <class Type>
		Type* create_array(size_t num_elements, const char* file, int line)
		{
			Type* block = (Type*)memory::allocator().allocate(sizeof(Type)*num_elements, file, line);
			for (size_t i = 0; i < num_elements; ++i)
			{
				new (&block[i]) Type;
			}

			return block;
		}

#if USE_DEBUG_ALLOCATOR
		// raw memory alloc/dealloc
		#define ALLOC(byte_count)	platform::memory::allocator().allocate(byte_count, __FILE__, __LINE__)
		#define DEALLOC(pointer) { platform::memory::allocator().deallocate(pointer); pointer = 0; }
		
		// helper macros for alloc and dealloc on classes and structures
		#define CREATE(Type, ...)	new (platform::memory::allocator().allocate(sizeof(Type), __FILE__, __LINE__)) Type(__VA_ARGS__)
		#define DESTROY(Type, pointer) { if (pointer) { pointer->~Type(); platform::memory::allocator().deallocate(pointer); pointer = 0; } }
		
		// NOTES:
		// 1. This only works if the Type has a default constructor
		// 2. In order to work around the ridiculous "standard" of placement new in 5.3.4.12,
		//	  This allocates a contiguous block of memory and then calls placement new on each element.
		#define CREATE_ARRAY(Type, num_elements) platform::memory::create_array<Type>(num_elements, __FILE__, __LINE__)
		#define DESTROY_ARRAY(Type, pointer, num_elements) if ( pointer ) { for( size_t i = 0; i < num_elements; ++i ) { (&pointer[i])->~Type(); } platform::memory::allocator().deallocate(pointer); pointer = 0;  }
#else
		#define ALLOC(byte_count)	malloc(byte_count)
		#define DEALLOC(pointer) { free(pointer); pointer = 0; }

		#define CREATE(Type, ...)	new Type(__VA_ARGS__)
		#define DESTROY(Type, pointer) { delete pointer; pointer = 0; }

		#define CREATE_ARRAY(Type, num_elements, ...)		new Type[ num_elements ]
		#define DESTROY_ARRAY(Type, pointer, num_elements) if ( pointer ) { delete [] pointer; pointer = 0;  }
#endif

		// GOALS: MEMORY REFACTOR
		// * create/destroy arrays of non-POD types
		// * create/destroy non-POD types
		// * create aligned allocs (for physics)
		// * tag or group allocations by category
		// * allocation tracking
		// * be able to pass arguments to the constructor
		// * be able to use allocator across dylib bounds (for data structures)
		// * be able to inject malloc/free style functions into third-party libraries
		
//		size_t test_align(size_t bytes, uint32_t alignment)
//		{
//			return (bytes + (alignment-1)) & ~(alignment-1);
//		}
		
//		void* aligned_allocate(size_t size, size_t alignment)
//		{
//			void* out;
//			int result = posix_memalign(&out, alignment, size);
////			assert(result == 0);
//			fprintf(stdout, "aligned_allocate [size=%zu, alignment=%zu]\n", size, alignment);
//			return out;
//		}
//		
//		void aligned_deallocate(void* pointer)
//		{
//			fprintf(stdout, "aligned_deallocate [pointer=%p]\n", pointer);
//			free(pointer);
//		}

		struct MemTagGlobal {};
		
		
		
#if PLATFORM_MEMORY_TRACKING_ENABLED
		template <class T>
		struct MemoryCategoryTracking
		{
			typedef MemoryCategoryTracking<T> this_type;
			static size_t total_bytes_used;
			
			static void add_allocation(size_t size)
			{
				this_type::total_bytes_used += size;
			}
			
			static void remove_allocation(size_t size)
			{
				this_type::total_bytes_used -= size;
			}
		};
		
		template <class T>
		size_t MemoryCategoryTracking<T>::total_bytes_used = 0;
#endif
		
		template <class A, class T>
		struct Allocator
		{
#if PLATFORM_MEMORY_TRACKING_ENABLED
			void tag_allocation(size_t size)
			{
				MemoryCategoryTracking<T>::add_allocation(size);
			}
			
			void untag_allocation(size_t size)
			{
				MemoryCategoryTracking<T>::remove_allocation(size);
			}
#endif
			
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
		struct GlobalDebugAllocator : public Allocator< GlobalDebugAllocator<T>, T >
		{
			void* allocate(size_t size, const char* filename, int line)
			{
				void* pointer = malloc(size);
				fprintf(stdout, "allocate: %p, %zu bytes, %s:%i\n", pointer, size, filename, line);
#if PLATFORM_MEMORY_TRACKING_ENABLED
				this->tag_allocation(size);
#endif
				
				return pointer;
			}
			
			void deallocate(void* pointer)
			{
#if PLATFORM_MEMORY_TRACKING_ENABLED
				this->untag_allocation(0);
#endif
				fprintf(stdout, "deallocate: %p\n", pointer);
				free(pointer);
			}
		}; // GlobalDebugAllocator



		typedef GlobalDebugAllocator<MemTagGlobal> GlobalAllocator;


		template <class _Type, class _Allocator>
		void deallocate_pointer(_Type* pointer, _Allocator& allocator)
		{
			pointer->~_Type();
			allocator.deallocate(pointer);
		}
		
		template <class _Type, class _Allocator>
		_Type* allocate_array(size_t elements, _Allocator& allocator, const char* filename, int line)
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
		void deallocate_array(_Type* pointer, _Allocator& allocator)
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
		
		
		GlobalAllocator& global_allocator();

		#define MEMORY_NEW(type, allocator) new (allocator.allocate(sizeof(type), __FILE__, __LINE__)) type
		#define MEMORY_DELETE(pointer, allocator) ::platform::memory::deallocate_pointer(pointer, allocator)
				
		#define MEMORY_NEW_ARRAY(type, elements, allocator) ::platform::memory::allocate_array< type >(elements, allocator, __FILE__, __LINE__)
		#define MEMORY_DELETE_ARRAY(pointer, allocator) ::platform::memory::deallocate_array(pointer, allocator)
	} // namespace memory
} // namespace platform

#include "mem_stl_allocator.h"

#if USE_DEBUG_ALLOCATOR
	#define CustomPlatformAllocator platform::memory::DebugAllocator
#else
	#define CustomPlatformAllocator std::allocator
#endif