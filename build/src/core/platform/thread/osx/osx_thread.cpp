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
#include "platform_internal.h"

namespace platform
{
	Thread* thread_create(ThreadEntry entry, void* data)
	{
		return posix_thread_create(entry, data);
	}

	void thread_destroy(Thread* thread)
	{
		posix_thread_destroy(thread);
	}

	int thread_join(Thread* thread, uint32_t)
	{
		return posix_thread_join(thread);
	}

	void thread_sleep(int milliseconds)
	{
		posix_thread_sleep(milliseconds);
	}

	pthread_t thread_id()
	{
		return posix_thread_id();
	}

	ThreadStatus thread_status(Thread* thread)
	{
		return posix_thread_status(thread);
	}

	bool thread_is_active(Thread* thread)
	{
		return posix_thread_is_active(thread);
	}

	Semaphore* semaphore_create(uint32_t initial_count, uint32_t max_count)
	{
		return posix_semaphore_create(initial_count, max_count);
	}

	void semaphore_wait(Semaphore* sem)
	{
		assert(sem);
		posix_semaphore_wait(sem);
	}

	void semaphore_signal(Semaphore* sem, uint32_t count)
	{
		assert(sem);
		posix_semaphore_signal(sem, count);
	}

	void semaphore_destroy(Semaphore* sem)
	{
		assert(sem);
		posix_semaphore_destroy(sem);
	}

} // namespace platform
