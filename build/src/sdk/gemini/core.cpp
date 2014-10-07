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
#include <gemini/typedefs.h>

#include <gemini/core.h>
#include <gemini/core/filesystem.h>
#include <gemini/core/log.h>
#include <gemini/core/timer.h>

#include <gemini/platform.h>

#include <slim/xlog.h>
#include <slim/xtime.h>

namespace core
{
	namespace _internal
	{
		#define GEMINI_LOG_PATH "logs"
		const unsigned int GEMINI_DATETIME_STRING_MAX = 128;
				
		static xlog_t _system_log;
		// log handler function declarations
		void file_logger_message( xlog_handler_t * handler, const char * message, const char * filename, const char * function, int line, int type );
		int file_logger_open( xlog_handler_t * handler );
		void file_logger_close( xlog_handler_t * handler );
		
		void stdout_message( xlog_handler_t * handler, const char * message, const char * filename, const char * function, int line, int type );
		int stdout_open( xlog_handler_t * handler );
		void stdout_close( xlog_handler_t * handler );
		
#if _WIN32
		void vs_message( xlog_handler_t * handler, const char * message, const char * filename, const char * function, int line, int type );
		int vs_open( xlog_handler_t * handler );
		void vs_close( xlog_handler_t * handler );
#endif
		
#if __ANDROID__
		void log_android_message( xlog_handler_t * handler, const char * message, const char * filename, const char * function, int line, int type );
		int log_android_open( xlog_handler_t * handler );
		void log_android_close( xlog_handler_t * handler );
#endif
				
		Result open_log_handlers()
		{
			Result result(Result::Success);
			
			int total_log_handlers = 0;
			
			// setup system log
			xlog_init( &_system_log );
			
			xlog_set_default_log( &_system_log );
			
#if _WIN32
			xlog_handler_t msvc_logger;
			memset( &msvc_logger, 0, sizeof(xlog_handler_t) );
			msvc_logger.close = vs_close;
			msvc_logger.open = vs_open;
			msvc_logger.message = vs_message;
			msvc_logger.userdata = 0;
			
			++total_log_handlers;
			xlog_add_handler( &_system_log, &msvc_logger );
#endif
			
#ifndef __ANDROID__
			xlog_handler_t stdout_logger;
			memset( &stdout_logger, 0, sizeof(xlog_handler_t) );
			stdout_logger.message = stdout_message;
			stdout_logger.open = stdout_open;
			stdout_logger.close = stdout_close;
			
			++total_log_handlers;
			xlog_add_handler( &_system_log, &stdout_logger );
#endif
			
#if !PLATFORM_IS_MOBILE
			
			
			xdatetime_t dt;
			xtime_now( &dt );
			char datetime_string[ GEMINI_DATETIME_STRING_MAX ];
			xstr_sprintf( datetime_string, GEMINI_DATETIME_STRING_MAX, "%02d-%02d-%04d-%02d-%02d-%02d.log",
						 dt.month, dt.day, dt.year, dt.hour, dt.minute, dt.second );
			
			char logdir[MAX_PATH_SIZE];
			memset( logdir, 0, MAX_PATH_SIZE );
			xstr_ncpy( logdir, filesystem::content_directory(), -1 );
			xstr_cat( logdir, "/" GEMINI_LOG_PATH "/" );
			platform::path::normalize( logdir, MAX_PATH_SIZE );
			
			// make sure target folder is created
			platform::path::make_directories( logdir );
			
			xstr_cat( logdir, datetime_string );
			
			xlog_handler_t filelogger;
			filelogger.message = file_logger_message;
			filelogger.open = file_logger_open;
			filelogger.close = file_logger_close;
			filelogger.userdata = logdir;
			
			xlog_add_handler( &_system_log, &filelogger );
			
			++total_log_handlers;
#endif
			
			
#if __ANDROID__
			xlog_handler_t android_log;
			android_log.message = log_android_message;
			android_log.open = log_android_open;
			android_log.close = log_android_close;
			android_log.userdata = 0;
			
			xlog_add_handler( &_system_log, &android_log );
			++total_log_handlers;
#endif
			
			if ( xlog_open( &_system_log ) < total_log_handlers )
			{
				fprintf( stderr, "Could not open one or more log handlers\n" );
				result = core::Result( core::Result::Warning, "Could not open one or more log handlers" );
			}
			
			LOGV( "Logging system initialized.\n" );
			
			return result;
		} // open_log_handlers
		
		
		void close_log_handlers()
		{
			xlog_close( &_system_log );
		} // close_log_handlers
	}; // namespace _internal

	
	Result startup()
	{
		core::Result result = platform::startup();
		if (result.failed())
		{
			fprintf(stderr, "platform startup failed! %s\n", result.message);
			return result;
		}

		// open logs
		result = _internal::open_log_handlers();
		if (result.failed())
		{
			fprintf( stderr, "failed to open logging handlers: %s\n", result.message);
			return result;
		}
		
		LOGV("setting root to '%s', content: '%s'\n", filesystem::root_directory(), filesystem::content_directory());
		
		return result;
	} // startup
	
	void shutdown()
	{
		_internal::close_log_handlers();
		
		platform::shutdown();
	} // shutdown

}; // namespace core