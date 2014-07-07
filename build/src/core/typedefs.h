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

#undef uint8
#undef uint16
#undef uint32
#undef uint64
#undef int8
#undef int16
#undef int32
#undef int64
#undef float32
#undef float64

/// unsigned 8-bit int
typedef unsigned char uint8;
/// unsigned 16-bit int
typedef unsigned short uint16;
/// unsigned 32-bit int
typedef unsigned int uint32;

/// signed 8-bit int
typedef signed char int8;
/// signed 16-bit int
typedef signed short int16;
/// signed 32-bit int
typedef signed int int32;

/// 32-bit float
typedef float float32;
/// 64-bit float
typedef double float64;

#if _MSC_VER
	/// signed 64-bit int
	typedef signed __int64 int64;
	/// unsigned 64-bit int
	typedef unsigned __int64 uint64;
	/// 16-bit char
	typedef wchar_t wchar;
#elif __GNUC__
	/// signed 64-bit int
	typedef long long int int64;
	/// unsigned 64-bit int
	typedef unsigned long long int uint64;
	/// 16-bit char
	typedef int16 wchar;
#endif

#include <config.h>
#include <assert.h>
#include "memory.hpp"
