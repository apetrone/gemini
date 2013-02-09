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
#if 0
USAGE:
	xlib_t lib;
	xlib_open( &lib,  "/path/to/library.so" );
	void * symbol = xlib_find_symbol( &lib, "open_something" );
	xlib_close( &lib );

#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	void * handle;
} xlib_t;

// load a dynamic library at library_path
// returns 1 on success
int xlib_open( xlib_t * lib, const char * library_path );

// close a library handle
void xlib_close( xlib_t * lib );

// load a symbol from the library, returns 0 on failure
void * xlib_find_symbol( xlib_t * lib, const char * procname );


#ifdef __cplusplus
}; // extern "C"
#endif