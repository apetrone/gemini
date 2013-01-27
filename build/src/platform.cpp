#include "core.hpp"
#include "platform.hpp"
#include "platform_common.hpp"

#if TARGET_OS_MAC
	#include "osx/osx_platform.h"
#endif

#if _WIN32
	#include <windows.h>
	#include <direct.h> // for _mkdir
#elif LINUX
	#include <sys/sysinfo.h>
	//#include <errno.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <stdio.h> // for snprintf
	#include <stdlib.h> // for abort

#elif __APPLE__
	#include <stdio.h>
	#include <sys/stat.h>
	#include <sys/types.h>
	#include <inttypes.h>
	#include <stdlib.h> // for realpath
	#include <unistd.h>	// for fork
#endif

#include <string> // for strrchr

namespace platform
{
	core::Error startup()
	{
		core::Error error(0);
		
#if __APPLE__
		error = osx_startup();
#endif
		
		return error;
	}
	
	void shutdown()
	{
#if __APPLE__
		osx_shutdown();
#endif
	}
	
	core::Error programDirectory( char * path, size_t size )
	{
		core::Error error(0);
		int result = 0;
		char * sep;
		
#if _WIN32
		result = GetModuleFileNameA( GetModuleHandleA(0), path, size);
		if ( result != 0 )
		{
			error.status = core::Error::Failure;
			error.message = "GetModuleFileNameA failed!";
		}
		
#elif LINUX
		{
			// http://www.flipcode.com/archives/Path_To_Executable_On_Linux.shtml
			char linkname[ 64 ] = {0};
			pid_t pid;
			
			
			pid = getpid();
			
			if ( snprintf(linkname, sizeof(linkname), "/proc/%i/exe", pid ) < 0 )
			{
				abort();
			}
			
			result = readlink( linkname, path, size );
			
			if ( result == -1 )
			{
				error.status = core::Error::Failure;
				error.message = "readlink failed";
			}
			else
			{
				path[result] = 0;
			}
		}
#endif
		
		if ( result != 0 )
		{
			sep = strrchr( path, PATH_SEPARATOR );
			
			if ( sep )
			{
				*sep = '\0';
			}
		}
		
#if __APPLE__
		error = osx_programDirectory( path, size );
#endif
		return error;
	}
	
	
	namespace path
	{		
		core::Error makeDirectory( const char * path )
		{
			core::Error error(0);
			int result = 0;
			
#if _WIN32
			result = _mkdir( path );
			if ( result == -1 )
			{
				// TODO: print out the errno
				error = core::Error( core::Error::Failure, "_mkdir failed!" );
			}
#elif LINUX || __APPLE__
			// http://pubs.opengroup.org/onlinepubs/009695399/functions/mkdir.html
			result = mkdir( path, (S_IRUSR | S_IWUSR | S_IXUSR ) | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH );
			if ( result == -1 )
			{
				// TODO: print out the errno
				error = core::Error( core::Error::Failure, "mkdir failed!" );
			}
#endif
			
			return error;
		} // makeDirectory
	}; // namespace path
}; // namespace platform