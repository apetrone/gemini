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

#include <string.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C"
{
#endif

// returns a printf-style formatted string
char * xstr_format( const char * format, ... );


// C standard functions
int xstr_sprintf( char * destination, size_t destination_max_size, const char * format, ... );
int xstr_vsnprintf( char * destination, size_t destination_max_size, const char * format, va_list arg );
char * xstr_ncpy( char * destination, const char * source, size_t source_bytes );
size_t xstr_len( const char * str );
char * xstr_cat( char * destination, const char * source );
char * xstr_ncat( char * destination, const char * source, size_t destination_size );
int xstr_nicmp( const char * s1, const char * s2, size_t count );
char * xstr_str( const char * s1, const char * s2 );
	
// return just the filename portion of a path
const char * xstr_filefrompath( const char * path );

#ifdef __cplusplus
}
#endif
