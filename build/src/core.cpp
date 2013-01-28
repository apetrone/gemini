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
#include "core.hpp"
#include "platform.hpp"
#include "memory.hpp"

#if !MOBILE_PLATFORM
	#include <xwl/xwl.h>
#endif

namespace core
{
	Error::Error( int error_status, const char * error_message ) :
		status(error_status), message(error_message)
	{
	}
	
	Error startup()
	{
		memory::startup();
		
		core::Error error = platform::startup();
		if ( error.failed() )
		{
			fprintf( stderr, "platform startup failed! %s\n", error.message );
			return error;
		}
		
		return error;
	} // startup
	
	void shutdown()
	{
		platform::shutdown();
		
		memory::shutdown();
	} // shutdown
	
	void beginFrame()
	{
#if !MOBILE_PLATFORM
		xwl_event_t e;
		memset( &e, 0, sizeof(xwl_event_t) );
		xwl_pollevent( &e );
#endif
	} // beginFrame
	
	void endFrame()
	{
#if !MOBILE_PLATFORM
		xwl_finish();
#endif
	} // endFrame
	
#if !MOBILE_PLATFORM
	core::Error createWindow( int width, int height, const char * title )
	{
		core::Error error(0);
		xwl_windowparams_t windowparams;
		windowparams.width = width;
		windowparams.height = height;
		windowparams.flags = XWL_OPENGL;
		unsigned int attribs[] = { XWL_GL_PROFILE, XWL_GLPROFILE_CORE3_2, 0 };
		
		xwl_window_t * window = xwl_create_window( &windowparams, title, attribs );
		if ( !window )
		{
			error = core::Error( core::Error::Failure, "Window creation failed" );
			return error;
		}
		
//		xwl_set_callback( event_callback_xwl );
		
		return error;
	} // createWindow
#endif
}; // namespace core