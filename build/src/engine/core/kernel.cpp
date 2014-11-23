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
#include <string>
#include <map>

#include <gemini/typedefs.h>
#include <gemini/core.h>
#include <gemini/platform.h>
#include <gemini/util/configloader.h>
#include <gemini/core/filesystem.h>

#include <slim/xlog.h>
#include <slim/xtime.h>

#include "kernel.h"
#include "renderer/renderer.h"
#include "audio.h"
#include "input.h"
#include "assets.h"
#include "font.h"
#include "script.h"
#include "debugdraw.h"
#include "physics.h"
#include "hotloading.h"


#if PLATFORM_LINUX
	#include <stdlib.h> // for qsort
#endif

namespace kernel
{
	// This is NOT hooked up to the memory allocator because this is accessed before the memory allocator is initialized.
	typedef std::map< std::string, ApplicationCreator> ApplicationCreatorByString;
	
	IKernel * _kernel = 0;
	IApplication * _active_application = 0;

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

	Params::Params()
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
		std::string game_path;
		
		ApplicationCreatorByString & creator_map()
		{
			static ApplicationCreatorByString _application_creator_map;
			return _application_creator_map;
		}
		
		static void register_application_by_name( const char * kernel_name, ApplicationCreator ApplicationCreator )
		{
			creator_map().insert( ApplicationCreatorByString::value_type( kernel_name, ApplicationCreator) );
		} // registerKernelApplicationCreatorByName
		
		static ApplicationCreator find_application_by_name( const char * kernel_name )
		{
			std::string kname = kernel_name;
			ApplicationCreatorByString::iterator it = creator_map().find( kname );
			if ( it != creator_map().end() )
			{
				return (*it).second;
			}
			
			return 0;
		} // findKernelApplicationCreatorByName
		
		
		const int TimeStepFilterMaxFrames = 11;
		struct TimeStepFilter
		{
			float deltas[ TimeStepFilterMaxFrames ];
			int current_frame;
			float filtered_value;
			float last_step;
			float lerp_value;
			
			TimeStepFilter();
			void tick( float delta_msec );
		};
		
		
		// this is a descending compare function
		static int TimeStepFilter_Compare( const void * left, const void * right )
		{
			float * lk = ((float*)left);
			float * rk = ((float*)right);
			if ( *lk < *rk )
			{
				return 1;
			}
			else if ( *lk > *rk )
			{
				return -1;
			}
			
			return 0;
		}
		
		TimeStepFilter::TimeStepFilter()
		{
			current_frame = 0;
			memset( deltas, 0, sizeof(float)*TimeStepFilterMaxFrames );
			filtered_value = 0;
			last_step = 0;
			lerp_value = 0.1;
		}
		
		void TimeStepFilter::tick( float delta_msec )
		{
			deltas[ (current_frame++ % TimeStepFilterMaxFrames) ] = delta_msec;
			
			// sort these from highest to lowest
			qsort( deltas, TimeStepFilterMaxFrames, sizeof(float), TimeStepFilter_Compare );
			
			// sum 7 of the middle values
			float sum = 0;
			for( int i = 2; i < TimeStepFilterMaxFrames-2; ++i )
			{
				sum += deltas[ i ];
			}
			
			float mean = sum / 7.0;
			filtered_value = (last_step * (1-lerp_value)) + (mean * lerp_value);
			last_step = filtered_value;
		}
		
		struct State
		{
			xtime_t timer;
			double accumulator;
			float last_time;
			TimeStepFilter tsa;
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
	
