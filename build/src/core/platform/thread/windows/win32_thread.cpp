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

	struct Win32Thread : public Thread
	{
		HANDLE handle;
		DWORD thread_id;
		ThreadStatus state;
		ThreadEntry entry;

		// signaled when the thread should close
		HANDLE close_event;

		// signaled when the win32 thread exits.
		HANDLE exit_event;

		Win32Thread()
			: handle(NULL)
			, entry(nullptr)
			, close_event(NULL)
			, exit_event(NULL)
		{
			user_data = nullptr;
		}

		~Win32Thread()
		{
			if (close_event)
			{
				CloseHandle(close_event);
				close_event = NULL;
			}

			if (exit_event)
			{
				CloseHandle(exit_event);
				exit_event = NULL;
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
		thread->entry(thread);
		SetEvent(thread->exit_event);
		return 0;
	}

	Thread* thread_create(ThreadEntry entry, void* data)
	{
		Win32Thread* thread = MEMORY_NEW(Win32Thread, platform::get_platform_allocator());
		thread->entry = entry;
		thread->state = THREAD_STATE_ACTIVE;
		thread->user_data = data;
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

			thread->exit_event = CreateEvent(
				NULL, // security attributes
				TRUE, // manual reset
				FALSE, // initial state
				NULL // name
			);

			return thread;
		}
		else
		{
			thread_destroy(thread);
			return nullptr;
		}
	}

	void thread_destroy(Thread* thread)
	{
		Win32Thread* native_thread = static_cast<Win32Thread*>(thread);
		MEMORY_DELETE(native_thread, platform::get_platform_allocator());
	}

	int thread_join(Thread* thread, uint32_t timeout_milliseconds)
	{
		Win32Thread* native_thread = static_cast<Win32Thread*>(thread);
		if (native_thread->state != THREAD_STATE_INACTIVE)
		{
			// tell the thread it should exit
			SetEvent(native_thread->close_event);

			// No timeout if 0 is passed in.
			if (timeout_milliseconds == 0)
			{
				timeout_milliseconds = INFINITE;
			}

			DWORD wait_close = WaitForSingleObject(native_thread->exit_event, timeout_milliseconds);
			if (WAIT_OBJECT_0 == wait_close)
			{
				CloseHandle(native_thread->exit_event);
				native_thread->exit_event = NULL;
				native_thread->state = THREAD_STATE_INACTIVE;
			}
			else if (WAIT_TIMEOUT == wait_close)
			{
				// Time expired! terminate the thread.
				// NOTE: This is not advised by MSDN; but since the exit_event
				// hasn't been reached, this is our only choice.
				// It doesn't allow the thread to properly clean up if the
				// thread allocated any HANDLEs.
				TerminateThread(native_thread->handle, static_cast<DWORD>(-1));
				CloseHandle(native_thread->handle);
				native_thread->handle = NULL;
				return 1;
			}
		}

		return 0;
	}

	void thread_sleep(int milliseconds)
	{
		Sleep(static_cast<DWORD>(milliseconds));
	}

	DWORD thread_id()
	{
		return GetCurrentThreadId();
	}

	ThreadStatus thread_status(const Thread* thread)
	{
		const Win32Thread* native_thread = static_cast<const Win32Thread*>(thread);
		return native_thread->state;
	}

	bool thread_is_active(Thread* thread)
	{
		Win32Thread* native_thread = static_cast<Win32Thread*>(thread);
		if (WaitForSingleObject(native_thread->close_event, 0) == WAIT_OBJECT_0)
		{
			CloseHandle(native_thread->close_event);
			native_thread->close_event = NULL;
			native_thread->state = THREAD_STATE_INACTIVE;
			return false;
		}

		return true;
	}

	class Win32Semaphore : public Semaphore
	{
	public:
		HANDLE handle;

		Win32Semaphore(uint32_t initial_count, uint32_t max_count)
		{

			handle = CreateSemaphoreEx(
				// security attributes,
				// (NULL cannot be inherited by child processes)
				NULL,

				// number of threads awake at startup
				static_cast<LONG>(initial_count),

				// maximum count
				static_cast<LONG>(max_count),

				// name
				NULL,

				// flags: reserved and must be zero
				0,

				// access mask
				SEMAPHORE_ALL_ACCESS
			);
		}

		~Win32Semaphore()
		{
			CloseHandle(handle);
			handle = NULL;
		}
	};

	Semaphore* semaphore_create(uint32_t initial_count, uint32_t max_count)
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

	void semaphore_signal(Semaphore* sem, uint32_t count)
	{
		Win32Semaphore* semaphore = static_cast<Win32Semaphore*>(sem);
		ReleaseSemaphore(semaphore->handle, static_cast<LONG>(count), NULL);
	}

	void semaphore_destroy(Semaphore* sem)
	{
		MEMORY_DELETE(sem, platform::get_platform_allocator());
	}

} // namespace platform
