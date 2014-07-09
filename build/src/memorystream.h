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

class MemoryStream
{
	char * data;
	long data_size;
	long offset;
	
public:
	MemoryStream();
	~MemoryStream() {}

	// initialize this stream with initial data
	void init( char * buffer, long _data_size );
	
	// reset the stream offset to zero
	void rewind();
	
	// read a number of bytes into the destination buffer
	// returns the number of bytes read
	int read( void * destination, int num_bytes );
	
	// write data to the stream
	// returns the number of bytes written
	int write( const void * src, int num_bytes );
	
	// seek to an offset location within the stream
	// if is_absolute is false, the seek happens relative to the current position
	void seek( long offset, bool is_absolute );
	
	// returns the position of the read/write pointer offset
	long offset_pointer() const;
	
	template <class Type>
	int write( const Type & value )
	{
		return this->write( &value, sizeof(Type) );
	} // write, Type
	
	
	template <class Type>
	int read( Type & destination )
	{
		return this->read( &destination, sizeof(Type) );
	} // read, Type
	
	// clear the stream's memory
	void clear();
	
	
	
	
}; // class MemoryStream
