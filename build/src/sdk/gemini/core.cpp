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
// TODO: remove old deps
//#include <slim/xlog.h>
//#include <slim/xtime.h>

#include <gemini/platform.h>
#include <gemini/util/stackstring.h>


namespace core
{
	namespace _internal
	{
		#define GEMINI_LOG_PATH "logs"
		const unsigned int GEMINI_DATETIME_STRING_MAX = 128;
		
		void set_content_directory_from_root( StackString<MAX_PATH_SIZE> & root )
		{
#if !TARGET_OS_IPHONE
//			fs::truncate_string_at_path( &root[0], "bin" );
#endif
//			fs::content_directory( &root[0], root.max_size() );
		}
				
		core::Error open_log_handlers()
		{
			core::Error error( 0 );
			
			return error;
		} // open_log_handlers
		
		
		void close_log_handlers()
		{
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
		//fs::root_directory( &fullpath[0], fullpath.max_size() );
		
		// set the content directory
		_internal::set_content_directory_from_root( fullpath );

		
		// open logs
		error = _internal::open_log_handlers();
		if ( error.failed() )
		{
			fprintf( stderr, "failed to open logging handlers: %s\n", error.message );
			return error;
		}
		
		//LOGV("TODO: set content directory!\n");
		//LOGV( "setting root to '%s', content: '%s'\n", fs::root_directory(), fs::content_directory() );
		
		return error;
	} // startup
	
	void shutdown()
	{
		_internal::close_log_handlers();
		
		platform::shutdown();
	} // shutdown

}; // namespace core