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

#include "platform_internal.h"

#define WIN32_LEAN_AND_MEAN 1
#define NOMINMAX
#include <windows.h>

namespace platform
{
	static LARGE_INTEGER _frequency;

	Result timer_startup()
	{
		// cache off the frequency for later timer use.
		QueryPerformanceFrequency(&_frequency);

		// Lock this thread to a single CPU to prevent jitter when timing.
		// This should be called from the main game thread.
		DWORD affinity_mask = 1;
		SetThreadAffinityMask(GetCurrentThread(), reinterpret_cast<DWORD_PTR>(&affinity_mask));

		return Result::success();
	}

	void timer_shutdown()
	{
	}

	uint64_t microseconds()
	{
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		return static_cast<uint64_t>(((static_cast<double>(now.QuadPart)) / static_cast<double>(_frequency.QuadPart)) * MicrosecondsPerSecond);
	}

	uint64_t time_ticks()
	{
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		return static_cast<uint64_t>(now.QuadPart);
	}

	void datetime(DateTime& datetime)
	{
		SYSTEMTIME st;
		GetLocalTime(&st);

		datetime.day = st.wDay;
		datetime.dayOfWeek = st.wDayOfWeek;
		datetime.hour = st.wHour;
		datetime.milliseconds = st.wMilliseconds;
		datetime.minute = st.wMinute;
		datetime.month = st.wMonth;
		datetime.second = st.wSecond;
		datetime.year = st.wYear;
	}
} // namespace platform
