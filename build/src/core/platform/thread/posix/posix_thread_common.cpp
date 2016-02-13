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

#include <signal.h> // for pthread_kill
#include <unistd.h> // for usleep

#include <algorithm>
#include <semaphore.h>

namespace platform
{
	void* posix_thread_entry(void* data)
	{
		Thread* thread_data = static_cast<Thread*>(data);
		thread_data->thread_id = thread_id();
		// TODO: determine how to get the thread id
//		ThreadId tid = thread_id();
//		memcpy(&thread_data->thread_id, &tid, std::min(sizeof(uint64_t), sizeof(ThreadId)));

#if defined(PTHREAD_CANCEL_ASYNCHRONOUS)
		// allow this thread to be cancelled at anytime
		pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, 0);
#endif
		thread_data->entry(thread_data->userdata);

		return 0;
	}


	Result posix_thread_create(Thread& thread, ThreadEntry entry, void* data)
	{
		pthread_attr_t attrib;
		if (pthread_attr_init(&attrib) != 0)
		{
			return Result::failure("Unable to initialize pthread_attr_t");
		}

		// We can start this in detached or joinable. detached threads will release
		// their resources once they terminate, but cannot be synchronized.
		// joinable threads must have pthread_join called on them to release resources,
		// but this allows thread sync.
		if (pthread_attr_setdetachstate(&attrib, PTHREAD_CREATE_JOINABLE) != 0)
		{
			return Result::failure("Unable to set pthread attach state");
		}

		thread.entry = entry;
		thread.userdata = data;

		int result = pthread_create(&thread.handle, &attrib, posix_thread_entry, &thread);
		if (result == 0)
		{
			thread.state = THREAD_STATE_ACTIVE;
			return Result::success();
		}
		else
		{
			thread.state = THREAD_STATE_INACTIVE;
			return Result::failure("Unable to create thread!");
		}
	}


	int posix_thread_join(Thread& thread)
	{
		int result = pthread_join(thread.handle, 0);

		if (result == 0)
		{
			thread.state = THREAD_STATE_INACTIVE;
			return 0;
		}

		return 1;
	}

	void posix_thread_sleep(int milliseconds)
	{
		usleep(milliseconds * MicrosecondsPerMillisecond);
	}

	void posix_thread_detach(Thread& thread)
	{
		int result = pthread_cancel(thread.handle);

		assert(result == 0);
		thread.state = THREAD_STATE_INACTIVE;
	}

	ThreadId posix_thread_id()
	{
		return pthread_self();
	}


	class PosixSemaphore : public platform::Semaphore
	{
	public:
		sem_t handle;

		PosixSemaphore(int32_t initial_count, int32_t max_count)
		{
			sem_init(&handle, initial_count, max_count);
		}

		~PosixSemaphore()
		{
			sem_destroy(&handle);
		}
	};

	Semaphore* posix_semaphore_create(int32_t initial_count, int32_t max_count)
	{
		return MEMORY_NEW(PosixSemaphore, platform::get_platform_allocator())(initial_count, max_count);
	}

	void posix_semaphore_wait(Semaphore* sem)
	{
		PosixSemaphore* posix_sem = static_cast<PosixSemaphore*>(sem);
		sem_wait(&posix_sem->handle);
	}

	void posix_semaphore_signal(Semaphore* sem)
	{
		PosixSemaphore* posix_sem = static_cast<PosixSemaphore*>(sem);
		sem_post(&posix_sem->handle);
	}

	void posix_semaphore_destroy(Semaphore* sem)
	{
		PosixSemaphore* posix_sem = static_cast<PosixSemaphore*>(sem);
		MEMORY_DELETE(posix_sem, platform::get_platform_allocator());
	}

} // namespace platform
