// -------------------------------------------------------------
// Copyright (C) 2015- Adam Petrone

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM,OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
// -------------------------------------------------------------

#include "mem.h"
#include "posix_timer.h"

#include <sys/time.h>
#include <time.h>
#include <assert.h>

namespace platform
{
	struct PosixTimerHandle : public TimerHandle
	{
		struct timeval initial_time;
	};

	TimerHandle* posix_create_timer()
	{
		PosixTimerHandle* handle = CREATE(PosixTimerHandle);
		gettimeofday(&handle->initial_time, 0);
		return handle;
	}
	
	void posix_destroy_timer(TimerHandle* timer)
	{
		PosixTimerHandle* handle = static_cast<PosixTimerHandle*>(timer);
		DESTROY(PosixTimerHandle, handle);
	}
	
	double posix_get_timer_msec(TimerHandle* timer)
	{
		PosixTimerHandle* handle = static_cast<PosixTimerHandle*>(timer);
		assert(handle != 0);
		
		struct timeval now;
		gettimeofday(&now, 0);
		return ((now.tv_sec-handle->initial_time.tv_sec)*1000.0f + (now.tv_usec-handle->initial_time.tv_usec)/1000.0f);
	}
	
	void posix_get_date_time(DateTime& datetime)
	{
		// this code is from cplusplus.com
		struct tm * timeinfo;
		time_t rawtime;
		
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		datetime.year = timeinfo->tm_year + 1900;
		datetime.day = timeinfo->tm_mday;
		datetime.month = timeinfo->tm_mon + 1;
		datetime.dayOfWeek = timeinfo->tm_wday; // 0-6, since Sunday
		datetime.hour = timeinfo->tm_hour; // tm_hour is 24-hour format
		datetime.minute = timeinfo->tm_min;
		datetime.second = timeinfo->tm_sec;
		//tm_isdst > 0 if Daylight Savings Time is in effect
	}
} // namespace platform
