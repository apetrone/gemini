// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone
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


#include "kernel.h"
#include <string.h>

namespace kernel
{
	namespace detail
	{
		IKernel* kernel_instance = 0;
		Parameters parameters;
		
		
		struct EventHooks
		{
			void* events[ kernel::EventTypeCount ];
			EventHooks();
		};
		
		EventHooks::EventHooks()
		{
			memset(events, 0, sizeof(void*) * kernel::EventTypeCount );
		}
		
		EventHooks event_hooks;
		
	} // namespace detail
	
	IKernel* instance()
	{
		return detail::kernel_instance;
	} // instance
	
	void set_instance(IKernel* instance)
	{
		detail::kernel_instance = instance;
	} // set_instance
	
	Parameters& parameters()
	{
		return detail::parameters;
	} // parameters
	
	
	
	
	void assign_listener_for_eventtype( kernel::EventType event_type, void * listener )
	{
		detail::event_hooks.events[ event_type ] = listener;
	} // assign_listener_for_eventtype
	
	void * find_listener_for_eventtype( kernel::EventType event_type )
	{
		return detail::event_hooks.events[ event_type ];
	} // find_listener_for_eventtype
	
	
	Parameters::Parameters()
	{
		error_message = 0;
		device_flags = 0;
		window_width = 0;
		window_height = 0;
//		step_alpha = 0;
//		step_interval_seconds = 0;
		use_fullscreen = false;
		use_vsync = true;
		target_display = 0;
		
		// we should default to swapping buffers ourself
		swap_buffers = 1;
		
		titlebar_height = 0;
//		current_tick = 0;
//		current_frame = 0;
	}

	
	Error startup()
	{
		Error result = kernel::Error::NoError;
		
		// perform any startup duties here before we init the core
		detail::kernel_instance->startup();
		
		return result;
	} // startup
	
	void shutdown()
	{
		
	}
	
	void resolution_changed(int width, int height)
	{
		
	}
	
	void tick()
	{
		
	}
	
	
	

} // namespace kernel



#if 0
#include <string>
#include <map>

#include <core/typedefs.h>
#include <core/core.h>
#include <platform/platform.h>
#include <core/configloader.h>
#include <core/filesystem.h>
#include <core/logging.h>

#include "kernel.h"
#include <renderer/renderer.h>
#include "audio.h"
#include "input.h"
#include "assets.h"
#include <renderer/font.h>

#include "debugdraw.h"
#include "physics/physics.h"
#include "hotloading.h"
#include "animation.h"


#include "debugdraw_interface.h"

#if PLATFORM_LINUX
	#include <stdlib.h> // for qsort
#endif

using namespace core;

namespace gemini
{
	namespace kernel
	{
		// This is NOT hooked up to the memory allocator because this is accessed before the memory allocator is initialized.
		typedef std::map< std::string, ApplicationCreator> ApplicationCreatorByString;
		
		const char FONT_SHADER[] = "shaders/fontshader";
		const char DEBUG_FONT[] = "fonts/debug";
		const char DEBUG_SHADER[] = "shaders/debug";
		
		IKernel * _kernel = 0;
		IApplication * _active_application = 0;

		static Parameters _parameters;
		Parameters& parameters()
		{
			return _parameters;
		}
		


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

		Parameters::Parameters()
		{
			error_message = 0;
			device_flags = 0;
			window_width = 0;
			window_height = 0;
			step_alpha = 0;
			step_interval_seconds = 0;
			use_fullscreen = false;
			use_vsync = true;
			target_display = 0;

			// we should default to swapping buffers ourself
			swap_buffers = 1;

			titlebar_height = 0;
			current_tick = 0;
			current_frame = 0;
		}

		namespace _internal
		{
			struct EventHooks
			{
				void * events[ kernel::EventTypeCount ];
				EventHooks();
			};
			
			EventHooks::EventHooks()
			{
				memset( events, 0, sizeof(void*) * kernel::EventTypeCount );
			}
			
