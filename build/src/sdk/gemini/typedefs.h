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

//
// Helpful Macros
//

/// make a string out of the input
#define STRINGIZE( x ) STRINGIZE_A( x )
#define STRINGIZE_A( x ) #x

/// concat two params
#define CONCAT( x, y ) x##y
/// concat two params with a space inbetween
#define CONCAT_SPACE( x, y ) x " " y
/// concat two params with a period inbetween
#define CONCAT_PERIOD( x, y ) x "." y

// This can be used to essentially remove macros by setting a macro's definition to this instead
// Ex: #define MyMacro(x) NULL_MACRO
#define NULL_MACRO (void(0))

// Third Level of indirection for names based on line numbers, based on GameDev.net
// daerid's code.
// This code allows us to create a UNIQUE identifier based on the line number its on ex: line_var_identifier_97
// Ex: type LINEVAR()

#define LINEVAR() _LINEVAR( __LINE__ )
#define _LINEVAR( line_num ) __LINEVAR( line_num )
#define __LINEVAR( line_num ) line_var_identifier_ ## line_num

//
// Import / Export Macros
//
#if defined( PLATFORM_WINDOWS )
	#define LIBRARY_EXPORT __declspec( dllexport )
	#define LIBRARY_IMPORT __declspec( dllimport )
	#define LIBRARY_CEXPORT extern "C" __declspec( dllexport )
#else
	#define LIBRARY_EXPORT
	#define LIBRARY_IMPORT
	#define LIBRARY_CEXPORT extern "C"
#endif

#if defined ( LIBRARY_SDK )
	#define	LIBRARY_SDK_EXPORT LIBRARY_EXPORT
	#define LIBRARY_SDK_CEXPORT LIBRARY_CEXPORT
#else
	#define LIBRARY_SDK_EXPORT LIBRARY_IMPORT
	#define LIBRARY_SDK_CEXPORT
#endif


// prefer the C99 types
#include <stdint.h>

#if _MSC_VER
	/// 16-bit char
	typedef wchar_t wchar;
#elif __GNUC__
	/// 16-bit char
	typedef int16_t wchar;
#endif


#include <gemini/config.h>
#include <assert.h>
#include <gemini/mem.h>
