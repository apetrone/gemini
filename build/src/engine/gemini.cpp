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

#include <platform/platform.h>
#include <platform/kernel.h>

#include "input.h"

#include <core/core.h>
#include <core/str.h>
#include <core/logging.h>
#include <core/filesystem.h>
#include <core/configloader.h>
#include <renderer/renderer.h>

#include "debugdraw.h"
#include "assets/asset_font.h"
#include "assets/asset_shader.h"

#include "audio.h"
#include "animation.h"
#include "physics.h"
#include "hotloading.h"

using namespace core;
using namespace gemini; // for renderer
using namespace input;

struct Settings
{
	StackString<64> kernel_name;
	uint32_t physics_tick_rate;
	int32_t debugdraw_max_primitives;
	uint32_t enable_asset_reloading : 1;
	
	renderer::RenderSettings render_settings;
	
	Settings()
	{
		// setup sane defaults
		physics_tick_rate = 60;
		debugdraw_max_primitives = 2048;
		enable_asset_reloading = 0;
	}
};

static util::ConfigLoadStatus load_render_config(const Json::Value& root, void* data)
{
	renderer::RenderSettings* settings = static_cast<renderer::RenderSettings*>(data);
	
	// TODO: there should be a better way to do this?
	if (!root["gamma_correct"].isNull())
	{
		settings->gamma_correct = root["gamma_correct"].asBool();
	}
	
	return util::ConfigLoad_Success;
}

static util::ConfigLoadStatus settings_conf_loader( const Json::Value & root, void * data )
{
	util::ConfigLoadStatus result = util::ConfigLoad_Success;
	
	Settings* cfg = (Settings*)data;
	if ( !cfg )
	{
		return util::ConfigLoad_Failure;
	}
	
	LOGV( "loading settings...\n" );
	const Json::Value& kernel_name = root["kernel_name"];
	if (!kernel_name.isNull())
	{
		cfg->kernel_name = kernel_name.asString().c_str();
	}
	
	const Json::Value& physics_tick_rate = root["physics_tick_rate"];
	if (!physics_tick_rate.isNull())
	{
		cfg->physics_tick_rate = physics_tick_rate.asUInt();
	}
	
	const Json::Value& debugdraw_max_primitives = root["debugdraw_max_primitives"];
	if (!debugdraw_max_primitives.isNull())
	{
		cfg->debugdraw_max_primitives = debugdraw_max_primitives.asInt();
	}
	
	const Json::Value& enable_asset_reloading = root["enable_asset_reloading"];
	if (!enable_asset_reloading.isNull())
	{
		cfg->enable_asset_reloading = enable_asset_reloading.asBool();
	}
	
	
	const Json::Value& renderer = root["renderer"];
	if (!renderer.isNull())
	{
		// load renderer settings
		result = load_render_config(renderer, &cfg->render_settings);
	}
	
	return result;
}



#include <SDL.h>




class EngineKernel : public kernel::IKernel
{



private:
	bool active;
	StackString<MAX_PATH_SIZE> game_path;
	
	SDL_Window* window;
	SDL_GLContext context;
	SDL_Rect* display_rects;
	uint8_t total_displays;
	uint8_t total_controllers;
	
	typedef CustomPlatformAllocator<std::pair<const unsigned int, input::Button> > ButtonKeyMapAllocator;
	typedef std::map<unsigned int, input::Button, std::less<unsigned int>, ButtonKeyMapAllocator> SDLToButtonKeyMap;
	SDLToButtonKeyMap key_map;
	
	input::MouseButton mouse_map[input::MOUSE_COUNT];
	SDL_GameController* controllers[input::MAX_JOYSTICKS];
	
	
	// Kernel State variables
	double accumulator;
	uint64_t last_time;
	
private:
	bool sdl_startup()
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
		
		
		if (kernel::parameters().use_vsync)
		{
			SDL_GL_SetSwapInterval(1);
		}
		
#if defined(PLATFORM_MOBILE)
#else
		kernel::parameters().device_flags |= kernel::DeviceDesktop;
#endif
		