			EventHooks _event_hooks;
			const char* game_path = 0;
			
			ApplicationCreatorByString & creator_map()
			{
				static ApplicationCreatorByString _application_creator_map;
				return _application_creator_map;
			}
			
			static void register_application_by_name( const char * kernel_name, ApplicationCreator ApplicationCreator )
			{
				creator_map().insert( ApplicationCreatorByString::value_type( kernel_name, ApplicationCreator) );
			} // register_application_by_name
			
			static ApplicationCreator find_application_by_name( const char * kernel_name )
			{
				std::string kname = kernel_name;
				ApplicationCreatorByString::iterator it = creator_map().find( kname );
				if ( it != creator_map().end() )
				{
					return (*it).second;
				}
				
				return 0;
			} // find_application_by_name
			
			
			struct State
			{
				double accumulator;
				uint64_t last_time;
			};
			
			util::ConfigLoadStatus load_render_config(const Json::Value& root, void* data)
			{
				renderer::RenderSettings* settings = static_cast<renderer::RenderSettings*>(data);
				
				// TODO: there should be a better way to do this?
				if (!root["gamma_correct"].isNull())
				{
					settings->gamma_correct = root["gamma_correct"].asBool();
				}
				
				return util::ConfigLoad_Success;
			}
			
			util::ConfigLoadStatus settings_conf_loader( const Json::Value & root, void * data )
			{
				util::ConfigLoadStatus result = util::ConfigLoad_Success;
			
				kernel::Settings * cfg = (kernel::Settings*)data;
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
			
			bool load_config( kernel::Settings& config )
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

			State _kernel_state;
		}; // namespace _internal
		
		Registrar::Registrar( const char * kernel_name, ApplicationCreator ApplicationCreator )
		{
			_internal::register_application_by_name( kernel_name, ApplicationCreator );
		}
		
		IKernel * instance()
		{
			return _kernel;
		} // instance
		
