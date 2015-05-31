// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//		* Redistributions of source code must retain the above copyright notice,
//		this list of conditions and the following disclaimer.

//		* Redistributions in binary form must reproduce the above copyright notice,
//		this list of conditions and the following disclaimer in the documentation
//		and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//		 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------

#include "xfile.h"

#include <platform/config.h>


#include <string.h> // for size_t

#if PLATFORM_APPLE
	#include <TargetConditionals.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_LINUX) || defined(PLATFORM_APPLE) || defined(PLATFORM_ANDROID)

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
