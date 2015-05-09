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

#include <platform/mem.h>

#include <assert.h>

namespace core
{
	// This is an implementation of a FIFO queue
	template <class Type, int MaxSize>
	class FixedSizeQueue
	{
	private:
	public:
		Type elements[MaxSize];
		size_t total_elements;
		size_t tail_index;
		static Type default_value;
		
		FixedSizeQueue() :
			total_elements(MaxSize),
			tail_index(0)
		{
		}
		
		~FixedSizeQueue()
		{
		}
	
		bool empty() const
		{
			return tail_index == 0;
		}
	
		size_t size() const
		{
			return tail_index;
		}

		// An item onto the queue. This can fail if it's full.
		bool push_back(const Type& item)
		{
			if (tail_index != (total_elements-1))
			{
				elements[tail_index++] = item;
				return true;
			}
			
			return false;
		}
		
		Type pop()
		{
			// If you hit this, there are no items in the queue.
			assert(tail_index > 0);
	
			Type item = elements[tail_index-1];
			tail_index--;
			return item;
		}
	};
} // namespace core