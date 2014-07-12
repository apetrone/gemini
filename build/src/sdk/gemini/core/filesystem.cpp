// -------------------------------------------------------------
// Copyright (C) 2012- Adam Petrone

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
#include <gemini/typedefs.h>
#include "filesystem.h"
#include <slim/xstr.h>
#include "platform.h"
#include <stdio.h> // for printf
#include <sys/stat.h> // for fs::FileExists
//#include "xfile.h"
//#include <slim/xlog.h>
#include <gemini/core/xfile.h>


namespace core
{
	namespace filesystem
	{
#if PLATFORM_IS_MOBILE


		void * mobile_audio_file_to_buffer( const char * filename, size_t & buffer_length );
	
#endif

#if PLATFORM_ANDROID
	#include <android/asset_manager.h>

	static AAssetManager * _asset_manager = 0;
	void set_asset_manager( AAssetManager * asset_manager )
	{
		_asset_manager = asset_manager;
	}

#endif
	}; // namespace filesystem
}; // namespace core



namespace core
{
	namespace filesystem
	{
		static char _root_directory[ MAX_PATH_SIZE ];
		void root_directory( char * path, int size )
		{
			memset( _root_directory, 0, MAX_PATH_SIZE );
			xstr_ncpy( _root_directory, path, xstr_len(path) );
		} // root_directory
		
		const char * root_directory()
		{
			return _root_directory;
		} // root_directory
		
		static char _content_directory[ MAX_PATH_SIZE ];
		void content_directory( const char * path, int size )
		{
			memset( _content_directory, 0, MAX_PATH_SIZE );
			xstr_ncpy( _content_directory, path, xstr_len(path) );
		} // content_directory

		const char * content_directory()
		{
			return _content_directory;
		} // content_directory

		void startup()
		{
			// TODO: create FileSystem instance?
		}
		
		void shutdown()
		{
			
		}

		void absolute_path_from_relative( char * fullpath, const char * relativepath, const char * content_directory )
		{
			if ( !content_directory )
			{
				content_directory = filesystem::content_directory();
			}
			
			size_t path_size = xstr_len(content_directory);
			xstr_ncpy( fullpath, content_directory, path_size );
			xstr_cat( fullpath, PATH_SEPARATOR_STRING );
			xstr_cat( fullpath, relativepath );
			platform::path::normalize( fullpath, path_size );
		} // absolute_path_from_relative

		void relative_path_from_absolute( char * relative_path, const char * absolute_path, const char * content_directory )
		{
			if ( !content_directory )
			{
				content_directory = filesystem::content_directory();
			}
			
			const char * temp = strstr(absolute_path, content_directory );

			size_t content_length = xstr_len(content_directory);
			if ( temp )
			{
				xstr_ncpy( relative_path, (absolute_path+content_length+1), xstr_len(absolute_path) - content_length);
				platform::path::normalize( relative_path, MAX_PATH_SIZE );
			}
		} // relative_path_from_absolute
		
		
		void truncate_string_at_path( char * path, const char * substr )
		{
			char * last;
			size_t len;
			size_t substr_len = xstr_len( substr );
			char buf[MAX_PATH_SIZE] = {0};
			len = xstr_len( path );
			last = path + len-1;
			for( int i = 0; path[i]; ++i )
			{
				char * p = strrchr( path, PATH_SEPARATOR );
				if ( p )
				{
					memset( buf, 0, MAX_PATH_SIZE );
					xstr_ncpy( buf, path + len - (path+len-1-p), last-p );
					
					last = p-1;
					*p = '\0';
					
					if ( xstr_nicmp( buf, substr, substr_len ) == 0 )
					{
						break;
					}
				}
			}
		} // truncate_string_at_path
		
		bool file_exists( const char * path, bool path_is_relative )
		{
			char fullpath[ MAX_PATH_SIZE ] = {0};
			int result = 0;
			
	#if PLATFORM_ANDROID
			// assume a relative path is always passed
			AAsset * asset = 0;
			asset = AAssetManager_open( _asset_manager, path, AASSET_MODE_BUFFER );
			if ( asset )
			{
				AAsset_close(asset);
				return 1;
			}
			
			return 0;
	#else
			struct stat stFileInfo;
			if ( path_is_relative )
			{
				filesystem::absolute_path_from_relative( fullpath, path );
			}
			else
			{
				xstr_ncpy( fullpath, path, -1 );
				platform::path::normalize( fullpath, MAX_PATH_SIZE );
			}
			result = stat( fullpath, &stFileInfo );
			return (result == 0) && ((stFileInfo.st_mode & S_IFMT) == S_IFREG);
	#endif
		} // file_exists
		
