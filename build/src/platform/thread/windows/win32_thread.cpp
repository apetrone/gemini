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
	DWORD __stdcall windows_thread_entry(LPVOID data)
	{
		Thread* thread_data = static_cast<Thread*>(data);
		thread_data->thread_id = thread_id();
		thread_data->entry(thread_data->userdata);
		return 0;
	}

	Result thread_create(Thread& thread, ThreadEntry entry, void* data)
	{
		HANDLE thread_handle = CreateThread(0, 0, windows_thread_entry, &thread, 0, &thread.thread_id);
		if (thread_handle)
		{
			thread.handle = thread_handle;
			thread.state = THREAD_STATE_ACTIVE;
			thread.entry = entry;
			return Result::success();
		}

		return Result::failure("Unable to create thread!");
	}

	int thread_join(Thread& thread)
	{
		int result = 0;

		// NOTE:
		// * MsgWait continues the MessagePump in case this thread created any Windows.
		// * MsgWaitForMultipleObjectsEx( 1, &hThread, milliseconds, QS_ALLEVENTS, 0 );
		if (WaitForSingleObject(thread.handle, INFINITE) != WAIT_OBJECT_0)
		{
			TerminateThread(thread.handle, 0);
			result = 1;
		}

		CloseHandle(thread.handle);
		thread.state = THREAD_STATE_INACTIVE;
		return result;
	}

	void thread_sleep(int milliseconds)
	{
		Sleep(milliseconds);
	}

	void thread_detach(Thread& thread)
	{
		CloseHandle(thread.handle);
		thread.state = THREAD_STATE_INACTIVE;
	}

	ThreadId thread_id()
	{
		return GetCurrentThreadId();
	}

} // namespace platform