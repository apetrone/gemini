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


#include "kernel_desktop.h"

//#include "hashtable.h"
#include <slim/xlog.h>

using namespace input;

#include <SDL.h>

#ifdef main
#undef main
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


struct DesktopKernelState
{
	SDL_Window* window;
	SDL_GLContext context;
	SDL_Rect* display_rects;
	uint8_t total_displays;
	uint8_t total_controllers;

	typedef GeminiAllocator<std::pair<const unsigned int, input::Button> > ButtonKeyMapAllocator;
	typedef std::map<unsigned int, input::Button, std::less<unsigned int>, ButtonKeyMapAllocator> SDLToButtonKeyMap;
	SDLToButtonKeyMap key_map;

	input::MouseButton mouse_map[input::MOUSE_COUNT];
	SDL_GameController* controllers[input::MAX_JOYSTICKS];

	DesktopKernelState() : window(0), total_displays(0), total_controllers(0)
	{
		memset(mouse_map, 0, sizeof(input::MouseButton)*input::MOUSE_COUNT);
		memset(controllers, 0, sizeof(SDL_GameController*)*input::MAX_JOYSTICKS);
	}
};



void controller_axis_event(SDL_ControllerDeviceEvent& device, SDL_ControllerAxisEvent& axis)
{
	LOGV("Axis Motion: %i, %i, %i, %i\n", device.which, axis.which, axis.axis, axis.value);
}

void controller_button_event(SDL_ControllerDeviceEvent& device, SDL_ControllerButtonEvent& button)
{
	bool is_down = (button.state == SDL_PRESSED);
	LOGV("Button %s: %i, %i, %i\n", (is_down ? "Yes" : "No"), device.which, button.button, button.state);
}

DesktopKernel::DesktopKernel( int argc, char ** argv ) : target_renderer(0)
{
	params.argc = argc;
	params.argv = argv;


}

void DesktopKernel::startup()
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC) == -1)
	{
		// failure!
		fprintf(stdout, "failure to init SDL\n");
	}
	
	state = CREATE(DesktopKernelState);

	state->total_displays = SDL_GetNumVideoDisplays();
	fprintf(stdout, "Found %i total displays.\n", state->total_displays);
	
	state->display_rects = CREATE_ARRAY(SDL_Rect, state->total_displays);
	for (int index = 0; index < state->total_displays; ++index)
	{
		SDL_DisplayMode current;
		SDL_GetCurrentDisplayMode(index, &current);
		fprintf(stdout, "%i) width: %i, height: %i, refresh_rate: %iHz\n", index, current.w, current.h, current.refresh_rate);
		
		// cache display bounds
		SDL_GetDisplayBounds(index, &state->display_rects[index]);
	}
		

	if (this->parameters().use_vsync)
	{
		SDL_GL_SetSwapInterval(1);
	}
	
	this->parameters().device_flags |= kernel::DeviceDesktop;
	
	
	fprintf(stdout, "Gather joystick infos...\n");
	fprintf(stdout, "Num Haptics: %i\n", SDL_NumHaptics());
	fprintf(stdout, "Num Joysticks: %i\n", SDL_NumJoysticks());
	
	// add game controller mappings?
//	SDL_GameControllerAddMappingsFromFile(<#file#>)
	
	assert(SDL_NumJoysticks() < input::MAX_JOYSTICKS);
	state->total_controllers = SDL_NumJoysticks();
	for (uint8_t i = 0; i < state->total_controllers; ++i)
	{
		input::JoystickInput& js = input::state()->joystick(i);
		input::state()->connect_joystick(i);
		js.reset();
		
		state->controllers[i] = SDL_GameControllerOpen(i);
		SDL_Joystick * joystick = SDL_GameControllerGetJoystick(state->controllers[i]);
		SDL_JoystickID joystickID = SDL_JoystickInstanceID( joystick );
		if (SDL_JoystickIsHaptic(joystick))
		{
			js.flags |= input::JoystickInput::HapticSupport;
			fprintf(stdout, "Joystick is haptic!\n");
//			http://blog.5pmcasual.com/game-controller-api-in-sdl2.html
			SDL_Haptic * haptic = SDL_HapticOpenFromJoystick( joystick );
			if (haptic)
			{
				SDL_HapticRumbleInit(haptic);
//				SDL_HapticRumblePlay(haptic, 1.0, 2000);
				
//				SDL_Delay(2000);
				SDL_HapticClose(haptic);
			}
			else
			{
				fprintf(stdout, "error opening haptic for joystickID: %i\n", joystickID);
			}
		}
	}
} // startup

