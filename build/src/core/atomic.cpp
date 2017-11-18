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
#include <core/atomic.h>

// TODO: Break these up into separate atomic files?
// At the moment, there isn't a clear architecture for that since
// there are OS AND compiler specifics to be organized.

namespace gemini
{
#if defined(PLATFORM_APPLE)
	bool atom_compare_and_swap32(volatile uint32_t* destination, uint32_t new_value, uint32_t comparand)
	{
		return OSAtomicCompareAndSwap32Barrier(
			static_cast<int32_t>(comparand),
			static_cast<int32_t>(new_value),
			reinterpret_cast<volatile int32_t*>(destination));
	}

	uint32_t atom_increment32(volatile uint32_t* destination)
	{
		return static_cast<uint32_t>(OSAtomicIncrement32(reinterpret_cast<volatile int32_t*>(destination)));
	}

	uint64_t atom_increment64(volatile uint64_t* destination, uint64_t value)
	{
		#error Implement on this platform.
		return 0;
	}
#elif defined(PLATFORM_WINDOWS)
	bool atom_compare_and_swap32(volatile uint32_t* destination, uint32_t new_value, uint32_t comparand)
	{
		unsigned int initial_destination = InterlockedCompareExchange(
			reinterpret_cast<volatile unsigned int*>(destination),
			static_cast<unsigned int>(new_value),
			static_cast<unsigned int>(comparand));
		return (initial_destination == comparand);
	}

	uint32_t atom_increment32(volatile uint32_t* destination)
	{
		return InterlockedIncrement((volatile unsigned int*)destination);
	}

	uint64_t atom_increment64(volatile uint64_t* destination, uint64_t value)
	{
		return static_cast<uint64_t>(InterlockedAdd64(reinterpret_cast<volatile LONG64*>(destination), static_cast<LONG64>(value)));
	}
#elif defined(PLATFORM_LINUX) && (defined(__clang__) || defined(__GNUC__))
	bool atom_compare_and_swap32(volatile uint32_t* destination, uint32_t new_value, uint32_t comparand)
	{
		return __sync_bool_compare_and_swap(destination, comparand, new_value);
	}

	uint32_t atom_increment32(volatile uint32_t* destination)
	{
		return __sync_add_and_fetch(destination, 1);
	}

	uint64_t atom_increment64(volatile uint64_t* destination, uint64_t value)
	{
		return __sync_add_and_fetch(destination, 1);
	}
#else
	#error No atomic synchronization functions defined for this platform.
#endif
} // namespace gemini