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
#include "log.h"
#include "xstr.h" // for xstr_filefrompath
#include <stdio.h> // for FILE, file functions

namespace core
{
	// log handler function definitions
	namespace _internal
	{
		void file_logger_message( log_handler_t * handler, const char * message, const char * filename, const char * function, int line, int type )
		{
			fprintf( (FILE*)handler->userdata, "[%i %s %s %i] %s", type, xstr_filefrompath(filename), function, line, message );
			//fprintf( (FILE*)handler->userdata, "\t%s", message );
			fflush( (FILE*)handler->userdata );
		}
		
		int file_logger_open( log_handler_t * handler )
		{
			const char * logname = (const char*)handler->userdata;
			handler->userdata = fopen( logname, "wb" );
			return handler->userdata != 0;
		}
		
		void file_logger_close( log_handler_t * handler )
		{
			if ( handler->userdata )
			{
				fclose( (FILE*)handler->userdata );
			}
		}
		
		
		void stdout_message( log_handler_t * handler, const char * message, const char * filename, const char * function, int line, int type )
		{
			fprintf( stdout, "[%i] - %s, %s, %i | %s", type, xstr_filefrompath(filename), function, line, message );
			//fflush( stdout );
		}
		
		int stdout_open( log_handler_t * handler )
		{
			return 1;
		}
		
		void stdout_close( log_handler_t * handler )
		{
		}
	}; // namespace _internal
}; // namespace core