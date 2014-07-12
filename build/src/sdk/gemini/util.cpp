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

#include <stdlib.h>

#include <gemini/util.h>


extern "C"
{
	//#include <murmur3.h>
};

namespace util
{
	unsigned int hash_32bit( const void * data, int data_size, unsigned int seed )
	{
		unsigned int hash;
		
		//MurmurHash3_x86_32( data, data_size, seed, &hash );
		
		return hash;
	} // hash_32bit


	
	void strip_shader_version( char * buffer, StackString<32> & version )
	{
		// remove preceding "#version" shader
		char * pos = xstr_str( buffer, "#version" );
		if ( pos )
		{
			char * end = pos;
			while( *end != '\n' )
				++end;
			
			version._length = (end-pos);
			memcpy( &version[0], &buffer[(pos-buffer)], version._length );
			memset( &buffer[(pos-buffer)], ' ', (end-pos) );
		}
	} // strip_shader_version
	
	float random_range( float min, float max )
	{
		return (float)rand() / RAND_MAX * (max - min) + min;
	} // random_range
}; // mamespace util