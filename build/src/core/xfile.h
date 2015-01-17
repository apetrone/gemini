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
#pragma once

#include <string.h> // for size_t

#ifdef __cplusplus
extern "C"
{
#endif


typedef struct
{
	void * opaque;
} xfile_t;

enum
{
	XF_READ,
	XF_WRITE
};

enum
{
	XF_SEEK_BEGIN,
	XF_SEEK_RELATIVE,
	XF_SEEK_END
};

// opens a file
xfile_t xfile_open( const char * path, unsigned short filemode );

// returns number of bytes read into ptr
size_t xfile_read( xfile_t handle, void * ptr, size_t size, size_t count );

// set position of stream
int xfile_seek( xfile_t handle, long int offset, int origin );

// return the current position of the stream
long int xfile_tell( xfile_t handle );

// close a file handle
void xfile_close( xfile_t handle );

// returns 1 if handle is open or uninitialized
int xfile_isopen( xfile_t handle );

	
long int xfile_length( xfile_t handle );
	
// write an array of count elements, each one with size of size bytes from ptr
// total amount written is size*count
// returns total number of elements successfully written (if number != count, error)	
size_t xfile_write( xfile_t handle, const void * ptr, size_t size, size_t count );
	
#ifdef __cplusplus
}; // extern "C"
#endif