		platform::Result load_application( const char * application_name )
		{
			platform::Result result(platform::Result::Success);
			fprintf(stdout, "Loading kernel '%s'\n", application_name);
			
			// search for the named kernel instance (passed on the command line or overriden above)
			ApplicationCreator creator = _internal::find_application_by_name( application_name );
			if ( creator )
			{
				// create an instance of the kernel
				_active_application = creator();
				if ( !_active_application )
				{
					fprintf(stdout, "Unable to create an instance of application: \"%s\", aborting!\n", application_name);
					result = platform::Result(platform::Result::Failure, "Cannot " );
				}
			}
			else
			{
				fprintf(stdout, "Application named \"%s\" not found, aborting!\n", application_name);
				result = platform::Result(platform::Result::Failure, "Application not found by name" );
			}
			
			return result;
		} // load_application
		
		
		Error startup(IKernel * kernel_instance)
		{
			// set instance
			_kernel = kernel_instance;
			if (!_kernel)
			{
				fprintf(stderr, "No valid kernel instance found\n");
				return kernel::NoInstance;
			}
			
			// setup parameters
			kernel::Parameters& params = parameters();
			
			// the kernel is ACTIVE here; callbacks after config/start may modify this
			_kernel->set_active(true);
			
			// initialize kernel's timer
			_internal::_kernel_state.last_time = platform::instance()->get_time_microseconds();
			_internal::_kernel_state.accumulator = 0;

			// perform any startup duties here before we init the core
			_kernel->startup();
			
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
			if (_internal::game_path == 0)
			{
				core::filesystem::construct_content_directory(content_path);
			}
			else
			{
				// dev builds (passed by -game) are located at:
				// "<game_path>/builds/<PLATFORM_NAME>"
				content_path = _internal::game_path;
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
					
			// ask the kernel to register services
			_kernel->register_services();
			
			// load boot config
			kernel::Settings config;
			_internal::load_config(config);
			
			params.step_interval_seconds = (1.0f/(float)config.physics_tick_rate);
			
			// load application
			result = load_application(config.kernel_name());
			if (result.failed())
			{
				fprintf(stderr, "Fatal error loading application '%s' -> %s, aborting.\n", config.kernel_name(), result.message);
				_kernel->set_active(false);
				return kernel::ApplicationFailure;
			}
			
			// application config
			ApplicationResult config_result = Application_Failure;
			if (_active_application)
			{
				config_result = _active_application->config(kernel::parameters());
			}
			
			// evaluate config result
			kernel::instance()->post_application_config(config_result);
		
			if (config_result == kernel::Application_Failure)
			{
				fprintf(stderr, "Application config failed. aborting.\n");
				return kernel::ConfigFailed;
			}
			
			// try to setup the renderer
			if (config_result != kernel::Application_NoWindow)
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
			
			// initialize subsystems
			audio::startup();
			input::startup();
			gemini::physics::startup();
			animation::startup();
			
			if (config.enable_asset_reloading)
			{
				hotloading::startup();
			}
			
			// application instance failed startup
			ApplicationResult startup_result = _active_application->startup(kernel::parameters());
			
			// evaluate startup result
			_kernel->post_application_startup(startup_result);
			
			if (startup_result == kernel::Application_Failure)
			{
				fprintf(stderr, "Application startup failed!\n");
				return kernel::StartupFailed;
			}
			
			_internal::_kernel_state.last_time = platform::instance()->get_time_microseconds();
			return kernel::NoError;
		} // startup
		
		void shutdown()
		{
			// cleanup
			if ( _active_application )
			{
				_active_application->shutdown(parameters());
				DESTROY(IApplication, _active_application);
			}

			// system cleanup
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
			
			// shutdown, cleanup
			if ( _kernel )
			{
				_kernel->shutdown();
				_kernel = 0;
			}
			
			_internal::game_path = 0;
		} // shutdown

		void update()
		{
			uint64_t current_ticks = platform::instance()->get_time_microseconds();

			// calculate delta ticks in miliseconds
			_parameters.framedelta_raw_msec = (current_ticks - _internal::_kernel_state.last_time)*0.001f;
			_internal::_kernel_state.last_time = current_ticks;

			// convert delta msec to seconds
			_internal::_kernel_state.accumulator += (_parameters.framedelta_raw_msec * 0.001f);

			while(_internal::_kernel_state.accumulator >= _parameters.step_interval_seconds)
			{
				// pass off to application
				_active_application->step( _parameters );
				
				// subtract the interval from the accumulator
				_internal::_kernel_state.accumulator -= _parameters.step_interval_seconds;

				// increment tick counter
				_parameters.current_tick++;
			}
			
			_parameters.step_alpha = _internal::_kernel_state.accumulator / _parameters.step_interval_seconds;
			if ( _parameters.step_alpha >= 1.0f )
			{
				_parameters.step_alpha -= 1.0f;
			}
		} // update

		void tick()
		{
			update();
			
			audio::update();
			input::update();
			animation::update(_parameters.framedelta_filtered_msec*.001f);
			_kernel->pre_tick();
			hotloading::tick();			
			_active_application->tick( _parameters );
			_kernel->post_tick();
			_parameters.current_frame++;
		} // tick
		
		void parse_commandline(int argc, char** argv)
		{
			const char* arg;
			for(int i = 0; i < argc; ++i)
			{
				arg = argv[i];
				if (String(arg) == "-game")
				{
					_internal::game_path = argv[i+1];
				}
			}
		}
		
		void assign_listener_for_eventtype( kernel::EventType event_type, void * listener )
		{
			_internal::_event_hooks.events[ event_type ] = listener;
		} // assign_listener_for_eventtype
		
		void * find_listener_for_eventtype( kernel::EventType event_type )
		{
			return _internal::_event_hooks.events[ event_type ];
		} // find_listener_for_eventtype

	} // namespace kernel
} // namespace gemini
#endif