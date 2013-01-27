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
