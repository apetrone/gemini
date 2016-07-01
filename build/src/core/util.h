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
	template <bool C, class T, class F>
	struct If
	{
		typedef F value;
	};

	template <class T, class F>
	struct If<true, T, F>
	{
		typedef T value;
	};

	template <class T>
	struct remove_reference
	{
		typedef T type;
	};

	template <class T>
	struct remove_reference<T&>
	{
		typedef T type;
	};

	template <class T>
	struct remove_reference<T&&>
	{
		typedef T type;
	};

	template <class T>
	T&& forward(typename remove_reference<T>::type& t)
	{
		return static_cast<T&&>(t);
	}

	template <class T>
	T&& forward(typename remove_reference<T>::type&& t)
	{
		return static_cast<T&&>(t);
	}


	// ---------------------------------------------------------------------
	// event handling / delegate
	// ---------------------------------------------------------------------
	// It is the responsibility of the caller to make sure the delegate is_valid
	// before invoking it.

	// This design is based in part on:
	// http://blog.coldflake.com/posts/C++-delegates-on-steroids/
	// The above was in turn, inspired by this original article:
	// http://www.jeremyong.com/blog/2014/01/10/interfacing-lua-with-templates-in-c-plus-plus-11/
	// After toying with the above, I couldn't add the features I wanted.
	// Namely, variadic types and support for void in return/parameter types.
	// Eventually, I found Stefan Reinalter's post:
	// http://blog.molecular-matters.com/2011/09/19/generic-type-safe-delegates-and-events-in-c/
	// This opened my eyes to a much better way of using the template types
	// with much less code than what I had originally.

	// I have created these macros for occasions where you want less verbose
	// code.
	#define MAKE_STATIC_DELEGATE(T, F) (gemini::Delegate<T>().bind<F>())
	#define MAKE_MEMBER_DELEGATE(T, C, F, P) (gemini::Delegate<T>().bind<C, F>(P))

	// base template
	template <typename T>
	class Delegate {};

	// fully templatized
	template <typename R, typename... P>
	class Delegate<R(P...)>
	{
		void* class_instance;
		typedef R(*function_type)(void*, P...);
		function_type delegate_stub;

	public:
		Delegate()
			: class_instance(nullptr)
			, delegate_stub(nullptr)
		{
		}

		//
		// stub functions
		//
		template <R(*function)(P...)>
		static inline R static_stub(void*, P... parameters)
		{
			return function(parameters...);
		}

		template <class C, R(C::*function)(P...)>
		static inline R member_stub(void* instance, P... parameters)
		{
			C* cl = reinterpret_cast<C*>(instance);
			return (cl->*function)(parameters...);
		}

		//
		// bind functions
		//
		template <R(*function)(P...)>
		Delegate& bind()
		{
			class_instance = nullptr;
			delegate_stub = &static_stub<function>;
			return *this;
		}

		template <class C, R(C::*function)(P...)>
		Delegate& bind(C* instance)
		{
			class_instance = instance;
			delegate_stub = &member_stub<C, function>;
			return *this;
		}

		R operator()(P... parameters)
		{
			return delegate_stub(class_instance, parameters...);
		}

		bool is_valid() const
		{
			return (delegate_stub != nullptr);
		}
	};

	// templatized return value, no parameters
	template <typename R>
	class Delegate<R()>
	{
		void* class_instance;
		typedef R(*function_type)(void*);
		function_type delegate_stub;

	public:
		Delegate()
			: class_instance(nullptr)
			, delegate_stub(nullptr)
		{
		}

		//
		// stub functions
		//
		template <R(*function)()>
		static inline R static_stub(void*)
		{
			return function();
		}

		template <class C, R(C::*function)()>
		static inline R member_stub(void* instance)
		{
			C* cl = reinterpret_cast<C*>(instance);
			return (cl->*function)();
		}

		//
		// bind functions
		//
		template <R(*function)()>
		Delegate& bind()
		{
			class_instance = nullptr;
			delegate_stub = &static_stub<function>;
			return *this;
		}

		template <class C, R(C::*function)()>
		Delegate& bind(C* instance)
		{
			class_instance = instance;
			delegate_stub = &member_stub<C, function>;
			return *this;
		}

		R operator()()
		{
			return delegate_stub(class_instance);
		}

		bool is_valid() const
		{
			return (delegate_stub != nullptr);
		}
	};

	// void return, no parameters
	template <>
	class Delegate<void()>
	{
		void* class_instance;
		typedef void(*function_type)(void*);
		function_type delegate_stub;

	public:
		Delegate()
			: class_instance(nullptr)
			, delegate_stub(nullptr)
		{
		}

		//
		// stub functions
		//
		template <void(*function)()>
		static inline void static_stub(void*)
		{
			function();
		}

		template <class C, void (C::*function)()>
		static inline void member_stub(void* instance)
		{
			C* cl = reinterpret_cast<C*>(instance);
			(cl->*function)();
		}

		//
		// bind functions
		//
		template <void(*function)()>
		Delegate& bind()
		{
			class_instance = nullptr;
			delegate_stub = &static_stub<function>;
			return *this;
		}

		template <class C, void(C::*function)()>
		Delegate& bind(C* instance)
		{
			class_instance = instance;
			delegate_stub = &member_stub<C, function>;
			return *this;
		}

		void operator()()
		{
			delegate_stub(class_instance);
		}

		bool is_valid() const
		{
			return (delegate_stub != nullptr);
		}
	}; // Delegate

} // namespace gemini
