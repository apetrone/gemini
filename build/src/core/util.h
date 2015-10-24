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

#include "typedefs.h"
#include "stackstring.h"
#include <string>

namespace core
{
	template <unsigned int C, class T>
	struct StackString;

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
			MEMORY_DELETE(p, core::memory::global_allocator());
		}
	}; // DestroyPointer

	namespace util
	{
		LIBRARY_EXPORT unsigned int hash_32bit(const void * data, int data_size, unsigned int seed);

		// return a float within the range: min, max, inclusive
		LIBRARY_EXPORT float random_range(float min, float max);

		template <class T>
		struct hash
		{
			uint32_t operator()(const T& value)
			{
				return hash_32bit(&value, sizeof(T), 0);
			}
		};

		template <>
		struct hash<std::string>
		{
			uint32_t operator()(const std::string& s)
			{
				return hash_32bit(&s[0], s.length(), 0);
			}
		};

		template <size_t C>
		struct hash<StackString<C, char>>
		{
			uint32_t operator()(const StackString<C, char>& s)
			{
				return hash_32bit(&s[0], s.size(), 0);
			}
		};
	} // namespace util


	template <class Iterator>
	void swap(const Iterator& a, Iterator& b)
	{
		b.swap(a);
	}

	// Quick Sort
	// base case: O(n log n)
	// worst case: O(n^2)
	// in-place operation
	struct quicksort
	{
		template <class Iterator>
		void operator()(Iterator start, Iterator end)
		{
			sort(start, end);
		}

		template <class Iterator>
		void sort(const Iterator& start, const Iterator& end)
		{
			if (start < end)
			{
				Iterator pivot = partition(start, end);

				// sort left block
				sort(start, pivot);

				// sort right block
				sort(pivot+1, end);
			}
		}

		template <class Iterator>
		Iterator partition(const Iterator& start, const Iterator& end)
		{
			// partition the list based on a pivot (last element in this case)
			// move all elements <= pivot to the left of the pivot
			// move all elements > pivot to the right
			Iterator pivot = (end-1);
			Iterator iter_swap = start;

			// swap elements
			for (Iterator current = start; current != pivot; ++current)
			{
				if (*current <= *pivot)
				{
					swap(current, iter_swap);
					++iter_swap;
				}
			}

			swap(iter_swap, pivot);

			return iter_swap;
		}
	};

	template <typename Algorithm, class Iterator>
	void sort(Iterator start, Iterator end)
	{
		Algorithm()(start, end);
	}

} // namespace core
