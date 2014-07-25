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
#include <limits.h>
#include <map>

#include "kernel_desktop.h"
#include "input.h"
//#include "hashtable.h"
#include <slim/xlog.h>

using namespace input;

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
		
		input::state()->keyboard().inject_key_event( e->key, (e->type == XWLE_KEYPRESSED) );
		
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
		input::state()->mouse().inject_mouse_button( (input::MouseButton)e->button, (e->type == XWLE_MOUSEBUTTON_PRESSED) );
		
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
	typedef GeminiAllocator<std::pair<const unsigned int, input::Button> > ButtonKeyMapAllocator;
	typedef std::map<unsigned int, input::Button, std::less<unsigned int>, ButtonKeyMapAllocator> SDLToButtonKeyMap;
	SDLToButtonKeyMap _key_map;
	input::MouseButton _mouse_map[input::MOUSE_COUNT];

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
	while (SDL_PollEvent(&event))
	{
		input::Button button;
	
		// dispatch event!
		switch(event.type)
		{
			case SDL_QUIT:
				kernel::instance()->set_active(false);
				break;
			
			case SDL_TEXTINPUT:
			{
//				LOGV("TODO: add unicode support from SDL: %s\n", event.text.text);
				break;
			}
			
			case SDL_KEYUP:
			case SDL_KEYDOWN:
			{
				button = _key_map[event.key.keysym.sym];
				
				if (event.key.repeat)
				{
					break;
				}
				
				//printf( "\t-> key: %i (%s)\n", e->key, xwl_key_to_string(e->key) );
				kernel::KeyboardEvent ev;
				ev.is_down = (event.type == SDL_KEYDOWN);
				ev.key = button;
				input::state()->keyboard().inject_key_event(button, ev.is_down);
				kernel::event_dispatch(ev);
				break;
			}

			case SDL_MOUSEBUTTONUP:
			case SDL_MOUSEBUTTONDOWN:
			{
				kernel::MouseEvent ev;
				ev.subtype = kernel::MouseButton;
				ev.button = _mouse_map[event.button.button];
				ev.is_down = (event.type == SDL_MOUSEBUTTONDOWN);
				input::state()->mouse().inject_mouse_button((MouseButton)ev.button, ev.is_down);
				kernel::event_dispatch(ev);
				break;
			}
			
			case SDL_MOUSEMOTION:
			{
				kernel::MouseEvent ev;
				ev.subtype = kernel::MouseMoved;
				ev.dx = event.motion.xrel;
				ev.dy = event.motion.yrel;
				ev.mx = event.motion.x;
				ev.my = event.motion.y;
				input::state()->mouse().inject_mouse_move(ev.mx, ev.my);
				kernel::event_dispatch(ev);
				break;
			}
			
			case SDL_MOUSEWHEEL:
			{
				kernel::MouseEvent ev;
				ev.subtype = kernel::MouseWheelMoved;
				ev.wheel_direction = event.wheel.y;
				input::state()->mouse().inject_mouse_wheel(ev.wheel_direction);
				kernel::event_dispatch(ev);
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
	int window_width, window_height;
	int render_width, render_height;
	
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
		


		xwl_get_window_size(_window, &window_width, &window_height );
		xwl_get_window_render_size( _window, &render_width, &render_height );
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

		// fetch the window size
		SDL_GetWindowSize(_window, &window_width, &window_height);
		SDL_GL_GetDrawableSize(_window, &render_width, &render_height);
		
		
		// populate and set input key map
		_key_map[SDLK_a] = KEY_A;
		_key_map[SDLK_b] = KEY_B;
		_key_map[SDLK_c] = KEY_C;
		_key_map[SDLK_d] = KEY_D;
		_key_map[SDLK_e] = KEY_E;
		_key_map[SDLK_f] = KEY_F;
		_key_map[SDLK_g] = KEY_G;
		_key_map[SDLK_h] = KEY_H;
		_key_map[SDLK_i] = KEY_I;
		_key_map[SDLK_j] = KEY_J;
		_key_map[SDLK_k] = KEY_K;
		_key_map[SDLK_l] = KEY_L;
		_key_map[SDLK_m] = KEY_M;
		_key_map[SDLK_n] = KEY_N;
		_key_map[SDLK_o] = KEY_O;
		_key_map[SDLK_p] = KEY_P;
		_key_map[SDLK_q] = KEY_Q;
		_key_map[SDLK_r] = KEY_R;
		_key_map[SDLK_s] = KEY_S;
		_key_map[SDLK_t] = KEY_T;
		_key_map[SDLK_u] = KEY_U;
		_key_map[SDLK_v] = KEY_V;
		_key_map[SDLK_w] = KEY_W;
		_key_map[SDLK_y] = KEY_Y;
		_key_map[SDLK_x] = KEY_X;
		_key_map[SDLK_z] = KEY_Z;
		_key_map[SDLK_MENU] = KEY_MENU;
		_key_map[SDLK_SEMICOLON] = KEY_SEMICOLON;
		_key_map[SDLK_SLASH] = KEY_SLASH;
		_key_map[SDLK_BACKSLASH] = KEY_BACKSLASH;
		_key_map[SDLK_EQUALS] = KEY_EQUALS;
		_key_map[SDLK_MINUS] = KEY_MINUS;
		_key_map[SDLK_LEFTBRACKET] = KEY_LBRACKET;
		_key_map[SDLK_RIGHTBRACKET] = KEY_RBRACKET;
		_key_map[SDLK_COMMA] = KEY_COMMA;
		_key_map[SDLK_PERIOD] = KEY_PERIOD;
		_key_map[SDLK_QUOTE] = KEY_QUOTE;
		_key_map[SDLK_ESCAPE] = KEY_ESCAPE;
		_key_map[SDLK_SPACE] = KEY_SPACE;
		_key_map[SDLK_RETURN] = KEY_RETURN;
		_key_map[SDLK_BACKSPACE] = KEY_BACKSPACE;
		_key_map[SDLK_TAB] = KEY_TAB;
		_key_map[SDLK_PAGEUP] = KEY_PAGEUP;
		_key_map[SDLK_PAGEDOWN] = KEY_PAGEDN;
		_key_map[SDLK_END] = KEY_END;
		_key_map[SDLK_HOME] = KEY_HOME;
		_key_map[SDLK_INSERT] = KEY_INSERT;
		_key_map[SDLK_DELETE] = KEY_DELETE;
		_key_map[SDLK_PAUSE] = KEY_PAUSE;
		_key_map[SDLK_LSHIFT] = KEY_LSHIFT;
		_key_map[SDLK_RSHIFT] = KEY_RSHIFT;
		_key_map[SDLK_LCTRL] = KEY_LCONTROL;
		_key_map[SDLK_RCTRL] = KEY_RCONTROL;
		_key_map[SDLK_LALT] = KEY_LALT;
		_key_map[SDLK_RALT] = KEY_RALT;
		_key_map[SDLK_NUMLOCKCLEAR] = KEY_NUMLOCK;
		_key_map[SDLK_CAPSLOCK] = KEY_CAPSLOCK;
		_key_map[SDLK_LGUI] = KEY_LGUI;
		_key_map[SDLK_0] = KEY_0;
		_key_map[SDLK_1] = KEY_1;
		_key_map[SDLK_2] = KEY_2;
		_key_map[SDLK_3] = KEY_3;
		_key_map[SDLK_4] = KEY_4;
		_key_map[SDLK_5] = KEY_5;
		_key_map[SDLK_6] = KEY_6;
		_key_map[SDLK_7] = KEY_7;
		_key_map[SDLK_8] = KEY_8;
		_key_map[SDLK_9] = KEY_9;
		_key_map[SDLK_F1] = KEY_F1;
		_key_map[SDLK_F2] = KEY_F2;
		_key_map[SDLK_F3] = KEY_F3;
		_key_map[SDLK_F4] = KEY_F4;
		_key_map[SDLK_F5] = KEY_F5;
		_key_map[SDLK_F6] = KEY_F6;
		_key_map[SDLK_F7] = KEY_F7;
		_key_map[SDLK_F8] = KEY_F8;
		_key_map[SDLK_F9] = KEY_F9;
		_key_map[SDLK_F10] = KEY_F10;
		_key_map[SDLK_F11] = KEY_F11;
		_key_map[SDLK_F12] = KEY_F12;
		_key_map[SDLK_F13] = KEY_F13;
		_key_map[SDLK_F14] = KEY_F14;
		_key_map[SDLK_F15] = KEY_F15;
		_key_map[SDLK_LEFT] = KEY_LEFT;
		_key_map[SDLK_RIGHT] = KEY_RIGHT;
		_key_map[SDLK_UP] = KEY_UP;
		_key_map[SDLK_DOWN] = KEY_DOWN;
		_key_map[SDLK_KP_0] = KEY_NUMPAD0;
		_key_map[SDLK_KP_1] = KEY_NUMPAD1;
		_key_map[SDLK_KP_2] = KEY_NUMPAD2;
		_key_map[SDLK_KP_3] = KEY_NUMPAD3;
		_key_map[SDLK_KP_4] = KEY_NUMPAD4;
		_key_map[SDLK_KP_5] = KEY_NUMPAD5;
		_key_map[SDLK_KP_6] = KEY_NUMPAD6;
		_key_map[SDLK_KP_7] = KEY_NUMPAD7;
		_key_map[SDLK_KP_8] = KEY_NUMPAD8;
		_key_map[SDLK_KP_9] = KEY_NUMPAD9;
		_key_map[SDLK_KP_PLUS] = KEY_NUMPAD_PLUS;
		_key_map[SDLK_KP_MINUS] = KEY_NUMPAD_MINUS;
		_key_map[SDLK_KP_PLUSMINUS] = KEY_NUMPAD_PLUSMINUS;
		_key_map[SDLK_KP_MULTIPLY] = KEY_NUMPAD_MULTIPLY;
		_key_map[SDLK_KP_DIVIDE] = KEY_NUMPAD_DIVIDE;
		
		
		// populate the mouse map
		_mouse_map[SDL_BUTTON_LEFT] = MOUSE_LEFT;
		_mouse_map[SDL_BUTTON_RIGHT] = MOUSE_RIGHT;
		_mouse_map[SDL_BUTTON_MIDDLE] = MOUSE_MIDDLE;
		_mouse_map[SDL_BUTTON_X1] = MOUSE_MOUSE4;
		_mouse_map[SDL_BUTTON_X2] = MOUSE_MOUSE5;

#endif
		
		parameters().window_width = window_width;
		parameters().window_height = window_height;
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
#if GEMINI_USE_XWL
	xwl_shutdown();
	_window = 0;
#endif

#if GEMINI_USE_SDL2
	SDL_GL_DeleteContext(_context);
	SDL_DestroyWindow(_window);
	SDL_Quit();
	
	_key_map.clear();
#endif



} // shutdown


void DesktopKernel::capture_mouse(bool capture)
{
#if GEMINI_USE_SDL2
	SDL_bool is_enabled = capture ? SDL_TRUE : SDL_FALSE;
	SDL_SetRelativeMouseMode(is_enabled);
#elif GEMINI_USE_XWL
	
#else
	#error capture_mouse is not implemented for this configuration!
#endif
}