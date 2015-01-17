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

#include "mem.h"
#include <limits> // for std::numeric_limits

namespace platform
{
	namespace memory
	{
		
		/// Custom STL allocator class for debugging / memory tracking
		template<class _Ty>
		class DebugAllocator
		{	// generic DebugAllocator for objects of class _Ty
		public:
			typedef _Ty				value_type;
			typedef _Ty*				pointer;
			typedef const _Ty*			const_pointer;
			typedef _Ty&				reference;
			typedef const _Ty&			const_reference;
			typedef std::size_t		size_type;
			typedef std::ptrdiff_t		difference_type;
			
			template<class U>
			struct rebind
			{	// convert an DebugAllocator<_Ty> to an DebugAllocator <U>
				typedef DebugAllocator<U> other;
			};
			
			pointer address(reference value) const
			{	// return address of mutable _Val
				return (&value);
			}
			
			const_pointer address(const_reference value) const
			{	// return address of nonmutable _Val
				return (&value);
			}
			
			DebugAllocator() throw()
			{	// construct default DebugAllocator (do nothing)
			}
			
			DebugAllocator(const DebugAllocator<_Ty>&) throw()
			{	// construct by copying (do nothing)
			}
			
			template<class U>
			DebugAllocator(const DebugAllocator<U>&) throw()
			{	// construct from a related DebugAllocator (do nothing)
			}
			
			~DebugAllocator() throw()
			{
			}
			
			template<class _Other>
			DebugAllocator<_Ty>& operator=(const DebugAllocator<_Other>&)
			{	// assign from a related DebugAllocator (do nothing)
				return (*this);
			}
					
			void deallocate(pointer _Ptr, size_type)
			{	// deallocate object at _Ptr, ignore size
	//			DESTROY( _Ty, _Ptr );
				DEALLOC( _Ptr );
	//			::operator delete(_Ptr);
			}
			
			pointer allocate(size_type _Count, const void * hint = 0)
			{	// allocate array of _Count elements
	//			return CREATE_ARRAY( _Ty, _Count );

				return (pointer)ALLOC( _Count * sizeof(_Ty) );
			}
			/*
			 pointer allocate(size_type _Count, const void * = 0)
			 {	// allocate array of _Count elements, ignore hint
			 pointer ret = (pointer)(::operator new(_Count*sizeof(_Ty)));
			 
			 //
			 return ret;
			 //return (allocate(_Count));
			 }*/
			
			void construct(pointer _Ptr, const _Ty& _Val)
			{	// construct object at _Ptr with value _Val
				new((void*)_Ptr)_Ty(_Val);
			}
			
			void destroy(pointer _Ptr)
			{	// destroy object at _Ptr
				//_DESTRUCTOR(_Ty, _Ptr);
				_Ptr->~_Ty();
			}
			
			size_type max_size() const throw()
			{	// return max number of elements that can be allocated
				// use parentheses around std:: ... ::max function to avoid macro expansion (some headers define max as a macro, ugh annoying.)
				return (std::numeric_limits<std::size_t>::max)() / sizeof(_Ty);
			}
			
		};
		
		template <class T1, class T2>
		bool operator== (const DebugAllocator<T1>&, const DebugAllocator<T2>&) throw()
		{
			return true;
		}
		
		template <class T1, class T2>
		bool operator!= (const DebugAllocator<T1>&, const DebugAllocator<T2>&) throw()
		{
			return true;
		}
	} // namespace memory
} // namespace platform