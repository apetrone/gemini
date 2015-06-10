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

#include <platform/typedefs.h>
#include <core/stackstring.h>

namespace core
{
	template <class Type>
	struct RangedValue
	{
		Type min;
		Type max;
		
		void set_range( const Type & minimum, const Type & maximum )
		{
			this->min = minimum;
			this->max = maximum;
		}
	}; // RangedValue

	// Useful to hand to a std::for_each statement in order to clean up a container.
	template <class _Type>
	struct DestroyPointer
	{
		void operator()(_Type * p)
		{
			MEMORY_DELETE(p, platform::memory::global_allocator());
		}
	}; // DestroyPointer

	namespace util
	{
		LIBRARY_EXPORT unsigned int hash_32bit(const void * data, int data_size, unsigned int seed);

		// return a float within the range: min, max, inclusive
		LIBRARY_EXPORT float random_range(float min, float max);
		
		
		template <class T>
		uint32_t hash32(const T& data);
		
		struct hash
		{
			template <class T>
			uint32_t operator()(const T& value)
			{
				return hash32(value);
			}
		};
	} // namespace util
} // namespace core