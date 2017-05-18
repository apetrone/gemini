// -------------------------------------------------------------
// Copyright (C) 2016- Adam Petrone
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
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------
#pragma once

#include <core/typedefs.h>
#include <platform/platform.h>

// memory barriers for different platforms
// There are four types of memory barriers.
// Jeff Preshing has an excellent series of articles
// on his blog regarding these; and in multi-threading
// in general.

// For educational purposes, I want to use the lower level system intrinsics
// and assembly -- until one or more platforms make the C++11 variants
// mandatory.
//#include <atomic>
//#define PLATFORM_WRITE_BARRIER() std::atomic_thread_fence(std::memory_order_release)
//#define PLATFORM_READ_BARRIER() std::atomic_thread_fence(std::memory_order_acquire)

#if defined(PLATFORM_APPLE)
	#include <libkern/OSAtomic.h>
	#define PLATFORM_MEMORY_FENCE() OSMemoryBarrier()
#elif defined(PLATFORM_WINDOWS)
	#include <intrin.h>
	#define PLATFORM_MEMORY_FENCE() _ReadWriteBarrier()
#elif defined(PLATFORM_LINUX) && (defined(__clang__) || defined(__GNUC__))
	#if defined(__arm__)
		// This is an ARM processor.
		#if defined(__ARM_ARCH_7A__) || (defined(__ARM_ARCH) && __ARM_ARCH == 7)
			// Data Memory Barrier: ensures all explicit memory accesses
			// before the instruction complete before any explicit memory
			// accesses after the instruction.
			#define PLATFORM_MEMORY_FENCE() asm volatile("dmb")
		#else
			#error PLATFORM_MEMORY_FENCE is undefined for this ARM architecture.
		#endif
	#elif defined(__x86_64) || defined(__M_X64) || defined(__aarch64__) || defined(_M_IX86) || defined(_X86_) || defined(__i386__) || defined(i386)
		#define PLATFORM_MEMORY_FENCE() asm volatile("" ::: "memory")
	#else
		#error PLATFORM_MEMORY_FENCE is undefined for this architecture.
	#endif
#else
	#error PLATFORM_MEMORY_FENCE is undefined for this platform.
#endif

namespace gemini
{
	/// @brief Perform an atomic compare and swap
	/// If the value in destination is equal to the comparand, destination is set to new_value.
	/// @returns true if the operation succeeded (destination now equals new_value)
	bool atom_compare_and_swap32(volatile uint32_t* destination, uint32_t new_value, uint32_t comparand);

	/// @brief Atomically increments an integer of 32-bit width.
	/// @returns The value of destination post increment.
	uint32_t atom_increment32(volatile uint32_t* destination);

	uint64_t atom_increment64(volatile uint64_t* destination);

	// Use this to wrap atomic variables. Update as needed.
	template <class T>
	struct atomic
	{
		typedef volatile T value_type;
		value_type value;

		atomic(const T& initial_value = T())
			: value(initial_value)
		{
		}

		~atomic()
		{
		}

		atomic& operator=(const atomic<T>& other)
		{
			value = other.value;
			return *this;
		}

		operator value_type() const
		{
			return value;
		}

		const T operator++()
		{
			return ++value;
		}

		const T operator++(int)
		{
			return value++;
		}

		value_type* operator&()
		{
			return &value;
		}
	}; // struct atomic
} // namespace gemini
