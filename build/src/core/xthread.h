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
	XTHREAD_ENTRY my_thread( void * data )
	{
		xthread_sleep( 5 );
		return 0;
	}

	xthread_t worker;
	xthread_start( &worker, my_thread, 0 );
	xthread_join( &worker, 2000 );

#endif

#pragma once

// os includes
#if _WIN32
	#include <windows.h>
	#include <process.h> // for _beginxthreadex
#elif LINUX || __APPLE__ || __ANDROID__
	#include <pthread.h>
	#include <unistd.h>
	#include <sys/types.h>
#endif


// xthread entry function
#if _WIN32
	typedef unsigned int (__stdcall *xthread_entry)( void * );
	#define XTHREAD_ENTRY unsigned int __stdcall
#elif LINUX || __APPLE__ || __ANDROID__
	typedef void * (*xthread_entry)( void * );
	#define XTHREAD_ENTRY void *
#endif

// xthread states
#define XTHREAD_STATE_INVALID		0
#define XTHREAD_STATE_ACTIVE			1
#define XTHREAD_STATE_SUSPENDED		2
#define XTHREAD_STATE_STOPPED		3

#ifdef __cplusplus
extern "C" {
#endif

typedef struct xthread_s
{
	unsigned int state;
#if _WIN32
	unsigned int id;
	HANDLE handle;
#elif LINUX || __APPLE__ || __ANDROID__
	pthread_t handle;
	pthread_attr_t attribs;
#endif
} xthread_t;


// start a xthread
void xthread_start( xthread_t * t, xthread_entry entry, void * data );

// kill a xthread
void xthread_stop( xthread_t * t );

// retrieve xthread status (one of the states above)
unsigned int xthread_status( xthread_t * t );

#if 0
// get a xthread's id
unsigned int xthread_id( xthread_t * t );

// returns this xthread's ID (according to the OS)
int xthread_selfid();
#endif


// wait for a xthread to complete. WINDOWS ONLY: Waits milliseconds before killing xthread. If timeout_milliseconds is -1, wait indefinitely
// returns 0 if the xthread timed out
// returns 1 if the xthread exited before timeout
int xthread_join( xthread_t * t, int timeout_milliseconds );

// let the xthread sleep for at least <milliseconds>
void xthread_sleep( int milliseconds );

//--------------------------------------------------

typedef struct xmutex_s
{
#if _WIN32
	CRITICAL_SECTION cs;
#elif LINUX || __APPLE__ || __ANDROID__
	pthread_mutex_t handle;
	pthread_mutexattr_t attribs;
#endif
} xmutex_t;

// create mutex
void xmutex_create( xmutex_t * m );

// destroy, cleanup
void xmutex_destroy( xmutex_t * m );

// locks a mutex
void xmutex_lock( xmutex_t * m );

// unlock mutex
void xmutex_unlock( xmutex_t * m );

#ifdef __cplusplus
}; // extern "C"
#endif

