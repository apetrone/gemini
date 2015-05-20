// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone
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

#include <queue>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace adt
{
	template <class Type>
	class ThreadSafeQueue
	{
		std::queue<Type> queue;
		std::condition_variable wait_condition;
		mutable std::mutex local_mutex;
		
	public:
		void enqueue(Type in)
		{
			std::unique_lock<std::mutex> lock(local_mutex);
			queue.push(in);
			
			lock.unlock();
			wait_condition.notify_one();
		}
		
		Type dequeue()
		{
			std::unique_lock<std::mutex> lock(local_mutex);
			while(queue.empty())
			{
				wait_condition.wait(lock);
			}
			
			Type value = queue.front();
			queue.pop();
			return value;
		}
		
		size_t size() const
		{
			std::lock_guard<std::mutex> lock(local_mutex);
			size_t total_size = queue.size();
			return total_size;
		}
	}; // ThreadSafeQueue
} // namespace adt