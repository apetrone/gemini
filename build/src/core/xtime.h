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
#if 0
USAGE:

	xtime_t t;
	xtime_startup( &t );
	double msec_time = xtime_msec( &t );

#endif

#pragma once

#if LINUX || __APPLE__ || __ANDROID__
	#include <sys/time.h>
	#include <time.h>
#elif _WIN32
	#include <time.h>
	#define WIN32_LEAN_AND_MEAN 1
    #include <windows.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


typedef struct
{
#if LINUX || __APPLE__ || __ANDROID__
	struct timeval initialtime;
#elif _WIN32
	LARGE_INTEGER frequency;
	LARGE_INTEGER last;
#endif

} xtime_t;

typedef struct
{
	unsigned short year;
	unsigned short month; // 1-12
	unsigned short dayOfWeek; // 0-6, starting from Sunday
	unsigned short day;
	unsigned short hour; // 24-hour format
	unsigned short minute;
	unsigned short second;
	unsigned short milliseconds;
} xdatetime_t;

void xtime_startup( xtime_t * t );
double xtime_msec( xtime_t * t );

void xtime_now( xdatetime_t * dt );
long xtime_epoch( void );

#ifdef __cplusplus
}; // extern "C"
#endif
