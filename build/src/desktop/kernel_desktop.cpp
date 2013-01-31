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
#include "kernel_desktop.hpp"
#include <xwl/xwl.h>
#include <string.h>
#include <stdio.h>


namespace kernel
{
	Error main( IKernel * kernel_instance, const char * kernel_name )
	{
		// attempt kernel startup, mostly initializing core systems
		Error error = startup( kernel_instance, kernel_name );
		if ( error != kernel::NoError )
		{
			fprintf( stderr, "Kernel startup failed with kernel code: %i\n", error );
			return kernel::StartupFailed;
		}
		else
		{
			// start kernel disabled.
			kernel::instance()->set_active( false );
			
			// startup succeeded; enter main loop if we have a window
			if ( kernel::instance()->parameters().has_window )
			{
				kernel::instance()->set_active( true );
				
				// main loop, kernels can modify is_active.
				while( kernel::instance()->is_active() )
				{
					tick();
				}
			}
		}
		
		// cleanup kernel memory
		shutdown();
		
		return error;
	} // main
}; // namespace kernel



void event_callback_xwl( xwl_event_t * e )
{
	if ( e->type == XWLE_KEYRELEASED || e->type == XWLE_KEYPRESSED )
	{
		if ( e->type == XWLE_KEYRELEASED && e->key == XWLK_ESCAPE )
		{
			kernel::instance()->set_active( false );
		}
		//printf( "\t-> key: %i (%s)\n", e->key, xwl_key_to_string(e->key) );		
		kernel::KeyboardEvent ev;
		ev.is_down = (e->type == XWLE_KEYPRESSED);
		ev.key = e->key;
		ev.unicode = e->unicode;
		kernel::event_dispatch( ev );
	}
	else if ( e->type == XWLE_MOUSEMOVE )
	{
		kernel::MouseEvent ev;
		ev.subtype = kernel::MouseMoved;
		ev.mx = e->mx;
		ev.my = e->my;
		kernel::event_dispatch( ev );
	}
	else if ( e->type == XWLE_MOUSEBUTTON_PRESSED || e->type == XWLE_MOUSEBUTTON_RELEASED )
	{
		kernel::MouseEvent ev;
		ev.subtype = kernel::MouseButton;
		ev.button = e->button;
		ev.is_down = (e->type == XWLE_MOUSEBUTTON_PRESSED);
		ev.mx = e->mx;
		ev.my = e->my;
		kernel::event_dispatch( ev );
	}
	else if ( e->type == XWLE_MOUSEWHEEL )
	{
		kernel::MouseEvent ev;
		ev.subtype = kernel::MouseWheelMoved;
		ev.mx = e->mx;
		ev.my = e->my;
		ev.wheel_direction = e->wheelDelta;
		kernel::event_dispatch( ev );
	}
	else if ( e->type == XWLE_SIZE )
	{
		kernel::SystemEvent ev;
		ev.subtype = kernel::WindowResized;
		ev.window_width = e->width;
		ev.window_height = e->height;
		kernel::event_dispatch( ev );
	}
	else if ( e->type == XWLE_GAINFOCUS || e->type == XWLE_LOSTFOCUS )
	{
		kernel::SystemEvent ev;
		ev.subtype = (e->type == XWLE_GAINFOCUS) ? kernel::WindowGainFocus : kernel::WindowLostFocus;
		ev.window_width = e->width;
		ev.window_height = e->height;
		kernel::event_dispatch( ev );
	}
} // event_callback_xwl

DesktopKernel::DesktopKernel( int argc, char ** argv ) : target_renderer(0)
{
	params.argc = argc;
	params.argv = argv;
}

void DesktopKernel::register_services()
{
	
}

void DesktopKernel::pre_tick()
{
	xwl_event_t e;
	memset( &e, 0, sizeof(xwl_event_t) );
	xwl_pollevent( &e );
} // pre_tick

void DesktopKernel::post_tick()
{
	xwl_finish();
} // post_tick

kernel::Error DesktopKernel::post_application_config()
{
	xwl_windowparams_t windowparams;
	windowparams.width = parameters().window_width;
	windowparams.height = parameters().window_height;
	windowparams.flags = XWL_OPENGL;
	
	// pick the core 3.2 profile if we can
	unsigned int attribs[] = { XWL_GL_PROFILE, XWL_GLPROFILE_CORE3_2, 0 };
	
	xwl_window_t * window = this->create_window( &windowparams, parameters().window_title, attribs );
	if ( !window )
	{		
		fprintf( stderr, "Window creation failed\n" );
		return kernel::PostConfig;
	}
	
	xwl_set_callback( event_callback_xwl );
	
	return kernel::NoError;
} // post_application_config

struct xwl_window_s *DesktopKernel::create_window( struct xwl_windowparams_s * windowparams, const char * title, unsigned int * attribs )
{
	xwl_window_t * window = 0;
	window = xwl_create_window( windowparams, title, attribs );
	if ( !window )
	{
		// try to scale back to the legacy profile
		attribs[1] = XWL_GLPROFILE_LEGACY;
		
		window = xwl_create_window( windowparams, title, attribs );
		if ( !window )
		{
			return 0;
		}
		
		// TODO: this was successful, set the target_renderer to some constant
		target_renderer = 1;
	}
	
	return window;
} // create_window
