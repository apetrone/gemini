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
#include "array.h"
#include <string>

#if defined(PLATFORM_COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable: 4265) // 'std::_Func_base<_Ret,const char *>': class has virtual functions, but destructor is not virtual
#endif

#include <functional>

#if defined(PLATFORM_COMPILER_MSVC)
#pragma warning(pop)
#endif

namespace core
{
	template <size_t C, class T>
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
		unsigned int hash_32bit(const void* data, size_t data_size, unsigned int seed);

		// return a float within the range: min, max, inclusive
		float random_range(float min, float max);

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

namespace gemini
{
	// ---------------------------------------------------------------------
	// event handling / delegate
	// ---------------------------------------------------------------------
	template <class T>
	class DelegateHandler
	{
		// inspirations:
		// boost::signals
		// Don Clugston's Fast Delegate
		// function and bind

		// Requirements:
		// + must handle arbitrary arguments or struct type
		// + need to bind a member function pointer or free (static) function
		// - should be able to support delayed invocation
		//	 This is possible, but not implemented at this time.

		// std::function has deleted operator==; therefore
		// we cannot implement a disconnect function as the backing
		// store of delegate<T> is an std::function.
	public:

		// connect overloads
		template <class C>
		void connect(void (C::*function_ptr)(T), C* instance)
		{
			Delegate func;
			func.set(function_ptr, instance);
			connections.push_back(func);
		}

		template <class C>
		void connect(void (C::*function_ptr)(T) const, C* instance)
		{
			Delegate func;
			func.set(function_ptr, instance);
			connections.push_back(func);
		}

		void connect(void (*function_ptr)(T))
		{
			Delegate func;
			func.set(function_ptr);
			connections.push_back(func);
		}

		// emit event to all connections
		void operator()(T value) const
		{
			for (size_t index = 0; index < connections.size(); ++index)
			{
				const Delegate& connection = connections[index];
				connection.invoke(value);
			}
		}

		// remove all connections from this event
		void clear_connections()
		{
			connections.clear();
		}

	private:
		class Delegate
		{
		public:
			template <class C>
			void set(void (C::*function_ptr)(T), C* instance)
			{
				function_pointer = std::bind(function_ptr, instance, std::placeholders::_1);
			}

			template <class C>
			void set(void (C::*function_ptr)(T) const, C* instance)
			{
				function_pointer = std::bind(function_ptr, instance, std::placeholders::_1);
			}

			void set(void (*function_ptr)(T))
			{
				function_pointer = std::bind(function_ptr, std::placeholders::_1);
			}

			void invoke(T value) const
			{
				assert(function_pointer != nullptr);
				function_pointer(value);
			}

		private:
			std::function<void (T)> function_pointer;
		};

		void remove_if_found(const Delegate& func)
		{
			connections.erase(func);
		}

		Array< Delegate > connections;
	};
} // namespace gemini
