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

#include <mach/mach_time.h>

namespace platform
{
	static double _time_scale = 0;
	static uint64_t _time_start = 0;

	Result timer_startup()
	{
		_time_start = mach_absolute_time();
		mach_timebase_info_data_t timebase;
		if (mach_timebase_info(&timebase) == KERN_SUCCESS)
		{
			// convert the timescale to return microseconds
			_time_scale = timebase.numer/timebase.denom / 1e3;
			return Result(Result::Success);
		}
		else
		{
			return Result(Result::Failure, "Failed fetching timebase!");
		}
	}

	void timer_shutdown()
	{
	}

	uint64_t microseconds()
	{
		return (mach_absolute_time() - _time_start) * _time_scale;
	}
	
	void datetime(DateTime& datetime)
	{
		posix_datetime(datetime);
	}
} // namespace platform