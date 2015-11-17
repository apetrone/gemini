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
	void posix_datetime(DateTime& datetime)
	{
		// this code is from cplusplus.com
		struct tm* timeinfo;
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
