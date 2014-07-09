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
#include <string.h> // for size_t

#include <xfile.h>
#include <gemini/config.h>

#if PLATFORM_APPLE
	#include <TargetConditionals.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#if (PLATFORM_WINDOWS || PLATFORM_LINUX || PLATFORM_APPLE || PLATFORM_ANDROID)

	#include <stdio.h>
	
	xfile_t xfile_open( const char * path, unsigned short filemode )
	{
		xfile_t f;
		const char * mode;
		switch( filemode )
		{
			case XF_READ: mode = "rb"; break;
			case XF_WRITE: mode = "wb"; break;
			default: mode = "rb"; break;
		}
		f.opaque = fopen( path, mode );
		
		return f;
	}
	
	size_t xfile_read( xfile_t handle, void * ptr, size_t size, size_t count )
	{
		return fread( ptr, size, count, (FILE*)handle.opaque );
	}
	
	int xfile_seek( xfile_t handle, long int offset, int origin )
	{
		int internal_origin = 0;
		switch( origin )
		{
			// set position to absolute value of offset
			case XF_SEEK_BEGIN: internal_origin = SEEK_SET; break;
			
			// set position to current position + offset
			case XF_SEEK_RELATIVE: internal_origin = SEEK_CUR; break;
			
			// set position to end of file
			case XF_SEEK_END: internal_origin = SEEK_END; break;
		}
		return fseek( (FILE*)handle.opaque, offset, internal_origin );
	}
	
	long int xfile_tell( xfile_t handle )
	{
		return ftell( (FILE*)handle.opaque );
	}
	
	void xfile_close( xfile_t handle )
	{
		fclose( (FILE*)handle.opaque );
	}
	
	int xfile_isopen( xfile_t handle )
	{
		return (handle.opaque != 0);
	}
	
	long int xfile_length( xfile_t handle )
	{
		long int size;
		xfile_seek( handle, 0, XF_SEEK_END );
		size = xfile_tell( handle );
		xfile_seek( handle, 0, XF_SEEK_BEGIN );
		return size;
	}
	
	size_t xfile_write( xfile_t handle, const void * ptr, size_t size, size_t count )
	{
		return fwrite( ptr, size, count, (FILE*)handle.opaque );
	}
#else
	#error Unknown platform!
#endif




#ifdef __cplusplus
}; // extern "C"
#endif