void DesktopKernel::register_services()
{
} // register_services

void DesktopKernel::pre_tick()
{
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
				button = state->key_map[event.key.keysym.sym];
				
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
				ev.button = state->mouse_map[event.button.button];
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
						
			case SDL_CONTROLLERAXISMOTION:
			{
				controller_axis_event(event.cdevice, event.caxis);
				
				kernel::GameControllerEvent ev;
				ev.subtype = kernel::JoystickAxisMoved;
				kernel::event_dispatch(ev);
				break;
			}
				
			case SDL_CONTROLLERBUTTONDOWN:
			{
				controller_button_event(event.cdevice, event.cbutton);
				
				kernel::GameControllerEvent ev;
				ev.subtype = kernel::JoystickButton;
				ev.is_down = true;
				ev.button = event.cbutton.button;
				kernel::event_dispatch(ev);
				break;
			}
				
			case SDL_CONTROLLERBUTTONUP:
			{
				controller_button_event(event.cdevice, event.cbutton);
				
				kernel::GameControllerEvent ev;
				ev.subtype = kernel::JoystickButton;
				ev.is_down = false;
				ev.button = event.cbutton.button;
				kernel::event_dispatch(ev);
				break;
			}
			
			case SDL_CONTROLLERDEVICEADDED:
			{
				// event 'which' member
				// describes an index into the list of active devices; NOT joystick id.
				LOGV("Device Added: %i\n", event.cdevice.which);

				input::JoystickInput& js = input::state()->joystick(event.cdevice.which);
				input::state()->connect_joystick(event.cdevice.which);
				js.reset();

				state->controllers[event.cdevice.which] = SDL_GameControllerOpen(event.cdevice.which);
				SDL_Joystick * joystick = SDL_GameControllerGetJoystick(state->controllers[event.cdevice.which]);

				
				kernel::GameControllerEvent ev;
				ev.subtype = kernel::JoystickConnected;
				kernel::event_dispatch(ev);
				break;
			}
				
			case SDL_CONTROLLERDEVICEREMOVED:
			{
				LOGV("Device Removed: %i\n", event.cdevice.which);

				input::state()->disconnect_joystick(event.cdevice.which);

				SDL_GameControllerClose(state->controllers[event.cdevice.which]);
				state->controllers[event.cdevice.which] = 0;


				kernel::GameControllerEvent ev;
				ev.subtype = kernel::JoystickDisconnected;
				kernel::event_dispatch(ev);
				break;
			}
		}
	}
} // pre_tick

void DesktopKernel::post_tick()
{
	// TODO: this needs to be controlled somehow
	// as the rift sdk performs buffer swaps during end frame.
	if (parameters().swap_buffers)
	{
		SDL_GL_SwapWindow(state->window);
	}
} // post_tick

