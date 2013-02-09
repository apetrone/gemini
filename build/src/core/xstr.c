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
#include "xstr.h"

#include <string.h>
#include <stdio.h> // for vsprintf

#define XSTR_BUFFER_SIZE 8192
#define XSTR_MAX_BUFFERS 2

#ifdef __cplusplus
extern "C"
{
#endif

static unsigned int xstr_current_buffer = 0;
static char xstr_buffers[ XSTR_MAX_BUFFERS ][ XSTR_BUFFER_SIZE ];



char * xstr_format( const char * format, ... )
{
	va_list arg;

	char * tmp = &xstr_buffers[ xstr_current_buffer ][0];

	va_start( arg, format );
	xstr_vsnprintf( tmp, XSTR_BUFFER_SIZE, format, arg );
	va_end( arg );

	++xstr_current_buffer;
	xstr_current_buffer = (xstr_current_buffer % XSTR_MAX_BUFFERS);

	return tmp;
} // xstr_format

int xstr_sprintf( char * destination, size_t destination_max_size, const char * format, ... )
{
	va_list arg;
	int result;

	va_start( arg, format );
	result = xstr_vsnprintf( destination, destination_max_size, format, arg );
	va_end( arg );

	return result;
} // xstr_sprintf


int xstr_vsnprintf( char * destination, size_t destination_max_size, const char * format, va_list arg )
{
	return vsnprintf( destination, destination_max_size, format, arg );
} // xstr_vsnprintf

char * xstr_ncpy( char * destination, const char * source, size_t source_bytes )
{
	if (source_bytes == -1)
	{
		source_bytes = xstr_len( source );
	}

	return strncpy( destination, source, source_bytes );
} // xstr_ncpy

size_t xstr_len( const char * str )
{
	return strlen( str );
} // xstr_len


char * xstr_cat( char * destination, const char * source )
{
    return strcat( destination, source );
}
// xstr_cat

// this function is from paul nettle's memory manager
const char * xstr_filefrompath( const char * path )
{
	// check backslash and forward slash
	const char * pos = strrchr( path, '\\' ); // look for backward slash

	if ( pos )
		return (pos + 1);

	pos = strrchr( path, '/' ); // look for fwd slash

	if ( pos )
		return (pos + 1);

	return path;
} // xstr_filefrompath

int xstr_nicmp( const char * s1, const char * s2, size_t count )
{
	size_t s1_len;
	size_t s2_len;	
	
	// size_t is implemented as 'unsigned int' on some platforms (Windows...)
	// use zero to mean: pick the smallest string
	if ( count == 0 )
	{
		s1_len = xstr_len(s1);
		s2_len = xstr_len(s2);
		
		if ( s1_len < s2_len )
			return -1;
		else if ( s1_len > s2_len )
			return 1;

		count = s1_len;
	}
#if LINUX || __APPLE__
    return strncasecmp( s1, s2, count );
#else
    return strnicmp( s1, s2, count );
#endif
} // xstr_nicmp

char * xstr_str( const char * s1, const char * s2 )
{
	return strstr( s1, s2 );
} // xstr_str
	
char * xstr_ncat( char * destination, const char * source, size_t destination_size )
{
	size_t dst_len = xstr_len(destination);
	size_t src_len = xstr_len(source);

	// we cannot possibly fit this string in the destination array
	if ( dst_len + src_len >= destination_size )
		return 0;
	
    return strcat( destination, source );
}
// xstr_ncat

#ifdef __cplusplus
}
#endif
