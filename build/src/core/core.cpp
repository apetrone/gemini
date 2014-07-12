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
#include <gemini/util/stackstring.h>

#include <slim/xlog.h>
#include <slim/xtime.h>

#include "core.h"
#include "platform.h"
#include "filesystem.h"

namespace core
{
	namespace _internal
	{
		#define GEMINI_LOG_PATH "logs"
		const unsigned int GEMINI_DATETIME_STRING_MAX = 128;
		
		void set_content_directory_from_root( StackString<MAX_PATH_SIZE> & root )
		{
#if !TARGET_OS_IPHONE
			fs::truncate_string_at_path( &root[0], "bin" );
#endif
			fs::content_directory( &root[0], root.max_size() );
		}
		
		
		static xlog_t _system_log;
		// log handler function declarations
		void file_logger_message( xlog_handler_t * handler, const char * message, const char * filename, const char * function, int line, int type );
		int file_logger_open( xlog_handler_t * handler );
		void file_logger_close( xlog_handler_t * handler );
		
		void stdout_message( xlog_handler_t * handler, const char * message, const char * filename, const char * function, int line, int type );
		int stdout_open( xlog_handler_t * handler );
		void stdout_close( xlog_handler_t * handler );
		
#if PLATFORM_WINDOWS
		void vs_message( xlog_handler_t * handler, const char * message, const char * filename, const char * function, int line, int type );
		int vs_open( xlog_handler_t * handler );
		void vs_close( xlog_handler_t * handler );
#endif

#if PLATFORM_ANDROID
		void log_android_message( xlog_handler_t * handler, const char * message, const char * filename, const char * function, int line, int type );
		int log_android_open( xlog_handler_t * handler );
		void log_android_close( xlog_handler_t * handler );
#endif
		
		core::Error open_log_handlers()
		{
			core::Error error( 0 );
			
			int total_log_handlers = 0;
						
			// setup system log
			xlog_init( &_system_log );
			
			xlog_set_default_log( &_system_log );
			
#if PLATFORM_WINDOWS
			xlog_handler_t msvc_logger;
			memset( &msvc_logger, 0, sizeof(xlog_handler_t) );
			msvc_logger.close = vs_close;
			msvc_logger.open = vs_open;
			msvc_logger.message = vs_message;
			msvc_logger.userdata = 0;

			++total_log_handlers;
			xlog_add_handler( &_system_log, &msvc_logger );
#endif

#ifndef PLATFORM_ANDROID
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
			xstr_ncpy( logdir, fs::content_directory(), -1 );
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


#if PLATFORM_ANDROID
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
				error = core::Error( core::Error::Warning, "Could not open one or more log handlers" );
			}
			
			LOGV( "Logging system initialized.\n" );
			
			return error;
		} // open_log_handlers
		
		
		void close_log_handlers()
		{
			xlog_close( &_system_log );
		} // close_log_handlers
	}; // namespace _internal
	
	
	
	Error::Error( int error_status, const char * error_message ) :
		status(error_status), message(error_message)
	{
	}
	

	
	Error startup()
	{
		core::Error error = platform::startup();
		if ( error.failed() )
		{
			fprintf( stderr, "platform startup failed! %s\n", error.message );
			return error;
		}
		
		//
		// setup our file system...
		StackString< MAX_PATH_SIZE > fullpath;
		error = platform::program_directory( &fullpath[0], fullpath.max_size() );
		if ( error.failed() )
		{
			fprintf( stderr, "failed to get the program directory!\n" );
			return error;
		}

		// set the startup directory: where the binary lives
		fs::root_directory( &fullpath[0], fullpath.max_size() );
		
		// set the content directory
		_internal::set_content_directory_from_root( fullpath );

		
		// open logs
		error = _internal::open_log_handlers();
		if ( error.failed() )
		{
			fprintf( stderr, "failed to open logging handlers: %s\n", error.message );
			return error;
		}
		
		LOGV( "setting root to '%s', content: '%s'\n", fs::root_directory(), fs::content_directory() );
		
		return error;
	} // startup
	
	void shutdown()
	{
		_internal::close_log_handlers();
		
		platform::shutdown();
	} // shutdown

}; // namespace core