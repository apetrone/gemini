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
		
		
		
		// TODO: kernel_desktop::register_services (load game controller config, etc.)
		
		// load engine settings
		// load boot config
		Settings config;
		load_config(config);
		
		// TODO: params.step_interval_seconds = (1.0f/(float)config.physics_tick_rate);
		
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
	
	virtual void tick() {}
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
	
	virtual void capture_mouse(bool capture) {}
	virtual void warp_mouse(int x, int y) {}
	virtual void get_mouse_position(int& x, int& y) {}
	virtual void show_mouse(bool show) {}
};



PLATFORM_MAIN
{
	int return_code;
	EngineKernel ek;
	return_code = platform::run_application(&ek);
	return return_code;
}