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

#include <pthread.h>
#include <signal.h> // for pthread_kill
#include <unistd.h> // for usleep

#include <algorithm>
#include <semaphore.h>

namespace platform
{
	struct PosixThread : public Thread
	{
		pthread_t handle;
		ThreadStatus state;
		ThreadEntry entry;
		sem_t launch;

		PosixThread() :
			handle(0),
			state(THREAD_STATE_INACTIVE),
			entry(nullptr)
		{
			user_data = nullptr;
			sem_init(&launch, 0, 1);
		}

		~PosixThread()
		{
			sem_destroy(&launch);
		}
	};

	void* posix_thread_entry(void* data)
	{
		PosixThread* thread_data = static_cast<PosixThread*>(data);

		// signal to calling thread that this launched
		sem_post(&thread_data->launch);

#if defined(PTHREAD_CANCEL_ASYNCHRONOUS)
		// allow this thread to be cancelled at anytime
		pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, 0);
#endif
		thread_data->entry(thread_data);

		return 0;
	}


	Thread* posix_thread_create(ThreadEntry entry, void* data)
	{
		pthread_attr_t attrib;
		if (pthread_attr_init(&attrib) != 0)
		{
			assert(!"pthread_attr_init failed");
			return nullptr;
		}

		// We can start the new thread in detached or joinable state.
		// detached threads will release their resources once they terminate,
		// but cannot be synchronized.
		// joinable threads must have pthread_join called on them to release
		// resources but this allows thread sync.
		// detached threads are not supported by this API.
		if (pthread_attr_setdetachstate(&attrib, PTHREAD_CREATE_JOINABLE) != 0)
		{
			assert("!Unable to set pthread attach state");
			return nullptr;
		}

		PosixThread* thread = MEMORY_NEW(PosixThread, platform::get_platform_allocator());
		thread->entry = entry;
		thread->user_data = data;

		// try to launch
		int result = pthread_create(&thread->handle, &attrib, posix_thread_entry, thread);
		if (result == 0)
		{
			// wait for signal from spawned thread.
			sem_wait(&thread->launch);

			thread->state = THREAD_STATE_ACTIVE;
			return thread;
		}
		else
		{
			MEMORY_DELETE(thread, platform::get_platform_allocator());
			assert(!"PosixThread failed on pthread_create");
			return nullptr;
		}
	}

	void posix_thread_destroy(Thread* thread)
	{
		MEMORY_DELETE(thread, platform::get_platform_allocator());
	}

	int posix_thread_join(Thread* thread)
	{
		PosixThread* posix_thread = static_cast<PosixThread*>(thread);
		posix_thread->state = THREAD_STATE_INACTIVE;

		int result = pthread_join(posix_thread->handle, 0);
		if (result == 0)
		{

			return 0;
		}

		return 1;
	}

	void posix_thread_sleep(int milliseconds)
	{
		usleep(milliseconds * MicrosecondsPerMillisecond);
	}

	uint64_t posix_thread_id()
	{
		return reinterpret_cast<uint64_t>(pthread_self());
	}

	ThreadStatus posix_thread_status(Thread* thread)
	{
		PosixThread* posix_thread = static_cast<PosixThread*>(thread);
		return posix_thread->state;
	}

	bool posix_thread_is_active(Thread* thread)
	{
		PosixThread* posix_thread = static_cast<PosixThread*>(thread);
		return (posix_thread->state == THREAD_STATE_ACTIVE);
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

	void posix_semaphore_signal(Semaphore* sem, uint32_t count)
	{
		PosixSemaphore* posix_sem = static_cast<PosixSemaphore*>(sem);
		for (size_t index = 0; index < count; ++index)
		{
			sem_post(&posix_sem->handle);
		}
	}

	void posix_semaphore_destroy(Semaphore* sem)
	{
		PosixSemaphore* posix_sem = static_cast<PosixSemaphore*>(sem);
		MEMORY_DELETE(posix_sem, platform::get_platform_allocator());
	}

} // namespace platform