	core::Result load_application( const char * application_name )
	{
		core::Result result(core::Result::Success);
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
				result = core::Result(core::Result::Failure, "Cannot " );
			}
		}
		else
		{
			fprintf(stdout, "Application named \"%s\" not found, aborting!\n", application_name);
			result = core::Result(core::Result::Failure, "Application not found by name" );
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
		kernel::Params & params = kernel_instance->parameters();
		
		// the kernel is ACTIVE here; callbacks after config/start may modify this
		_kernel->set_active(true);
		
		// initialize kernel's timer
		xtime_startup(&_internal::_kernel_state.timer);
		_internal::_kernel_state.last_time = xtime_msec(&_internal::_kernel_state.timer);
		_internal::_kernel_state.accumulator = 0;

		// perform any startup duties here before we init the core
		_kernel->startup();
		
		//
		// setup our file system...
		StackString< MAX_PATH_SIZE > root_path;
		core::Result result = platform::program_directory(&root_path[0], root_path.max_size());
		assert(!result.failed());
		
		// set the startup directory: where the binary lives
		core::filesystem::root_directory(&root_path[0], root_path.max_size());
		
		// if no game is specified on the command line, construct the content path
		// from the current root directory
		StackString<MAX_PATH_SIZE> content_path;
		if (_internal::game_path.empty())
		{
			core::filesystem::construct_content_directory(content_path);
		}
		else
		{
			// dev builds (passed by -game) are located at:
			// "<game_path>/builds/<PLATFORM_NAME>"
			content_path = _internal::game_path.c_str();
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
			config_result = _active_application->config(kernel::instance()->parameters());
		}
		
		// evaluate config result
		kernel::instance()->post_application_config(config_result);
	
		if (config_result == kernel::Application_Failure)
		{
			fprintf(stderr, "Application config failed. aborting.\n");
			return kernel::ConfigFailed;
		}
		
		// setup script subsystem
		script::startup();
		
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
			font::startup();
			debugdraw::startup(config.debugdraw_max_primitives);
		}
		
		// initialize subsystems
		audio::startup();
		input::startup();
		physics::startup();
		
		if (config.enable_asset_reloading)
		{
			hotloading::startup();
		}
		
		// application instance failed startup
		ApplicationResult startup_result = _active_application->startup(kernel::instance()->parameters());
		
		// evaluate startup result
		_kernel->post_application_startup(startup_result);
		
		if (startup_result == kernel::Application_Failure)
		{
			fprintf(stderr, "Application startup failed!\n");
			return kernel::StartupFailed;
		}
		
		return kernel::NoError;
	} // startup
	
	void shutdown()
	{
		// cleanup
		if ( _active_application )
		{
			_active_application->shutdown( _kernel->parameters() );
			DESTROY(IApplication, _active_application);
		}

		// system cleanup
		hotloading::shutdown();
		script::shutdown();
		
		
		physics::shutdown();
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
		
	} // shutdown

	void update()
	{
		float now_msec = xtime_msec(&_internal::_kernel_state.timer);
		float raw_delta_msec = now_msec - _internal::_kernel_state.last_time;
		_internal::_kernel_state.last_time = now_msec;

		_internal::_kernel_state.tsa.tick( raw_delta_msec );
		
		_kernel->parameters().framedelta_filtered_msec = _internal::_kernel_state.tsa.filtered_value;
		_kernel->parameters().framedelta_raw_msec = raw_delta_msec;
		
		_internal::_kernel_state.accumulator += (_internal::_kernel_state.tsa.filtered_value * .001);
		while( _internal::_kernel_state.accumulator >= _kernel->parameters().step_interval_seconds )
		{
			_internal::_kernel_state.accumulator -= _kernel->parameters().step_interval_seconds;
			_kernel->parameters().step_alpha = _internal::_kernel_state.accumulator / _kernel->parameters().step_interval_seconds;
			if ( _kernel->parameters().step_alpha >= 1.0f )
			{
				_kernel->parameters().step_alpha -= 1.0f;
			}
			
			// pass off to application
			_active_application->step( _kernel->parameters() );
			
			// step physics before we let the application have a chance
			physics::step(_kernel->parameters().step_interval_seconds);
			
			// step debug draw
			debugdraw::update(_kernel->parameters().step_interval_seconds);
		}
	} // update

	void tick()
	{
		audio::update();
		input::update();

		update();
		_kernel->pre_tick();
		hotloading::tick();
		_active_application->tick( _kernel->parameters() );
		_kernel->post_tick();
	} // tick
	
	void parse_commandline(int argc, char** argv)
	{
		const char* arg;
		for(int i = 0; i < argc; ++i)
		{
			arg = argv[i];
			if (std::string(arg) == "-game")
			{
				_internal::game_path = std::string(argv[i+1]);
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


}; // namespace kernel