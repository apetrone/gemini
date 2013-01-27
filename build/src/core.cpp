#include "core.hpp"
#include "platform.hpp"

namespace core
{
	Error::Error( int error_status, const char * error_message ) :
		status(error_status), message(error_message)
	{
	}
	
	Error startup()
	{
		core::Error error = platform::startup();
		if ( error.failed() )
		{
			fprintf( stderr, "platform startup failed! %s\n", error.message );
			return error;
		}
		
		return error;
	} // startup
	
	
	void shutdown()
	{
		platform::shutdown();
	}
}