		//
		// setup input mappings

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



		
		return true;
	} // sdl_startup
	
	
	void sdl_shutdown()
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
		
		
		SDL_GL_DeleteContext(context);
		SDL_DestroyWindow(window);
		SDL_Quit();
	} // sdl_shutdown
	
	void create_window()
	{
		int window_width, window_height;
		int render_width, render_height;
		
		if ( is_active() )
		{
			assert( kernel::parameters().window_width != 0 || kernel::parameters().window_height != 0 );
			
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
			SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
			
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
			SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
			
			uint32_t window_flags = 0;
			window_flags |= SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
			
			if (kernel::parameters().use_fullscreen)
			{
				window_flags |= SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS;
			}
			else
			{
				window_flags |= SDL_WINDOW_RESIZABLE;
			}
			
			window = SDL_CreateWindow(
											 kernel::parameters().window_title, 0, 0,
											 kernel::parameters().window_width, kernel::parameters().window_height,
											 window_flags);
			
			if (!window)
			{
				LOGE("Failed to create SDL window: %s\n", SDL_GetError());
			}
			
			// move the window to the correct display
			SDL_SetWindowPosition(window, display_rects[kernel::parameters().target_display].x, display_rects[kernel::parameters().target_display].y);
			
			context = SDL_GL_CreateContext(window);
			if (!context)
			{
				LOGE("Failed to create SDL GL context: %s\n", SDL_GetError());
			}
			
			// try to set our window size; it might still be smaller than requested.
			SDL_SetWindowSize(window, kernel::parameters().window_width, kernel::parameters().window_height);
			
			// center our window
			SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
			
			// fetch the window size and renderable size
			SDL_GetWindowSize(window, &window_width, &window_height);
			SDL_GL_GetDrawableSize(window, &render_width, &render_height);
			
			// hide the mouse cursor
			show_mouse(false);
			
			kernel::parameters().window_width = window_width;
			kernel::parameters().window_height = window_height;
			kernel::parameters().render_width = render_width;
			kernel::parameters().render_height = render_height;
			
			if ( render_width > window_width && render_height > window_height )
			{
				LOGV( "Retina display detected. Render Resolution is (%i x %i)\n", render_width, render_height );
				kernel::parameters().device_flags |= kernel::DeviceSupportsRetinaDisplay;
			}
			else
			{
				LOGV( "window resolution %i x %i\n", window_width, window_height );
				LOGV( "render resolution %i x %i\n", render_width, render_height );
			}
		}
	} // create_window

	bool load_config(Settings& config)
	{
		bool success = util::json_load_with_callback( "conf/settings.conf", settings_conf_loader, &config, true );
		if ( !success )
		{
			LOGV( "Unable to locate settings.conf.\n" );
			// load sane defaults
			config.kernel_name = "HelloWorld";
		}
		
		return success;
	} // load_config
	
