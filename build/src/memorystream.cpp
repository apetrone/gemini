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
#include <string.h> // for memcpy
#include <gemini/typedefs.h>
#include "memorystream.h"



MemoryStream::MemoryStream()
{
	this->init( 0, 0 );
} // MemoryStream

void MemoryStream::init( char * buffer, long buffer_size )
{
	data = buffer;
	data_size = buffer_size;
	offset = 0;
} // init

void MemoryStream::rewind()
{
	offset = 0;
} // rewind


int MemoryStream::read( void * destination, int num_bytes )
{
	if ( !data )
	{
		return 0;
	}
	
	// check for read violation
	assert( (offset + num_bytes) <= data_size );
	
	// copy memory and advance the pointer
	memcpy( destination, &data[offset], num_bytes );
	offset += num_bytes;
	
	return num_bytes;
} // read


long MemoryStream::offset_pointer() const
{
	return offset;
} // offset_pointer

int MemoryStream::write( const void * src, int num_bytes )
{
	if ( !data )
	{
		return 0;
	}
	
	// check for write violation
	assert( (offset + num_bytes) <= data_size );
	
	// copy memory and advance pointer
	memcpy( &data[offset], src, num_bytes );
	offset += num_bytes;

	return num_bytes;
} // write


void MemoryStream::seek( long requested_offset, bool is_absolute )
{
	if ( data )
	{
		if ( is_absolute )
		{
			offset = requested_offset;
		}
		else
		{
			offset += requested_offset;
		}
	}
} // seek

void MemoryStream::clear()
{
	memset( this->data, 0, this->data_size );
} // clear