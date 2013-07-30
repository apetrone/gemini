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
#include "typedefs.h"
#include "core.hpp"
#include "kernel.hpp"
#include "platform.hpp"
#include <string>
#include <map>

#include <slim/xlog.h>
#include "xtime.h"
#include "renderer.hpp"
#include "audio.hpp"
#include "input.hpp"
#include "assets.hpp"
#include "configloader.hpp"
#include "font.hpp"

#if LINUX
	#include <stdlib.h> // for qsort
#endif

namespace kernel
{
	// This is NOT hooked up to the memory allocator because this is accessed before the memory allocator is initialized.
	typedef std::map< std::string, ApplicationCreator> ApplicationCreatorByString;
	
	IKernel * _kernel = 0;
	IApplication * _active_application = 0;

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
		
		
		struct BootConfig
		{
			StackString<64> kernel_name;
		};
		
		
		util::ConfigLoadStatus boot_conf_loader( const Json::Value & root, void * data )
		{
			BootConfig * cfg = (BootConfig*)data;
			if ( !cfg )
			{
				return util::ConfigLoad_Failure;
			}
			
			LOGV( "loading boot.conf...\n" );
			
			cfg->kernel_name = root["kernel_name"].asString().c_str();
		
		
			return util::ConfigLoad_Success;
		}
		
		bool load_boot_config( BootConfig & boot_config )
		{
			bool success = util::json_load_with_callback( "conf/boot.conf", boot_conf_loader, &boot_config, true );
			if ( !success )
			{
				LOGV( "Unable to locate boot.conf.\n" );
				// load sane defaults
				boot_config.kernel_name = "HelloWorld";
			}
			
			return success;
		} // load_boot_config
		
		


		
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
	
	core::Error load_application( const char * application_name )
	{
		core::Error error(0);
		LOGV( "Loading kernel '%s'\n", application_name );
		
		// search for the named kernel instance (passed on the command line or overriden above)
		ApplicationCreator creator = _internal::find_application_by_name( application_name );
		if ( creator )
		{
			// create an instance of the kernel
			_active_application = creator();
			if ( !_active_application )
			{
				fprintf( stdout, "Unable to create an instance of application: \"%s\", aborting!\n", application_name );
				error = core::Error( core::Error::Failure, "Cannot " );
			}
		}
		else
		{
			fprintf( stdout, "Application named \"%s\" not found, aborting!\n", application_name );
			error = core::Error( core::Error::Failure, "Application not found by name" );
		}
		
		return error;
	} // load_application
	
	
	Error startup( IKernel * kernel_instance )
	{
		// set instance
		_kernel = kernel_instance;
		if ( !_kernel )
		{
			fprintf( stderr, "No valid kernel instance found\n" );
			return kernel::NoInstance;
		}
		
		// setup parameters
		kernel::Params & params = kernel_instance->parameters();
		params.error_message = 0;
		params.device_flags = 0;
		params.window_width = 0;
		params.window_height = 0;
		params.step_alpha = 0;
		params.step_interval_seconds = 1/60.0f;
		
		// the kernel is ACTIVE here; callbacks after config/start may modify this
		_kernel->set_active( true );
		
		// initialize kernel's timer
		xtime_startup( &_internal::_kernel_state.timer );
		_internal::_kernel_state.last_time = xtime_msec( &_internal::_kernel_state.timer );
		_internal::_kernel_state.accumulator = 0;

		
		// perform any startup duties here before we init the core
		_kernel->startup();
		
		// startup duties; lower-level system init
		core::Error core_error = core::startup();
		if ( core_error.failed() )
		{
			fprintf( stderr, "Fatal error: %s\n", core_error.message );
			core::shutdown();
			return kernel::CoreFailed;
		}
		
		// ask the kernel to register services
		_kernel->register_services();
		
		// load boot config
		_internal::BootConfig boot_config;
		_internal::load_boot_config( boot_config );
		
		// load application
		core_error = load_application( boot_config.kernel_name() );
		if ( core_error.failed() )
		{
			fprintf( stderr, "Fatal error loading application '%s' -> %s, aborting.\n", boot_config.kernel_name(), core_error.message );
			return kernel::ApplicationFailure;
		}
		
		// application config
		ApplicationResult config_result = Application_Failure;
		if ( _active_application )
		{
			config_result = _active_application->config( kernel::instance()->parameters() );
		}
		
		// evaluate config result
		kernel::instance()->post_application_config( config_result );
	
		if ( config_result == kernel::Application_Failure )
		{
			fprintf( stderr, "Application config failed. aborting.\n" );
			return kernel::ConfigFailed;
		}
		
		// try to setup the renderer
		if ( config_result != kernel::Application_NoWindow )
		{
			int render_result =	renderer::startup( renderer::Default );
			if ( render_result == 0 )
			{
				LOGE( "renderer initialization failed!\n" );
				return kernel::RendererFailed;
			}

			assets::startup();
			font::startup();
		}
		
		// try to setup audio
		audio::startup();
		input::startup();
		
		// application instance failed startup
		ApplicationResult startup_result = _active_application->startup( kernel::instance()->parameters() );
		
		// evaluate startup result
		_kernel->post_application_startup( startup_result );
		
		if ( startup_result == kernel::Application_Failure )
		{
			fprintf( stderr, "Application startup failed!\n" );
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
		
		_kernel->parameters().framedelta_filtered = _internal::_kernel_state.tsa.filtered_value;
		_kernel->parameters().framedelta_raw = raw_delta_msec;
		
		_internal::_kernel_state.accumulator += (_internal::_kernel_state.tsa.filtered_value * .001);
//		while( _internal::_kernel_state.accumulator >= _kernel->parameters().step_interval_seconds )
		{
			_internal::_kernel_state.accumulator -= _kernel->parameters().step_interval_seconds;
			_kernel->parameters().step_alpha = 0.0f; // _internal::_kernel_state.accumulator / _kernel->parameters().step_interval_seconds;
			if ( _kernel->parameters().step_alpha >= 1.0f )
			{
				_kernel->parameters().step_alpha -= 1.0f;
			}
			
			_active_application->step( _kernel->parameters() );
		}
	} // update

	void tick()
	{
		audio::update();
		input::update();

		update();
		_kernel->pre_tick();
		_active_application->tick( _kernel->parameters() );
		_kernel->post_tick();
	} // tick
	
	
	void assign_listener_for_eventtype( kernel::EventType event_type, void * listener )
	{
		_internal::_event_hooks.events[ event_type ] = listener;
	} // assign_listener_for_eventtype
	
	void * find_listener_for_eventtype( kernel::EventType event_type )
	{
		return _internal::_event_hooks.events[ event_type ];
	} // find_listener_for_eventtype


}; // namespace kernel