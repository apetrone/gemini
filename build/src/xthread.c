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
#include "xthread.h"

#define PARANOID 1

#if LINUX || __APPLE__
	#include <signal.h> // for pthread_kill
#endif

#ifdef __cplusplus
extern "C" {
#endif

void xthread_start( xthread_t * t, xthread_entry entry, void * data )
{
	if ( !t || !entry )
		return;

#if _WIN32
	t->handle = CreateThread( 0, 0, entry, data, 0, 0 );
	//t->handle = (HANDLE)_beginxthreadex( 0, 0, entry, data, 0, &t->id );
	t->state = XTHREAD_STATE_ACTIVE;
#elif LINUX || __APPLE__
	{
		int err;

		// create a joinable xthread
		pthread_attr_init( &t->attribs );
		pthread_attr_setdetachstate( &t->attribs, PTHREAD_CREATE_JOINABLE );

		err = pthread_create( &t->handle, &t->attribs, entry, data );

		if ( err == 0 )
		{
			t->state = XTHREAD_STATE_ACTIVE; // success
		}
		else
		{
			t->state = XTHREAD_STATE_INVALID;
		}
	}
#endif
} // xthread_start

void xthread_stop( xthread_t * t )
{
	if ( !t )
		return;

#if _WIN32
	if ( WaitForSingleObject( t->handle, 1000 ) != WAIT_OBJECT_0 )
	{
		// Thread timed out!
	}

	CloseHandle( (HANDLE)t->handle );
	t->handle = 0;
	t->state = XTHREAD_STATE_STOPPED;
#elif LINUX || __APPLE__
	pthread_setcanceltype( PTHREAD_CANCEL_ASYNCHRONOUS, 0 );
	pthread_cancel( (pthread_t)&t->handle );
	t->state = XTHREAD_STATE_STOPPED;
#endif
} // xthread_stop

unsigned int xthread_status( xthread_t * t )
{
	if ( !t )
	{
		return XTHREAD_STATE_INVALID;
	}
#if _WIN32
	{
		unsigned int code;
		if ( GetExitCodeThread( t->handle, (LPDWORD)&code ) )
		{
			if ( code == STILL_ACTIVE )
			{
				t->state = XTHREAD_STATE_ACTIVE;
			}
			else
			{
				t->state = XTHREAD_STATE_STOPPED;
			}
		}
	}

#elif LINUX || __APPLE__
	if ( pthread_kill( t->handle, 0 ) == 0 )
	{
		// xthread is still active
		t->state = XTHREAD_STATE_ACTIVE;
	}
	else
	{
		t->state = XTHREAD_STATE_STOPPED;
	}
#endif

	return t->state;
} // xthread_status

#if 0
unsigned int xthread_id( xthread_t * t )
{
	if ( !t )
		return 0;

	return t->id;
} // xthread_id

int xthread_selfid()
{
#if _WIN32
	return GetCurrentThreadId();
#elif LINUX || __APPLE__
	return pthread_self();
#endif
} // xthread_selfid
#endif

int xthread_join( xthread_t * t, int timeout_milliseconds )
{
	int ret;
	
	if ( t && (t->state != XTHREAD_STATE_ACTIVE) )
		return 0;

#if _WIN32

	if ( timeout_milliseconds == -1 )
		timeout_milliseconds = INFINITE;

	// NOTE:
	// * MsgWait continues the MessagePump in case this xthread created any Windows.
	// * MsgWaitForMultipleObjectsEx( 1, &hThread, milliseconds, QS_ALLEVENTS, 0 );
	ret = WaitForSingleObjectEx( (HANDLE)t->handle, timeout_milliseconds, 0 );

	if ( ret != WAIT_OBJECT_0 )
	{
		// xthread did not terminate
		TerminateThread( (HANDLE)t->handle, 0 );
	}

	// close the OS handle
	CloseHandle( (HANDLE)t->handle );
	t->handle = 0;
	t->state = XTHREAD_STATE_STOPPED;

	if ( ret == WAIT_OBJECT_0 )
	{
		return 1;
	}
	else
	{
		return 0;
	}
#elif LINUX || __APPLE__
	ret = pthread_join( t->handle, 0 );

	if ( ret == 0 )
	{
		t->state = XTHREAD_STATE_STOPPED;
		return 1;
	}
	else
	{
		t->state = XTHREAD_STATE_INVALID;
		return 0;
	}
#endif
} // xthread_join

void xthread_sleep( int milliseconds )
{
#if _WIN32
	Sleep( milliseconds );
#elif LINUX || __APPLE__
	// convert milliseconds to microseconds
	usleep( milliseconds * 1000 );
#endif
} // xthread_sleep

//--------------------------------------------------

// NOTE: 
// * pthread_mutex_trylock( &m->handle );
// * if mutex is unlocked, this will lock it and return zero.
// * otherwise it will return with EBUSY



void xmutex_create( xmutex_t * m )
{
#if PARANOID
	if ( !m )
		return;
#endif

#if _WIN32
	InitializeCriticalSection( &m->cs );
#elif LINUX || __APPLE__
	pthread_mutexattr_init( &m->attribs );
	pthread_mutex_init( &m->handle, &m->attribs );
#endif
} // xmutex_create

void xmutex_destroy( xmutex_t * m )
{
#if _WIN32
	DeleteCriticalSection( &m->cs );
#elif LINUX || __APPLE__
	pthread_mutex_destroy( &m->handle );
	pthread_mutexattr_destroy( &m->attribs );
#endif
} // xmutex_destroy

void xmutex_lock( xmutex_t * m )
{
#if PARANOID
	if ( !m )
		return;
#endif

#if _WIN32
	if ( m->cs.DebugInfo != 0 )
	{
		EnterCriticalSection( &m->cs );
	}
#elif LINUX || __APPLE__
	pthread_mutex_lock( &m->handle );
#endif
} // xmutex_lock

void xmutex_unlock( xmutex_t * m )
{
#if PARANOID
	if ( !m )
		return;
#endif

#if _WIN32
	LeaveCriticalSection( &m->cs );
#elif LINUX || __APPLE__
	pthread_mutex_unlock( &m->handle );
#endif
} // xmutex_unlock

#ifdef __cplusplus
}; // extern "C"
#endif
