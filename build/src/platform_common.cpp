#include "platform.hpp"
#include <string.h> // for strlen, memset
namespace platform
{
	namespace path
	{
		void normalize( char * path, size_t size )
		{
			while( *path )
			{
				if ( *path == '/' || *path == '\\' )
				{
					// conform to this platform's path separator
					*path = PATH_SEPARATOR;
				}
				
				++path;
			}
		} // normalize
		
		
		void makeDirectories( const char * normalized_path )
		{
			const char * path = normalized_path;
			char directory[ MAX_PATH_SIZE ];
			
			// don't accept paths that are too short
			if ( strlen( normalized_path ) < 2 )
			{
				return;
			}
			
			memset( directory, 0, MAX_PATH_SIZE );
			
			// loop through and call mkdir on each separate directory progressively
			while( *path )
			{
				if ( *path == PATH_SEPARATOR )
				{
					strncpy( directory, normalized_path, (path+1)-normalized_path );
					platform::path::makeDirectory( directory );
				}
				
				++path;
			}
		} // makeDirectories
	}; // namespace path
};
