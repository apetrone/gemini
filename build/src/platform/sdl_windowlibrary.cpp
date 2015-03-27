// -------------------------------------------------------------
// Copyright (C) 2015- Adam Petrone
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//		* Redistributions of source code must retain the above copyright notice,
//		this list of conditions and the following disclaimer.

//		* Redistributions in binary form must reproduce the above copyright notice,
//		this list of conditions and the following disclaimer in the documentation
//		and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//		 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------

#include "mem.h"
#include "sdl_windowlibrary.h"
#include "kernel.h"

#include <assert.h>

using namespace input;

namespace platform
{
	struct SDLWindow : public NativeWindow
	{
		SDL_Window* window;
		SDL_GLContext context;
		
		SDLWindow() : window(0)
		{
		}
	};

	SDLWindowLibrary::SDLWindowLibrary() :
	display_rects(0),
	total_displays(0),
	total_controllers(0)
	{
		memset(mouse_map, 0, sizeof(input::MouseButton)*input::MOUSE_COUNT);
		memset(controllers, 0, sizeof(SDL_GameController*)*input::MAX_JOYSTICKS);
	}
	
	void SDLWindowLibrary::startup(kernel::Parameters& parameters)
	{
		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC) == -1)
		{
			// failure!
			fprintf(stdout, "failure to init SDL\n");
		}
		
		total_displays = SDL_GetNumVideoDisplays();
		fprintf(stdout, "Found %i total displays.\n", total_displays);
		
		display_rects = CREATE_ARRAY(SDL_Rect, total_displays);
		for (int index = 0; index < total_displays; ++index)
		{
			SDL_DisplayMode current;
			SDL_GetCurrentDisplayMode(index, &current);
			fprintf(stdout, "%i) width: %i, height: %i, refresh_rate: %iHz\n", index, current.w, current.h, current.refresh_rate);
			
			// cache display bounds
			SDL_GetDisplayBounds(index, &display_rects[index]);
		}
		
		if (parameters.use_vsync)
		{
			SDL_GL_SetSwapInterval(1);
		}
		
		//
		// setup input mappings
		populate_input_map();
		
		setup_joysticks();
	}
	
	void SDLWindowLibrary::populate_input_map()
	{
		// populate the keyboard map
		
		key_map[SDLK_a] = KEY_A;
		key_map[SDLK_b] = KEY_B;
		key_map[SDLK_c] = KEY_C;
		key_map[SDLK_d] = KEY_D;
		key_map[SDLK_e] = KEY_E;
		key_map[SDLK_f] = KEY_F;
		key_map[SDLK_g] = KEY_G;
		key_map[SDLK_h] = KEY_H;
		key_map[SDLK_i] = KEY_I;
		key_map[SDLK_j] = KEY_J;
		key_map[SDLK_k] = KEY_K;
		key_map[SDLK_l] = KEY_L;
		key_map[SDLK_m] = KEY_M;
		key_map[SDLK_n] = KEY_N;
		key_map[SDLK_o] = KEY_O;
		key_map[SDLK_p] = KEY_P;
		key_map[SDLK_q] = KEY_Q;
		key_map[SDLK_r] = KEY_R;
		key_map[SDLK_s] = KEY_S;
		key_map[SDLK_t] = KEY_T;
		key_map[SDLK_u] = KEY_U;
		key_map[SDLK_v] = KEY_V;
		key_map[SDLK_w] = KEY_W;
		key_map[SDLK_y] = KEY_Y;
		key_map[SDLK_x] = KEY_X;
		key_map[SDLK_z] = KEY_Z;
		key_map[SDLK_MENU] = KEY_MENU;
		key_map[SDLK_SEMICOLON] = KEY_SEMICOLON;
		key_map[SDLK_SLASH] = KEY_SLASH;
		key_map[SDLK_BACKSLASH] = KEY_BACKSLASH;
		key_map[SDLK_EQUALS] = KEY_EQUALS;
		key_map[SDLK_MINUS] = KEY_MINUS;
		key_map[SDLK_LEFTBRACKET] = KEY_LBRACKET;
		key_map[SDLK_RIGHTBRACKET] = KEY_RBRACKET;
		key_map[SDLK_COMMA] = KEY_COMMA;
		key_map[SDLK_PERIOD] = KEY_PERIOD;
		key_map[SDLK_QUOTE] = KEY_QUOTE;
		key_map[SDLK_ESCAPE] = KEY_ESCAPE;
		key_map[SDLK_SPACE] = KEY_SPACE;
		key_map[SDLK_RETURN] = KEY_RETURN;
		key_map[SDLK_BACKSPACE] = KEY_BACKSPACE;
		key_map[SDLK_TAB] = KEY_TAB;
		key_map[SDLK_PAGEUP] = KEY_PAGEUP;
		key_map[SDLK_PAGEDOWN] = KEY_PAGEDN;
		key_map[SDLK_END] = KEY_END;
		key_map[SDLK_HOME] = KEY_HOME;
		key_map[SDLK_INSERT] = KEY_INSERT;
		key_map[SDLK_DELETE] = KEY_DELETE;
		key_map[SDLK_PAUSE] = KEY_PAUSE;
		key_map[SDLK_LSHIFT] = KEY_LSHIFT;
		key_map[SDLK_RSHIFT] = KEY_RSHIFT;
		key_map[SDLK_LCTRL] = KEY_LCONTROL;
		key_map[SDLK_RCTRL] = KEY_RCONTROL;
		key_map[SDLK_LALT] = KEY_LALT;
		key_map[SDLK_RALT] = KEY_RALT;
		key_map[SDLK_NUMLOCKCLEAR] = KEY_NUMLOCK;
		key_map[SDLK_CAPSLOCK] = KEY_CAPSLOCK;
		key_map[SDLK_LGUI] = KEY_LGUI;
		key_map[SDLK_0] = KEY_0;
		key_map[SDLK_1] = KEY_1;
		key_map[SDLK_2] = KEY_2;
		key_map[SDLK_3] = KEY_3;
		key_map[SDLK_4] = KEY_4;
		key_map[SDLK_5] = KEY_5;
		key_map[SDLK_6] = KEY_6;
		key_map[SDLK_7] = KEY_7;
		key_map[SDLK_8] = KEY_8;
		key_map[SDLK_9] = KEY_9;
		key_map[SDLK_F1] = KEY_F1;
		key_map[SDLK_F2] = KEY_F2;
		key_map[SDLK_F3] = KEY_F3;
		key_map[SDLK_F4] = KEY_F4;
		key_map[SDLK_F5] = KEY_F5;
		key_map[SDLK_F6] = KEY_F6;
		key_map[SDLK_F7] = KEY_F7;
		key_map[SDLK_F8] = KEY_F8;
		key_map[SDLK_F9] = KEY_F9;
		key_map[SDLK_F10] = KEY_F10;
		key_map[SDLK_F11] = KEY_F11;
		key_map[SDLK_F12] = KEY_F12;
		key_map[SDLK_F13] = KEY_F13;
		key_map[SDLK_F14] = KEY_F14;
		key_map[SDLK_F15] = KEY_F15;
		key_map[SDLK_LEFT] = KEY_LEFT;
		key_map[SDLK_RIGHT] = KEY_RIGHT;
		key_map[SDLK_UP] = KEY_UP;
		key_map[SDLK_DOWN] = KEY_DOWN;
		key_map[SDLK_KP_0] = KEY_NUMPAD0;
		key_map[SDLK_KP_1] = KEY_NUMPAD1;
		key_map[SDLK_KP_2] = KEY_NUMPAD2;
		key_map[SDLK_KP_3] = KEY_NUMPAD3;
		key_map[SDLK_KP_4] = KEY_NUMPAD4;
		key_map[SDLK_KP_5] = KEY_NUMPAD5;
		key_map[SDLK_KP_6] = KEY_NUMPAD6;
		key_map[SDLK_KP_7] = KEY_NUMPAD7;
		key_map[SDLK_KP_8] = KEY_NUMPAD8;
		key_map[SDLK_KP_9] = KEY_NUMPAD9;
		key_map[SDLK_KP_PLUS] = KEY_NUMPAD_PLUS;
		key_map[SDLK_KP_MINUS] = KEY_NUMPAD_MINUS;
		key_map[SDLK_KP_PLUSMINUS] = KEY_NUMPAD_PLUSMINUS;
		key_map[SDLK_KP_MULTIPLY] = KEY_NUMPAD_MULTIPLY;
		key_map[SDLK_KP_DIVIDE] = KEY_NUMPAD_DIVIDE;
		
		// populate the mouse map
		mouse_map[SDL_BUTTON_LEFT] = MOUSE_LEFT;
		mouse_map[SDL_BUTTON_RIGHT] = MOUSE_RIGHT;
		mouse_map[SDL_BUTTON_MIDDLE] = MOUSE_MIDDLE;
		mouse_map[SDL_BUTTON_X1] = MOUSE_MOUSE4;
		mouse_map[SDL_BUTTON_X2] = MOUSE_MOUSE5;
	}
	
	void SDLWindowLibrary::setup_joysticks()
	{
		// add game controller db
//		size_t length = 0;
//		char* buffer = core::filesystem::file_to_buffer("conf/gamecontrollerdb.conf", 0, &length);
//		int result = SDL_GameControllerAddMapping(buffer);
//		DEALLOC(buffer);
		
		// If you hit this assert, there was an error laoding the gamecontrollerdb.
		// Otherwise, it was either added (result == 1) or updated (result == 0).
//		assert(result != -1);
		
		
		fprintf(stdout, "Gather joystick infos...\n");
		fprintf(stdout, "Num Haptics: %i\n", SDL_NumHaptics());
		fprintf(stdout, "Num Joysticks: %i\n", SDL_NumJoysticks());
		
		
		assert(SDL_NumJoysticks() < input::MAX_JOYSTICKS);
		total_controllers = SDL_NumJoysticks();
		for (uint8_t i = 0; i < total_controllers; ++i)
		{
			input::JoystickInput& js = input::state()->joystick(i);
			input::state()->connect_joystick(i);
			js.reset();
			
			controllers[i] = SDL_GameControllerOpen(i);
			if (SDL_IsGameController(i))
			{
				fprintf(stdout, "Found compatible controller: \"%s\"\n", SDL_GameControllerNameForIndex(i));
				//			fprintf(stdout, "Mapped as: \"%s\"\n", SDL_GameControllerMapping(state->controllers[i]));
				
				SDL_Joystick * joystick = SDL_GameControllerGetJoystick(controllers[i]);
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
			else
			{
				fprintf(stderr, "GameController at index %i, is not a compatible controller.\n", i);
			}
		}
	}
	
	void SDLWindowLibrary::shutdown()
	{
		DESTROY_ARRAY(SDL_Rect, display_rects, total_displays);
		
		// close all controllers
		for (uint8_t i = 0; i < total_controllers; ++i)
		{
			input::JoystickInput& js = input::state()->joystick(i);
			input::state()->disconnect_joystick(i);
			
			SDL_GameController* controller = controllers[i];
			if (controller)
			{
				SDL_GameControllerClose(controller);
				controllers[i] = 0;
			}
		}
		
		key_map.clear();
		
		
		SDL_Quit();
	} // shutdown
	
	
	NativeWindow* SDLWindowLibrary::create_window(kernel::Parameters& parameters)
	{
		SDLWindow* sdlw = 0;
		
		int window_width, window_height;
		int render_width, render_height;
		
		if (kernel::instance()->is_active())
		{
			assert( parameters.window_width != 0 || parameters.window_height != 0 );
			
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
			SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
			
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
			SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
			
			uint32_t window_flags = 0;
			window_flags |= SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
			
			if (parameters.use_fullscreen)
			{
				window_flags |= SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS;
			}
			else
			{
				window_flags |= SDL_WINDOW_RESIZABLE;
			}
			
			sdlw = CREATE(SDLWindow);
			
			sdlw->window = SDL_CreateWindow(
									  parameters.window_title, 0, 0,
									  parameters.window_width, parameters.window_height,
									  window_flags);
			
			if (!sdlw->window)
			{
				fprintf(stderr, "Failed to create SDL window: %s\n", SDL_GetError());
			}
			
			// move the window to the correct display
			SDL_SetWindowPosition(sdlw->window, display_rects[parameters.target_display].x, display_rects[parameters.target_display].y);
			
			sdlw->context = SDL_GL_CreateContext(sdlw->window);
			if (!sdlw->context)
			{
				fprintf(stderr, "Failed to create SDL GL context: %s\n", SDL_GetError());
			}
			
			// try to set our window size; it might still be smaller than requested.
			SDL_SetWindowSize(sdlw->window, parameters.window_width, parameters.window_height);
			
			// center our window
			SDL_SetWindowPosition(sdlw->window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
			
			// fetch the window size and renderable size
			SDL_GetWindowSize(sdlw->window, &window_width, &window_height);
			SDL_GL_GetDrawableSize(sdlw->window, &render_width, &render_height);
			
			// hide the mouse cursor
			show_mouse(false);
			
			parameters.window_width = window_width;
			parameters.window_height = window_height;
			parameters.render_width = render_width;
			parameters.render_height = render_height;
			
			if ( render_width > window_width && render_height > window_height )
			{
				fprintf(stdout, "Retina display detected. Render Resolution is (%i x %i)\n", render_width, render_height );
				parameters.device_flags |= kernel::DeviceSupportsRetinaDisplay;
			}
			else
			{
				fprintf(stdout, "window resolution %i x %i\n", window_width, window_height);
				fprintf(stdout, "render resolution %i x %i\n", render_width, render_height);
			}
		}
		
		if (sdlw)
		{
			windows.push_back(sdlw);
		}
		return sdlw;
	} // create_window
	
	void SDLWindowLibrary::destroy_window(platform::NativeWindow* window)
	{
		SDLWindow* sdlw = static_cast<SDLWindow*>(window);
		if (sdlw)
		{
			if (sdlw->context)
			{
				SDL_GL_DeleteContext(sdlw->context);
			}
			
			if (sdlw->window)
			{
				SDL_DestroyWindow(sdlw->window);
			}
			
			for (auto it = windows.begin(); it != windows.end(); ++it)
			{
				if (sdlw == (*it))
				{
					windows.erase(it);
					DESTROY(SDLWindow, sdlw);
					break;
				}
			}
		}
	}
	
	void controller_axis_event(SDL_ControllerDeviceEvent& device, SDL_ControllerAxisEvent& axis)
	{
		fprintf(stdout, "Axis Motion: %i, %i, %i, %i\n", device.which, axis.which, axis.axis, axis.value);
	}
	
	void controller_button_event(SDL_ControllerDeviceEvent& device, SDL_ControllerButtonEvent& button)
	{
		bool is_down = (button.state == SDL_PRESSED);
		fprintf(stdout, "Button %s: %i, %i, %i\n", (is_down ? "Yes" : "No"), device.which, button.button, button.state);
	}
	
	void SDLWindowLibrary::process_events()
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
					//					LOGV("TODO: add unicode support from SDL: %s\n", event.text.text);
					break;
				}
					
				case SDL_KEYUP:
				case SDL_KEYDOWN:
				{
					button = key_map[event.key.keysym.sym];
					
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
					ev.button = mouse_map[event.button.button];
					ev.is_down = (event.type == SDL_MOUSEBUTTONDOWN);
					input::state()->mouse().inject_mouse_button((input::MouseButton)ev.button, ev.is_down);
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
					input::JoystickInput& joystick = input::state()->joystick(event.cdevice.which);
					input::AxisState& axis = joystick.axes[event.caxis.axis];
					axis.value = event.caxis.value;
					axis.normalized_value = (event.caxis.value/(float)SHRT_MAX);
					
					// check for values outside the deadzone
					if (event.caxis.value > 3200 || event.caxis.value < -3200)
					{
						kernel::GameControllerEvent ev;
						ev.gamepad_id = event.cdevice.which;
						ev.subtype = kernel::JoystickAxisMoved;
						ev.joystick_id = event.caxis.axis;
						ev.joystick_value = event.caxis.value;
						kernel::event_dispatch(ev);
					}
					else
					{
						axis.value = 0;
						axis.normalized_value = 0;
					}
					break;
				}
					
				case SDL_CONTROLLERBUTTONDOWN:
				{
					controller_button_event(event.cdevice, event.cbutton);
					
					kernel::GameControllerEvent ev;
					ev.gamepad_id = event.cdevice.which;
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
					ev.gamepad_id = event.cdevice.which;
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
					fprintf(stdout, "Device Added: %i\n", event.cdevice.which);
					
					input::JoystickInput& js = input::state()->joystick(event.cdevice.which);
					js.reset();
					input::state()->connect_joystick(event.cdevice.which);
					
					
					controllers[event.cdevice.which] = SDL_GameControllerOpen(event.cdevice.which);
//					SDL_Joystick * joystick = SDL_GameControllerGetJoystick(controllers[event.cdevice.which]);
					
					
					kernel::GameControllerEvent ev;
					ev.gamepad_id = event.cdevice.which;
					ev.subtype = kernel::JoystickConnected;
					kernel::event_dispatch(ev);
					break;
				}
					
				case SDL_CONTROLLERDEVICEREMOVED:
				{
					fprintf(stdout, "Device Removed: %i\n", event.cdevice.which);
					
					input::state()->disconnect_joystick(event.cdevice.which);
					
					SDL_GameControllerClose(controllers[event.cdevice.which]);
					controllers[event.cdevice.which] = 0;
					
					
					kernel::GameControllerEvent ev;
					ev.gamepad_id = event.cdevice.which;
					ev.subtype = kernel::JoystickDisconnected;
					kernel::event_dispatch(ev);
					break;
				}
					
					// handle window events
				case SDL_WINDOWEVENT:
				{
					switch (event.window.event)
					{
						case SDL_WINDOWEVENT_FOCUS_LOST:
						{
							kernel::Event<kernel::System> event;
							event.subtype = kernel::WindowLostFocus;
							kernel::event_dispatch(event);
							break;
						}
							
						case SDL_WINDOWEVENT_FOCUS_GAINED:
						{
							kernel::Event<kernel::System> event;
							event.subtype = kernel::WindowGainFocus;
							kernel::event_dispatch(event);
							break;
						}
					}
				}
			}
		}
	} // process_events
	
	void SDLWindowLibrary::activate_window(NativeWindow* window)
	{
		// ...
	}
	
	void SDLWindowLibrary::swap_buffers(NativeWindow* window)
	{
		SDLWindow* sdlw = static_cast<SDLWindow*>(window);
		SDL_GL_SwapWindow(sdlw->window);
	}
	
	void SDLWindowLibrary::capture_mouse(bool capture)
	{
		SDL_bool is_enabled = capture ? SDL_TRUE : SDL_FALSE;
		SDL_SetRelativeMouseMode(is_enabled);
	}
	
	void SDLWindowLibrary::warp_mouse(int x, int y)
	{
		assert(!windows.empty());
		SDLWindow* sdlw = windows[0];
		
		SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
		SDL_WarpMouseInWindow(sdlw->window, x, y);
		SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);
	}
	
	void SDLWindowLibrary::get_mouse(int& x, int& y)
	{
		SDL_GetMouseState(&x, &y);
	}
	
	void SDLWindowLibrary::show_mouse(bool show)
	{
		SDL_ShowCursor((show ? SDL_TRUE : SDL_FALSE));
	}
} // namespace platform