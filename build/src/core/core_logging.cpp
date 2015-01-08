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
#include <slim/xlog.h>
#include <slim/xstr.h> // for xstr_filefrompath
#include <stdio.h> // for FILE, file functions

#if __ANDROID__
	#include <android/log.h>
#endif


#include "logging.h"

using namespace gemini::core::logging;

namespace core
{
	// log handler function definitions
	namespace _internal
	{
		void file_logger_message( xlog_handler_t * handler, const char * message, const char * filename, const char * function, int line, int type )
		{
			fprintf( (FILE*)handler->userdata, "[%i %s %s %i] %s", type, xstr_filefrompath(filename), function, line, message );
			//fprintf( (FILE*)handler->userdata, "\t%s", message );
			fflush( (FILE*)handler->userdata );
		}
		
		int file_logger_open( xlog_handler_t * handler )
		{
			const char * logname = (const char*)handler->userdata;
			handler->userdata = fopen( logname, "wb" );
			return handler->userdata != 0;
		}
		
		void file_logger_close( xlog_handler_t * handler )
		{
			if ( handler->userdata )
			{
				fclose( (FILE*)handler->userdata );
			}
		}
		
		
		void stdout_message(Handler* handler, const char* message, const char* filename, const char* function, int line, int type)
		{
			const char *message_types[] = {0, "VERBOSE", "WARNING", " ERROR "};
			fprintf(stdout, "[%s] %s, %s, %i | %s", message_types[type], xstr_filefrompath(filename), function, line, message);
			//fflush( stdout );
		}
		
		int stdout_open(Handler* handler)
		{
			return 1;
		}
		
		void stdout_close(Handler* handler)
		{
		}

#if _WIN32
		void vs_message( xlog_handler_t * handler, const char * message, const char * filename, const char * function, int line, int type )
		{
	//		const char *message_types[] = { 0, "VERBOSE", "WARNING", " ERROR " };
//			fprintf( stdout, "[%s] %s, %s, %i | %s", message_types[ type ], xstr_filefrompath(filename), function, line, message );


			OutputDebugStringA( message );
		}
		
		int vs_open( xlog_handler_t * handler )
		{
			return 1;
		}
		
		void vs_close( xlog_handler_t * handler )
		{
		}	
#endif
		
#if __ANDROID__
		void log_android_message( xlog_handler_t * handler, const char * message, const char * filename, const char * function, int line, int type )
		{
			// this must match with the android_LogPriority enum in <android/log.h>
			android_LogPriority message_types[] = { ANDROID_LOG_UNKNOWN, ANDROID_LOG_VERBOSE, ANDROID_LOG_WARN, ANDROID_LOG_ERROR };
			__android_log_print( message_types[ type ], "gemini", message );
		}
		
		int log_android_open( xlog_handler_t * handler )
		{
			return 1;
		}
		
		void log_android_close( xlog_handler_t * handler )
		{
		}
#endif
	}; // namespace _internal
}; // namespace core