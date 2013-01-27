#include "core.hpp"
#include "kernel.hpp"
#include "platform.hpp"
#include <string>
#include <map>

namespace kernel
{
	typedef std::map< std::string, Creator> KernelCreatorByString;
	KernelCreatorByString _kernel_creator_map;
	IKernel * _active_instance = 0;
	Params _kernel_params;

	
	namespace _internal
	{
		struct State
		{
			
		};
		
		static void registerKernelCreatorByName( const char * kernel_name, Creator creator )
		{
			_kernel_creator_map.insert( KernelCreatorByString::value_type( kernel_name, creator) );
		} // registerKernelCreatorByName
		
		static Creator findKernelCreatorByName( const char * kernel_name )
		{
			std::string kname = kernel_name;
			KernelCreatorByString::iterator it = _kernel_creator_map.find( kname );
			if ( it != _kernel_creator_map.end() )
			{
				return (*it).second;
			}
			
			return 0;
		} // findKernelCreatorByName
	};
	
	Registrar::Registrar( const char * kernel_name, Creator creator )
	{
		_internal::registerKernelCreatorByName( kernel_name, creator );
	}
	
	IKernel * instance()
	{
		return _active_instance;
	}
	
	
	Error startup( int argc, char ** argv, const char * kernel_name )
	{
		Creator creator = 0;

		// initialize kernel's timer
//		xtime_startup( &_kernelState._clock );
		
		// startup duties; lower-level system init
		core::Error core_error = core::startup();
		if ( core_error.failed() )
		{
			fprintf( stderr, "FATAL ERROR: %s\n", core_error.message );
			return kernel::CoreFailed;
		}
		
		// if there is no kernel_name provided, report an error
		if ( !kernel_name )
		{
			fprintf( stdout, "You must specify a kernel on the command line.\n\n" );
			return kernel::NoKernel;
		}
		
		// search for the named kernel instance (passed on the command line or overriden above)
		creator = _internal::findKernelCreatorByName( kernel_name );
		if ( !creator )
		{
			fprintf( stdout, "Unable to load kernel: \"%s\", aborting!\n", kernel_name );
			core::shutdown();
			return kernel::NotFound;
		}
		
		// create an instance of the kernel
		_active_instance = creator();
		if ( !_active_instance )
		{
			fprintf( stdout, "No kernel instance. Aborting!\n" );
			core::shutdown();
			return kernel::NoInstance;
		}
				
		return kernel::NoError;
	}
	
	void shutdown()
	{
		// shutdown, cleanup
		if ( _active_instance )
		{
			_active_instance->shutdown();
		
			// delete kernel instance
			delete _active_instance;
			_active_instance = 0;
		}
		
		// system cleanup
		core::shutdown();
	}

	void tick()
	{
//		sys::pre_frame();
		_active_instance->tick( _kernel_params );
//		sys::post_frame();
	}

	Error main( int argc, char ** argv, const char * kernel_name )
	{
		Error error = startup( argc, argv, kernel_name );
		if ( error != kernel::NoError )
		{
			fprintf( stderr, "Kernel startup failed with kernel code: %i\n", error );
		}
		else
		{
			// set window attribs
			int config_result = _active_instance->config( _kernel_params );
			if ( config_result == kernel::Success )
			{
//				sys::create_window( _kernel_params.window_width, _kernel_params.window_height, _kernel_params.window_title );
			}
			else if ( config_result == kernel::Failure )
			{
				fprintf( stdout, "Config failed. aborting.\n" );
				return kernel::ConfigFailed;
			}
			
			// kernel instance failed startup
			int startup_result = _active_instance->startup( _kernel_params );
			if ( startup_result == kernel::Failure )
			{
				fprintf( stdout, "kernel startup failed!\n" );
				return kernel::StartupFailed;
			}
			
			// startup succeeded; enter main loop
			if ( startup_result == kernel::Success )
			{
				_kernel_params.is_active = true;
				
				// main loop, kernels can modify is_active.
				while( _kernel_params.is_active )
				{
					tick();
				}
			}
		}

		// cleanup kernel memory
		shutdown();
		
		return error;
	} // main

}; // namespace kernel