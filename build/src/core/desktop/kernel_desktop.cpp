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
#include "input.hpp"
#include "log.h"

static xwl_window_t * _window = 0;
namespace kernel
{
	Error main( IKernel * kernel_instance, const char * kernel_name )
	{
		// attempt kernel startup, mostly initializing core systems
		Error error = startup( kernel_instance );
		if ( error != kernel::NoError )
		{
			fprintf( stderr, "Kernel startup failed with kernel code: %i\n", error );
			shutdown();
			return kernel::StartupFailed;
		}
		else
		{			
			// startup succeeded; enter main loop
			while( kernel::instance()->is_active() )
			{
				tick();
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
		
		input::state()->keyboard().inject_key_event( e->key, (e->type == XWLE_KEYPRESSED), e->unicode );
		
		//printf( "\t-> key: %i (%s)\n", e->key, xwl_key_to_string(e->key) );		
		kernel::KeyboardEvent ev;
		ev.is_down = (e->type == XWLE_KEYPRESSED);
		ev.key = e->key;
		ev.unicode = e->unicode;
		kernel::event_dispatch( ev );
	}
	else if ( e->type == XWLE_MOUSEMOVE )
	{
		input::state()->mouse().inject_mouse_move( e->mx, e->my );
		
		kernel::MouseEvent ev;
		ev.subtype = kernel::MouseMoved;
		ev.mx = e->mx;
		ev.my = e->my;
		kernel::event_dispatch( ev );
	}
	else if ( e->type == XWLE_MOUSEBUTTON_PRESSED || e->type == XWLE_MOUSEBUTTON_RELEASED )
	{
		input::state()->mouse().inject_mouse_button( e->button, (e->type == XWLE_MOUSEBUTTON_PRESSED) );
		
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
		input::state()->mouse().inject_mouse_wheel( e->wheelDelta );
		
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

void DesktopKernel::startup()
{
	xwl_startup( XWL_WINDOW_PROVIDER_DEFAULT, XWL_API_PROVIDER_DEFAULT, XWL_INPUT_PROVIDER_DEFAULT );

	this->parameters().device_flags |= kernel::DeviceDesktop;
} // startup

void DesktopKernel::register_services()
{
} // register_services

void DesktopKernel::pre_tick()
{
	xwl_dispatch_events();
} // pre_tick

void DesktopKernel::post_tick()
{
	xwl_swap_buffers( _window );
} // post_tick

void DesktopKernel::post_application_config( kernel::ApplicationResult result )
{
	set_active( (result != kernel::Application_NoWindow) );
	
	if ( is_active() )
	{
		unsigned int attribs[] = {

#if defined( PLATFORM_USE_GLES2 )
			XWL_API, XWL_API_GLES2,
			XWL_API_MAJOR_VERSION, 2,
			XWL_API_MINOR_VERSION, 0,
#else
			XWL_API, XWL_API_OPENGL,
			XWL_API_MAJOR_VERSION, 3,
			XWL_API_MINOR_VERSION, 2,
#endif


			XWL_WINDOW_WIDTH, parameters().window_width,
			XWL_WINDOW_HEIGHT, parameters().window_height,
//			XWL_DEPTH_SIZE, 24,
//			XWL_STENCIL_SIZE, 8,
//			XWL_USE_FULLSCREEN, 1,
			XWL_NONE,
		};
		
		_window = xwl_create_window( parameters().window_title, attribs );
		if ( !_window )
		{
			fprintf( stderr, "Window creation failed\n" );
		}
	
		xwl_set_callback( event_callback_xwl );
		
		int window_width, window_height;
		int render_width, render_height;

		xwl_get_window_size(_window, &window_width, &window_height );
		parameters().window_width = window_width;
		parameters().window_height = window_height;
		
		xwl_get_window_render_size( _window, &render_width, &render_height );
		parameters().render_width = render_width;
		parameters().render_height = render_height;

		if ( render_width > window_width && render_height > window_height )
		{
			LOGV( "Retina display detected. Render Resolution is (%i x %i)\n", render_width, render_height );
			this->parameters().device_flags |= kernel::DeviceSupportsRetinaDisplay;
		}
		else
		{
			LOGV( "window resolution %i x %i\n", window_width, window_height );
			LOGV( "render resolution %i x %i\n", render_width, render_height );	
		}
	}
} // post_application_config

void DesktopKernel::post_application_startup( kernel::ApplicationResult result )
{
} // post_application_startup


void DesktopKernel::shutdown()
{
	xwl_shutdown();
	_window = 0;
} // shutdown