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

	struct Win32Thread : public ThreadTwo
	{
		HANDLE handle;
		DWORD thread_id;
		ThreadStatus state;
		ThreadEntry entry;
		void* user_data;
		HANDLE close_event;

		Win32Thread()
			: handle(NULL)
			, entry(nullptr)
			, user_data(NULL)
			, close_event(NULL)
		{
		}

		~Win32Thread()
		{
			if (close_event)
			{
				CloseHandle(close_event);
				close_event = NULL;
			}

			if (handle)
			{
				CloseHandle(handle);
				handle = NULL;
			}
		}
	};

	DWORD __stdcall windows_thread_entry(LPVOID data)
	{
		Win32Thread* thread = static_cast<Win32Thread*>(data);

		thread->entry(thread->user_data);

		for (;;)
		{


			// This is the preferred solution on Windows, to see if the thread
			// should exit its loop.
			if (WaitForSingleObjectEx(thread->close_event, 0, FALSE) == WAIT_OBJECT_0)
				return 0;
		}

		return 0;
	}

	ThreadTwo* thread_create(ThreadEntry entry, void* data)
	{
		Win32Thread* thread = MEMORY_NEW(Win32Thread, platform::get_platform_allocator());
		thread->handle = CreateThread(
			NULL,					// security attributes
			0,						// initial size of stack in bytes
			windows_thread_entry,	// start address
			thread,					// thread's data parameter
			0,						// creation flags
			&thread->thread_id		// thread id
		);

		// If you hit this assert, thread creation failed!
		assert(thread->handle != NULL);

		if (thread->handle)
		{
			thread->close_event = CreateEvent(
				NULL, // security attributes
				TRUE, // manual reset
				FALSE, // initial state
				NULL // name
			);

			thread->entry = entry;
			thread->user_data = data;
			thread->state = THREAD_STATE_ACTIVE;
			return thread;
		}
		else
		{
			thread_destroy(thread);
			return nullptr;
		}
	}

	void thread_destroy(ThreadTwo* thread)
	{
		Win32Thread* native_thread = static_cast<Win32Thread*>(thread);
		MEMORY_DELETE(native_thread, platform::get_platform_allocator());
	}

	int thread_join(ThreadTwo* thread)
	{
		Win32Thread* native_thread = static_cast<Win32Thread*>(thread);
		SetEvent(native_thread->close_event);
		return 0;
	}

	void thread_sleep(int milliseconds)
	{
		Sleep(static_cast<DWORD>(milliseconds));
	}

	void thread_detach(ThreadTwo* thread)
	{
		Win32Thread* native_thread = static_cast<Win32Thread*>(thread);
		SetEvent(native_thread->close_event);
		if (native_thread->handle)
		{
			CloseHandle(native_thread->handle);
			native_thread->handle = NULL;
		}
		native_thread->state = THREAD_STATE_INACTIVE;
	}

	ThreadId thread_id()
	{
		return GetCurrentThreadId();
	}

	ThreadStatus thread_status(ThreadTwo* thread)
	{
		Win32Thread* native_thread = static_cast<Win32Thread*>(thread);
		return native_thread->state;
	}

	class Win32Semaphore : public Semaphore
	{
	public:
		HANDLE handle;

		Win32Semaphore(int32_t initial_count, int32_t max_count)
		{
			handle = CreateSemaphoreEx(NULL, // security attributes, (NULL cannot be inherited by child processes)
				initial_count, 				// number of threads awake at startup
				max_count, 					// maximum count
				NULL,						// name
				0,							// flags: reserved and must be zero
				SEMAPHORE_ALL_ACCESS		// access mask
			);
		}

		~Win32Semaphore()
		{
			CloseHandle(handle);
			handle = NULL;
		}
	};

	Semaphore* semaphore_create(int32_t initial_count, int32_t max_count)
	{
		return MEMORY_NEW(Win32Semaphore, platform::get_platform_allocator())(initial_count, max_count);
	}

	void semaphore_wait(Semaphore* sem)
	{
		Win32Semaphore* semaphore = static_cast<Win32Semaphore*>(sem);
		WaitForSingleObjectEx(semaphore->handle, // event handle
			INFINITE,			// time-out interval in milliseconds
			FALSE				// alertable
		);
	}

	void semaphore_signal(Semaphore* sem)
	{
		Win32Semaphore* semaphore = static_cast<Win32Semaphore*>(sem);
		ReleaseSemaphore(semaphore->handle, 1, NULL);
	}

	void semaphore_destroy(Semaphore* sem)
	{
		MEMORY_DELETE(sem, platform::get_platform_allocator());
	}

} // namespace platform
