// -------------------------------------------------------------
// Copyright (C) 2012- Adam Petrone

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
#pragma once

#include <xstr.h>
#include <xthread.h>

#define XLOG_MAX_HANDLERS 3
#define XLOG_VERBOSE 1
#define XLOG_WARNING 2
#define XLOG_ERROR 3


#if DEBUG
#else
//#define LogMsg( fmt, ... ) (void(0))
#endif

#define LogMsg( fmt, ... ) log_default( XLOG_VERBOSE, xstr_format( fmt, ##__VA_ARGS__ ), __FUNCTION__, __FILE__, __LINE__ )
#define LogWarning( fmt, ... ) log_default( XLOG_WARNING, xstr_format( fmt, ##__VA_ARGS__ ), __FUNCTION__, __FILE__, __LINE__ )
#define LogError( fmt, ... ) log_default( XLOG_ERROR, xstr_format( fmt, ##__VA_ARGS__ ), __FUNCTION__, __FILE__, __LINE__ )



#ifdef __cplusplus
extern "C"
{
#endif

// Log a message
void log_default( unsigned int type, const char * message, const char * function, const char * file, int linenumber );

typedef struct log_handler_s
{
	// called when a log message is received
	void (*message)( struct log_handler_s * handler, const char * message, const char * filename, const char * function, int line, int type );

	// called when the log should open
	// expects 0 on failure; 1 on success
	int (*open)( struct log_handler_s * handler );

	// called when the log should close
	void (*close)( struct log_handler_s * handler );

	// userdata
	void * userdata;
} log_handler_t;


typedef struct
{
	log_handler_t log_handlers[ XLOG_MAX_HANDLERS ];
	unsigned int total_handlers;
	xmutex_t log_mutex;
} log_t;

void log_init( log_t * log );
void log_add_handler( log_t * log, log_handler_t * handler );

// this will set the default log: all messages sent to log_default will redirect to log
void log_set_default_log( log_t * log );

// returns 0 on failure; else, returns number of successfully opened log handlers
int log_open( log_t * log ); // open a log
void log_message( log_t * log, const char * message, const char * filename, const char * function, int line, int type );
void log_close( log_t * log ); // close a log

#ifdef __cplusplus
}
#endif