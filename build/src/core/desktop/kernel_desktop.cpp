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
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "kernel_desktop.hpp"
#include "input.hpp"
#include <slim/xlog.h>

#if GEMINI_USE_XWL
	#include <xwl/xwl.h>
#endif

#if GEMINI_USE_SDL2
	#include <SDL.h>
#endif

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


#if GEMINI_USE_XWL
static xwl_window_t * _window = 0;

void event_callback_xwl( xwl_event_t * e )
{
//	LOGV( "event_callback_xwl - event->type = %i\n", e->type );

	if ( e->type == XWLE_KEYRELEASED || e->type == XWLE_KEYPRESSED )
	{
#if 0
		if ( e->type == XWLE_KEYRELEASED && e->key == XWLK_ESCAPE )
		{
			kernel::instance()->set_active( false );
		}
#endif
		
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
#endif



#if GEMINI_USE_SDL2
	SDL_Window * _window = 0;
	SDL_GLContext _context = 0;
#endif



DesktopKernel::DesktopKernel( int argc, char ** argv ) : target_renderer(0)
{
	params.argc = argc;
	params.argv = argv;
}

void DesktopKernel::startup()
{
#if GEMINI_USE_XWL
	xwl_startup( XWL_WINDOW_PROVIDER_DEFAULT, XWL_API_PROVIDER_DEFAULT, XWL_INPUT_PROVIDER_DEFAULT );
#endif

#if GEMINI_USE_SDL2
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) == -1)
	{
		// failure!
		LOGV("failure to init SDL\n");
	}
#endif

	this->parameters().device_flags |= kernel::DeviceDesktop;
} // startup

void DesktopKernel::register_services()
{
} // register_services

void DesktopKernel::pre_tick()
{
#if GEMINI_USE_XWL
	xwl_dispatch_events();
#endif

#if GEMINI_USE_SDL2
	SDL_Event event;
	if (SDL_PollEvent(&event))
	{
		// dispatch event!
		switch(event.type)
		{
			case SDL_QUIT:
				kernel::instance()->set_active(false);
				break;
				
			case SDL_KEYUP:
			{
				if (event.key.keysym.sym == SDLK_ESCAPE)
				{
					kernel::instance()->set_active(false);
				}
				break;
			}
		}
	}
#endif
} // pre_tick

void DesktopKernel::post_tick()
{
#if GEMINI_USE_XWL
	xwl_swap_buffers( _window );
#endif

#if GEMINI_USE_SDL2
	SDL_GL_SwapWindow(_window);
#endif
} // post_tick

void DesktopKernel::post_application_config( kernel::ApplicationResult result )
{
	set_active( (result != kernel::Application_NoWindow) );

	if ( is_active() )
	{
		assert( parameters().window_width != 0 || parameters().window_height != 0 );
		
		
		
#if GEMINI_USE_XWL
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
#endif // GEMINI_USE_XWL


#if GEMINI_USE_SDL2

		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	

		_window = SDL_CreateWindow(
			parameters().window_title, 0, 0,
			parameters().window_width, parameters().window_height,
			SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
			
		if (!_window)
		{
			LOGE("Failed to create SDL window: %s\n", SDL_GetError());
		}

		_context = SDL_GL_CreateContext(_window);
		if (!_context)
		{
			LOGE("Failed to create SDL GL context: %s\n", SDL_GetError());
		}

#endif

	}
} // post_application_config

void DesktopKernel::post_application_startup( kernel::ApplicationResult result )
{
} // post_application_startup


void DesktopKernel::shutdown()
{
#if GEMINI_USE_XWL
	xwl_shutdown();
	_window = 0;
#endif

#if GEMINI_USE_SDL2
	SDL_GL_DeleteContext(_context);
	SDL_DestroyWindow(_window);
	SDL_Quit();
#endif
} // shutdown