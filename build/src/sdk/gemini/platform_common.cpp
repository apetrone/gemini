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
#include <string.h> // for strlen, memset
#include <gemini/platform.h>

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
		
		
		void make_directories( const char * normalized_path )
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
					platform::path::make_directory( directory );
				}
				
				++path;
			}
		} // makeDirectories
	}; // namespace path
}; // namespace platform