public:
	EngineKernel() :
		active(true),
		window(0),
		context(0),
		display_rects(0),
		total_displays(0),
		total_controllers(0),
		accumulator(0.0f),
		last_time(0)
	{
		game_path = "/Users/apetrone/Documents/games/vrpowergrid";
		
		memset(mouse_map, 0, sizeof(input::MouseButton)*input::MOUSE_COUNT);
		memset(controllers, 0, sizeof(SDL_GameController*)*input::MAX_JOYSTICKS);
	}
	
	virtual ~EngineKernel()
	{
		
	}
	
	virtual bool is_active() const { return active; }
	virtual void set_active(bool isactive) { active = isactive; }
	
	virtual void resolution_changed(int width, int height) {}
	
	void sdl_setup_joysticks()
	{
		// add game controller db
		size_t length = 0;
		char* buffer = core::filesystem::file_to_buffer("conf/gamecontrollerdb.conf", 0, &length);
		int result = SDL_GameControllerAddMapping(buffer);
		DEALLOC(buffer);
		
		// If you hit this assert, there was an error laoding the gamecontrollerdb.
		// Otherwise, it was either added (result == 1) or updated (result == 0).
		assert(result != -1);
		
		
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
	
	virtual kernel::Error startup()
	{
		const char FONT_SHADER[] = "shaders/fontshader";
		const char DEBUG_FONT[] = "fonts/debug";
		const char DEBUG_SHADER[] = "shaders/debug";
		kernel::Parameters& params = kernel::parameters();
	
		// initialize timer
		last_time = platform::instance()->get_time_microseconds();
	
	
		sdl_startup();
	
	
	
	
		//
		// setup our file system...
		StackString< MAX_PATH_SIZE > root_path;
		platform::Result result = platform::instance()->get_program_directory(&root_path[0], root_path.max_size());
		assert(!result.failed());
		
		// set the startup directory: where the binary lives
		core::filesystem::root_directory(&root_path[0], root_path.max_size());
		
		
		// if no game is specified on the command line, construct the content path
		// from the current root directory
		StackString<MAX_PATH_SIZE> content_path;
		if (game_path.is_empty())
		{
			core::filesystem::construct_content_directory(content_path);
		}
		else
		{
			// dev builds (passed by -game) are located at:
			// "<game_path>/builds/<PLATFORM_NAME>"
			content_path = game_path;
			content_path.append(PATH_SEPARATOR_STRING);
			content_path.append("builds");
			content_path.append(PATH_SEPARATOR_STRING);
			content_path.append(PLATFORM_NAME);
		}
		
		// set the content path
		core::filesystem::content_directory(content_path());
		
		// startup duties; lower-level system init
		result = core::startup();
		if (result.failed())
		{
			fprintf(stderr, "Fatal error: %s\n", result.message);
			core::shutdown();
			return kernel::CoreFailed;
		}
		
		
		sdl_setup_joysticks();
		// TODO: kernel_desktop::register_services (load game controller config, etc.)
		
		// load engine settings
		// load boot config
		Settings config;
		load_config(config);
		
		params.step_interval_seconds = (1.0f/(float)config.physics_tick_rate);
		
		// used to load the application here
		// used to run application config here
		
		// TODO: we should load these from a config; for now just set them.
		params.window_width = 1280;
		params.window_height = 720;
		params.window_title = "gemini";
		
		// TODO: do old application config; setup VR headset
		
		// TODO: post application config; create the window
		create_window();

		// initialize rendering subsystems
		{
			int render_result =	renderer::startup(renderer::Default, config.render_settings);
			if ( render_result == 0 )
			{
				LOGE("renderer initialization failed!\n");
				return kernel::RendererFailed;
			}
			
			assets::startup();
			
			assets::Shader* fontshader = assets::shaders()->load_from_path(FONT_SHADER);
			assert(fontshader != 0);
			font::startup(fontshader->program, params.render_width, params.render_height);
			
			assets::Shader* debugshader = assets::shaders()->load_from_path(DEBUG_SHADER);
			assets::Font* debugfont = assets::fonts()->load_from_path(DEBUG_FONT);
			assert(debugshader != 0);
			assert(debugfont != 0);
			debugdraw::startup(config.debugdraw_max_primitives, debugshader->program, debugfont->handle);
		}
		
		// initialize main subsystems
		audio::startup();
		input::startup();
		gemini::physics::startup();
		animation::startup();

		if (config.enable_asset_reloading)
		{
			hotloading::startup();
		}
		
		// TODO: call the old Application startup function here.
		
		// TODO: post_application_startup
		
		// TODO: initialize timer
		uint64_t current_time = platform::instance()->get_time_microseconds();
		LOGV("startup in %2.2fms\n", (current_time-last_time)*.001f);
		last_time = current_time;
		
		return kernel::NoError;
	}
	
	
	
	void update()
	{
		uint64_t current_time = platform::instance()->get_time_microseconds();
		kernel::Parameters& params = kernel::parameters();
		
		// calculate delta ticks in miliseconds
		params.framedelta_raw_msec = (current_time - last_time)*0.001f;
		// cache the value in seconds
		params.framedelta_filtered_seconds = params.framedelta_raw_msec*0.001f;
		last_time = current_time;
		
		// update accumulator
		accumulator += params.framedelta_filtered_seconds;
		
		while(accumulator >= params.step_interval_seconds)
		{
			// pass off to application
//			_active_application->step( _parameters );
			
			// subtract the interval from the accumulator
			accumulator -= params.step_interval_seconds;
			
			// increment tick counter
			params.current_tick++;
		}
		
		params.step_alpha = accumulator / params.step_interval_seconds;
		if ( params.step_alpha >= 1.0f )
		{
			params.step_alpha -= 1.0f;
		}
	}
	
	
	void controller_axis_event(SDL_ControllerDeviceEvent& device, SDL_ControllerAxisEvent& axis)
	{
		LOGV("Axis Motion: %i, %i, %i, %i\n", device.which, axis.which, axis.axis, axis.value);
	}
	
	void controller_button_event(SDL_ControllerDeviceEvent& device, SDL_ControllerButtonEvent& button)
	{
		bool is_down = (button.state == SDL_PRESSED);
		LOGV("Button %s: %i, %i, %i\n", (is_down ? "Yes" : "No"), device.which, button.button, button.state);
	}
	
	void pre_tick()
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
					LOGV("Device Added: %i\n", event.cdevice.which);
					
					input::JoystickInput& js = input::state()->joystick(event.cdevice.which);
					js.reset();
					input::state()->connect_joystick(event.cdevice.which);
					
					
					controllers[event.cdevice.which] = SDL_GameControllerOpen(event.cdevice.which);
					SDL_Joystick * joystick = SDL_GameControllerGetJoystick(controllers[event.cdevice.which]);
					
					
					kernel::GameControllerEvent ev;
					ev.gamepad_id = event.cdevice.which;
					ev.subtype = kernel::JoystickConnected;
					kernel::event_dispatch(ev);
					break;
				}
					
				case SDL_CONTROLLERDEVICEREMOVED:
				{
					LOGV("Device Removed: %i\n", event.cdevice.which);
					
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
	} // pre_tick
	
	virtual void tick()
	{
		update();
	
		audio::update();
		input::update();
		animation::update(kernel::parameters().framedelta_filtered_seconds);
		pre_tick();
		hotloading::tick();
		// TODO: application -> tick
		post_tick();
		kernel::parameters().current_frame++;
	}
	
	void post_tick()
	{
		// TODO: this needs to be controlled somehow
		// as the rift sdk performs buffer swaps during end frame.
		if (kernel::parameters().swap_buffers)
		{
			SDL_GL_SwapWindow(window);
		}
	}
	
	virtual void shutdown()
	{
		// TODO: application shutdown
		
		hotloading::shutdown();
		animation::shutdown();
		gemini::physics::shutdown();
		debugdraw::shutdown();
		font::shutdown();
		assets::shutdown();
		input::shutdown();
		audio::shutdown();
		renderer::shutdown();
		core::shutdown();
	
		sdl_shutdown();
	}
	
	virtual void capture_mouse(bool capture)
	{
		SDL_bool is_enabled = capture ? SDL_TRUE : SDL_FALSE;
		SDL_SetRelativeMouseMode(is_enabled);
	}
	
	virtual void warp_mouse(int x, int y)
	{
		SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
		SDL_WarpMouseInWindow(window, x, y);
		SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);
	}
	
	virtual void get_mouse_position(int& x, int& y)
	{
		SDL_GetMouseState(&x, &y);
	}
	
	virtual void show_mouse(bool show)
	{
		SDL_ShowCursor((show ? SDL_TRUE : SDL_FALSE));
	}
};



PLATFORM_MAIN
{
	int return_code;
	return_code = platform::run_application(new EngineKernel());
	return return_code;
}