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

#include <sys/time.h>
#include <time.h>

namespace platform
{
	// Check for and use a monotonic clock, if one exists.
	// Fallback to gettimeofday.
	// Per Doug Coleman and Thomas Habets
	// http://code-factor.blogspot.com/2009/11/monotonic-timers.html
	// https://blog.habets.se/2010/09/gettimeofday-should-never-be-used-to-measure-time

	typedef uint64_t (*get_microseconds_func)();
	static get_microseconds_func get_microseconds = nullptr;

	static uint64_t get_microseconds_monotonic()
	{
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		return ((ts.tv_sec * MicrosecondsPerSecond) + (ts.tv_nsec * MicrosecondsPerNanosecond));
	}

	static uint64_t get_microseconds_gettimeofday()
	{
		struct timeval now;
		gettimeofday(&now, 0);
		return ((now.tv_sec * MicrosecondsPerSecond) + now.tv_usec);
	}

	Result timer_startup()
	{
		// determine if this kernel has a monotonic clock
		struct timespec ts;
		bool has_monotonic = (clock_gettime(CLOCK_MONOTONIC, &ts) != 0);

		get_microseconds = get_microseconds_gettimeofday;

		if (has_monotonic)
		{
			get_microseconds = get_microseconds_monotonic;
		}

		return Result::success();
	}

	void timer_shutdown()
	{
	}

	uint64_t microseconds()
	{
		return get_microseconds();
	}

	uint64_t time_ticks()
	{
		return get_microseconds();
	}

	void datetime(DateTime& datetime)
	{
		posix_datetime(datetime);
	}
} // namespace platform