void DesktopKernel::post_application_config( kernel::ApplicationResult result )
{
	int window_width, window_height;
	int render_width, render_height;
	
	set_active( (result != kernel::Application_NoWindow) );
	
	state->context = 0;

	if ( is_active() )
	{
		assert( parameters().window_width != 0 || parameters().window_height != 0 );

		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	
		uint32_t window_flags = 0;
		window_flags |= SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
		
		if (parameters().use_fullscreen)
		{
			window_flags |= SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS;
		}
		else
		{
			window_flags |= SDL_WINDOW_RESIZABLE;
		}
		
		state->window = SDL_CreateWindow(
			parameters().window_title, 0, 0,
			parameters().window_width, parameters().window_height,
			window_flags);
			
		if (!state->window)
		{
			LOGE("Failed to create SDL window: %s\n", SDL_GetError());
		}
		
		// move the window to the correct display
		SDL_SetWindowPosition(state->window, state->display_rects[params.target_display].x, state->display_rects[params.target_display].y);
		
		state->context = SDL_GL_CreateContext(state->window);
		if (!state->context)
		{
			LOGE("Failed to create SDL GL context: %s\n", SDL_GetError());
		}

		// try to set our window size; it might still be smaller than requested.
		SDL_SetWindowSize(state->window, parameters().window_width, parameters().window_height);
		
		// center our window
		SDL_SetWindowPosition(state->window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
		
		// fetch the window size and renderable size
		SDL_GetWindowSize(state->window, &window_width, &window_height);
		SDL_GL_GetDrawableSize(state->window, &render_width, &render_height);
		
		
		// populate and set input key map
		state->key_map[SDLK_a] = KEY_A;
		state->key_map[SDLK_b] = KEY_B;
		state->key_map[SDLK_c] = KEY_C;
		state->key_map[SDLK_d] = KEY_D;
		state->key_map[SDLK_e] = KEY_E;
		state->key_map[SDLK_f] = KEY_F;
		state->key_map[SDLK_g] = KEY_G;
		state->key_map[SDLK_h] = KEY_H;
		state->key_map[SDLK_i] = KEY_I;
		state->key_map[SDLK_j] = KEY_J;
		state->key_map[SDLK_k] = KEY_K;
		state->key_map[SDLK_l] = KEY_L;
		state->key_map[SDLK_m] = KEY_M;
		state->key_map[SDLK_n] = KEY_N;
		state->key_map[SDLK_o] = KEY_O;
		state->key_map[SDLK_p] = KEY_P;
		state->key_map[SDLK_q] = KEY_Q;
		state->key_map[SDLK_r] = KEY_R;
		state->key_map[SDLK_s] = KEY_S;
		state->key_map[SDLK_t] = KEY_T;
		state->key_map[SDLK_u] = KEY_U;
		state->key_map[SDLK_v] = KEY_V;
		state->key_map[SDLK_w] = KEY_W;
		state->key_map[SDLK_y] = KEY_Y;
		state->key_map[SDLK_x] = KEY_X;
		state->key_map[SDLK_z] = KEY_Z;
		state->key_map[SDLK_MENU] = KEY_MENU;
		state->key_map[SDLK_SEMICOLON] = KEY_SEMICOLON;
		state->key_map[SDLK_SLASH] = KEY_SLASH;
		state->key_map[SDLK_BACKSLASH] = KEY_BACKSLASH;
		state->key_map[SDLK_EQUALS] = KEY_EQUALS;
		state->key_map[SDLK_MINUS] = KEY_MINUS;
		state->key_map[SDLK_LEFTBRACKET] = KEY_LBRACKET;
		state->key_map[SDLK_RIGHTBRACKET] = KEY_RBRACKET;
		state->key_map[SDLK_COMMA] = KEY_COMMA;
		state->key_map[SDLK_PERIOD] = KEY_PERIOD;
		state->key_map[SDLK_QUOTE] = KEY_QUOTE;
		state->key_map[SDLK_ESCAPE] = KEY_ESCAPE;
		state->key_map[SDLK_SPACE] = KEY_SPACE;
		state->key_map[SDLK_RETURN] = KEY_RETURN;
		state->key_map[SDLK_BACKSPACE] = KEY_BACKSPACE;
		state->key_map[SDLK_TAB] = KEY_TAB;
		state->key_map[SDLK_PAGEUP] = KEY_PAGEUP;
		state->key_map[SDLK_PAGEDOWN] = KEY_PAGEDN;
		state->key_map[SDLK_END] = KEY_END;
		state->key_map[SDLK_HOME] = KEY_HOME;
		state->key_map[SDLK_INSERT] = KEY_INSERT;
		state->key_map[SDLK_DELETE] = KEY_DELETE;
		state->key_map[SDLK_PAUSE] = KEY_PAUSE;
		state->key_map[SDLK_LSHIFT] = KEY_LSHIFT;
		state->key_map[SDLK_RSHIFT] = KEY_RSHIFT;
		state->key_map[SDLK_LCTRL] = KEY_LCONTROL;
		state->key_map[SDLK_RCTRL] = KEY_RCONTROL;
		state->key_map[SDLK_LALT] = KEY_LALT;
		state->key_map[SDLK_RALT] = KEY_RALT;
		state->key_map[SDLK_NUMLOCKCLEAR] = KEY_NUMLOCK;
		state->key_map[SDLK_CAPSLOCK] = KEY_CAPSLOCK;
		state->key_map[SDLK_LGUI] = KEY_LGUI;
		state->key_map[SDLK_0] = KEY_0;
		state->key_map[SDLK_1] = KEY_1;
		state->key_map[SDLK_2] = KEY_2;
		state->key_map[SDLK_3] = KEY_3;
		state->key_map[SDLK_4] = KEY_4;
		state->key_map[SDLK_5] = KEY_5;
		state->key_map[SDLK_6] = KEY_6;
		state->key_map[SDLK_7] = KEY_7;
		state->key_map[SDLK_8] = KEY_8;
		state->key_map[SDLK_9] = KEY_9;
		state->key_map[SDLK_F1] = KEY_F1;
		state->key_map[SDLK_F2] = KEY_F2;
		state->key_map[SDLK_F3] = KEY_F3;
		state->key_map[SDLK_F4] = KEY_F4;
		state->key_map[SDLK_F5] = KEY_F5;
		state->key_map[SDLK_F6] = KEY_F6;
		state->key_map[SDLK_F7] = KEY_F7;
		state->key_map[SDLK_F8] = KEY_F8;
		state->key_map[SDLK_F9] = KEY_F9;
		state->key_map[SDLK_F10] = KEY_F10;
		state->key_map[SDLK_F11] = KEY_F11;
		state->key_map[SDLK_F12] = KEY_F12;
		state->key_map[SDLK_F13] = KEY_F13;
		state->key_map[SDLK_F14] = KEY_F14;
		state->key_map[SDLK_F15] = KEY_F15;
		state->key_map[SDLK_LEFT] = KEY_LEFT;
		state->key_map[SDLK_RIGHT] = KEY_RIGHT;
		state->key_map[SDLK_UP] = KEY_UP;
		state->key_map[SDLK_DOWN] = KEY_DOWN;
		state->key_map[SDLK_KP_0] = KEY_NUMPAD0;
		state->key_map[SDLK_KP_1] = KEY_NUMPAD1;
		state->key_map[SDLK_KP_2] = KEY_NUMPAD2;
		state->key_map[SDLK_KP_3] = KEY_NUMPAD3;
		state->key_map[SDLK_KP_4] = KEY_NUMPAD4;
		state->key_map[SDLK_KP_5] = KEY_NUMPAD5;
		state->key_map[SDLK_KP_6] = KEY_NUMPAD6;
		state->key_map[SDLK_KP_7] = KEY_NUMPAD7;
		state->key_map[SDLK_KP_8] = KEY_NUMPAD8;
		state->key_map[SDLK_KP_9] = KEY_NUMPAD9;
		state->key_map[SDLK_KP_PLUS] = KEY_NUMPAD_PLUS;
		state->key_map[SDLK_KP_MINUS] = KEY_NUMPAD_MINUS;
		state->key_map[SDLK_KP_PLUSMINUS] = KEY_NUMPAD_PLUSMINUS;
		state->key_map[SDLK_KP_MULTIPLY] = KEY_NUMPAD_MULTIPLY;
		state->key_map[SDLK_KP_DIVIDE] = KEY_NUMPAD_DIVIDE;
		
		
		// populate the mouse map
		state->mouse_map[SDL_BUTTON_LEFT] = MOUSE_LEFT;
		state->mouse_map[SDL_BUTTON_RIGHT] = MOUSE_RIGHT;
		state->mouse_map[SDL_BUTTON_MIDDLE] = MOUSE_MIDDLE;
		state->mouse_map[SDL_BUTTON_X1] = MOUSE_MOUSE4;
		state->mouse_map[SDL_BUTTON_X2] = MOUSE_MOUSE5;


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
	DESTROY_ARRAY(SDL_Rect, state->display_rects, state->total_displays);

	// close all controllers
	for (uint8_t i = 0; i < state->total_controllers; ++i)
	{
		input::JoystickInput& js = input::state()->joystick(i);
		input::state()->disconnect_joystick(i);
		
		SDL_GameController* controller = state->controllers[i];
		if (controller)
		{
			SDL_GameControllerClose(controller);
			state->controllers[i] = 0;
		}
	}

	SDL_GL_DeleteContext(state->context);
	SDL_DestroyWindow(state->window);
	SDL_Quit();
	
	state->key_map.clear();

	DESTROY(DesktopKernelState, state);

} // shutdown


void DesktopKernel::capture_mouse(bool capture)
{
	SDL_bool is_enabled = capture ? SDL_TRUE : SDL_FALSE;
	SDL_SetRelativeMouseMode(is_enabled);
}