		bool directory_exists( const char * path, bool path_is_relative )
		{
			struct stat stFileInfo;
			int result = 0;
			char fullpath[ MAX_PATH_SIZE ] = {0};
			
			if ( path_is_relative )
			{
				filesystem::absolute_path_from_relative( fullpath, path );
			}
			else
			{
				xstr_ncpy( fullpath, path, -1 );
				platform::path::normalize( fullpath, MAX_PATH_SIZE );
			}
			result = stat( fullpath, &stFileInfo );
			return (result == 0) && ((stFileInfo.st_mode & S_IFMT) == S_IFDIR);
			return false;
		} // directory_exists
		
		char * file_to_buffer( const char * filename, char * buffer, size_t * buffer_length, bool path_is_relative )
		{
			size_t file_size;
			
			if ( !buffer_length )
			{
	//			LOGE( "ERROR: file_to_buffer called with INVALID value!\n" );
				return 0;
			}
			
			if ( !filesystem::file_exists(filename, true) )
			{
	//			LOGE( "File does not exist! \"%s\"\n", filename );
				return 0;
			}
			
	#if PLATFORM_ANDROID
			AAsset * asset = AAssetManager_open( _asset_manager, filename, AASSET_MODE_BUFFER );
			if ( asset )
			{
				file_size = AAsset_getLength(asset);
				
				if ( buffer && *buffer_length > 0 )
				{
					if ( file_size > *buffer_length )
					{
						printf( "Request to read file size larger than buffer! (%ld > %d)\n", file_size, *buffer_length );
						file_size = *buffer_length;
					}
				}
				
				*buffer_length = file_size;
				if ( !buffer )
				{
					buffer = (char*)ALLOC( (*buffer_length)+1 );
					memset( buffer, 0, (*buffer_length)+1 );
				}
				
				AAsset_read( asset, buffer, file_size );
				AAsset_close(asset);
			}

	#else
			char fullpath[ MAX_PATH_SIZE ] = {0};
			if ( path_is_relative )
			{
				filesystem::absolute_path_from_relative( fullpath, filename );
			}
			else
			{
				xstr_ncpy( fullpath, filename, -1 );
				platform::path::normalize( fullpath, MAX_PATH_SIZE );
			}

			xfile_t f = xfile_open( fullpath, XF_READ );
			if ( xfile_isopen( f ) )
			{
				xfile_seek( f, 0, XF_SEEK_END );
				file_size = xfile_tell( f );
				xfile_seek( f, 0, XF_SEEK_BEGIN );
				
				if ( buffer && *buffer_length > 0 )
				{
					if ( file_size > *buffer_length )
					{
						printf( "Request to read file size larger than buffer! (%ld > %zu)\n", file_size, *buffer_length );
						file_size = *buffer_length;
					}
				}
				
				*buffer_length = file_size;
				if ( !buffer )
				{
					buffer = (char*)ALLOC( (*buffer_length)+1 );
					memset( buffer, 0, (*buffer_length)+1 );
				}
				
				xfile_read( f, buffer, 1, file_size );			
				xfile_close( f );
			}
	#endif
			return buffer;
		} // file_to_buffer

		void * audiofile_to_buffer( const char * filename, size_t & buffer_length )
		{
	#if PLATFORM_APPLE && PLATFORM_IS_MOBILE
			return mobile_audio_file_to_buffer( filename, buffer_length );
	#else
	//		return file_to_buffer( filename, 0, &buffer_length );
	#warning fix this
			return 0;
	#endif
		} // audiofile_to_buffer

		int read_file_stats( const char * fullpath, FileStats & stats )
		{
	#if PLATFORM_LINUX || PLATFORM_ANDROID
			struct stat file_stats;
			stat( fullpath, &file_stats );

		#if PLATFORM_LINUX
				printf( "time: %i\n", file_stats.st_mtim.tv_sec );
		#else
				printf( "time: %i\n", file_stats.st_mtime );
		#endif
	#endif

			return 0;
		}

	}; // namespace filesystem
}; // namespace core
