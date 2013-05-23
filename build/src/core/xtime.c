// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone

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
#include "xtime.h"

#ifdef __cplusplus
extern "C" {
#endif


void xtime_startup( xtime_t * t )
{
#if PARANOID
	if ( !t )
	{
		printf( "timevalue_t * is invalid!\n" );
		return;
	}
#endif

#if LINUX || __APPLE__ || __ANDROID__
	gettimeofday(&t->initialtime, 0);
#elif _WIN32

	// get timer freq
	QueryPerformanceFrequency( &t->frequency );
	
	// get current count
	QueryPerformanceCounter( &t->last );

#endif
}

double xtime_msec( xtime_t * t )
{
#if PARANOID
	if (!t)
	{
		printf( "timevalue_t * is invalid!\n" );
		return 0;
	}
#endif

#if LINUX || __APPLE__ || __ANDROID__
    struct timeval now;
    gettimeofday(&now, 0);
    return ((now.tv_sec-t->initialtime.tv_sec)*1000.0f + (now.tv_usec-t->initialtime.tv_usec)/1000.0f);
#elif _WIN32
	LARGE_INTEGER now;
	QueryPerformanceCounter( &now );

	return ((now.QuadPart - t->last.QuadPart) / (double)t->frequency.QuadPart) * 1000.0;
#endif
}

/* fetch the current time of day */
void xtime_now( xdatetime_t * dt )
{
#if _WIN32
	SYSTEMTIME st;

	GetLocalTime( &st );

	dt->day = st.wDay;
	dt->dayOfWeek = st.wDayOfWeek;
	dt->hour = st.wHour;
	dt->milliseconds = st.wMilliseconds;
	dt->minute = st.wMinute;
	dt->month = st.wMonth;
	dt->second = st.wSecond;
	dt->year = st.wYear;

#elif LINUX || __APPLE__ || __ANDROID__
	// this code from cplusplus.com
	struct tm * timeinfo;
	time_t rawtime;

	time( &rawtime );
	timeinfo = localtime( &rawtime );
	dt->year = timeinfo->tm_year + 1900;
	dt->day = timeinfo->tm_mday;
	dt->month = timeinfo->tm_mon + 1;
	dt->dayOfWeek = timeinfo->tm_wday; // 0-6, since Sunday
	dt->hour = timeinfo->tm_hour; // tm_hour is 24-hour format
	dt->minute = timeinfo->tm_min;
	dt->second = timeinfo->tm_sec;
	//tm_isdst > 0 if Daylight Savings Time is in effect

#endif
}
	
long xtime_epoch()
{
	return time(0);
}		

#ifdef __cplusplus
}; // extern "C"
#endif
