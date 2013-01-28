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
#include "core.hpp"
#include "kernel.hpp"
#include "platform.hpp"
#include <string>
#include <map>

#include "xtime.h"

namespace kernel
{
	typedef std::map< std::string, ApplicationCreator> ApplicationCreatorByString;
	ApplicationCreatorByString _application_creator_map;
	IKernel * _kernel = 0;
	IApplication * _active_application = 0;

	namespace _internal
	{
		struct State
		{
			xtime_t timer;
		};
		
		State _kernel_state;
		
		static void register_application_by_name( const char * kernel_name, ApplicationCreator ApplicationCreator )
		{
			_application_creator_map.insert( ApplicationCreatorByString::value_type( kernel_name, ApplicationCreator) );
		} // registerKernelApplicationCreatorByName
		
		static ApplicationCreator find_application_by_name( const char * kernel_name )
		{
			std::string kname = kernel_name;
			ApplicationCreatorByString::iterator it = _application_creator_map.find( kname );
			if ( it != _application_creator_map.end() )
			{
				return (*it).second;
			}
			
			return 0;
		} // findKernelApplicationCreatorByName
	};
	
	Registrar::Registrar( const char * kernel_name, ApplicationCreator ApplicationCreator )
	{
		_internal::register_application_by_name( kernel_name, ApplicationCreator );
	}
	
	IKernel * instance()
	{
		return _kernel;
	}
	
	core::Error load_application( const char * application_name )
	{
		core::Error error(0);
		
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
	}
	
	
	Error startup( int argc, char ** argv, IKernel * kernel_instance, const char * application_name )
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
		params.argc = argc;
		params.argv = argv;
		params.error_message = 0;
		params.device_flags = 0;
		params.window_width = 0;
		params.window_height = 0;
		params.has_window = 0;
		
		// initialize kernel's timer
		xtime_startup( &_internal::_kernel_state.timer );
		
		// startup duties; lower-level system init
		core::Error core_error = core::startup();
		if ( core_error.failed() )
		{
			fprintf( stderr, "Fatal error: %s\n", core_error.message );
			core::shutdown();
			return kernel::CoreFailed;
		}
		
		// load application
		core_error = load_application( application_name );
		if ( core_error.failed() )
		{
			fprintf( stderr, "Fatal error loading application '%s' -> %s, aborting.\n", application_name, core_error.message );
			return kernel::ApplicationFailure;
		}
		
		// application config
		int config_result = _active_application->config( kernel::instance()->parameters() );
		if ( config_result == kernel::Success )
		{
			// call this event on the kernel
			kernel::Error kernel_error = kernel::instance()->post_application_config();
			
			// check for kernel error on post_application_config
			if ( kernel_error != kernel::NoError )
			{
				fprintf( stderr, "Fatal error in kernel post_application_config: %i\n", kernel_error );
				return kernel::PostConfig;
			}
		}
		else if ( config_result == kernel::Failure )
		{
			fprintf( stderr, "Application config failed. aborting.\n" );
			return kernel::ConfigFailed;
		}
		
		// application instance failed startup
		int startup_result = _active_application->startup( kernel::instance()->parameters() );
		if ( startup_result == kernel::Failure )
		{
			fprintf( stderr, "Application startup failed!\n" );
			return kernel::StartupFailed;
		}
		
		// set has window param (kind of hacky right now)
		params.has_window = (startup_result != kernel::NoWindow);

		return kernel::NoError;
	} // startup
	
	void shutdown()
	{
		// shutdown, cleanup
		if ( _kernel )
		{
			_kernel = 0;
		}
		
		// system cleanup
		core::shutdown();
	}

	void tick()
	{
		_kernel->pre_tick();
		_active_application->tick( _kernel->parameters() );
		_kernel->post_tick();
	}

}; // namespace kernel