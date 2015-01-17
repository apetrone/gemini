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

//
// Helpful Macros
//

/// make a string out of the input
#define STRINGIZE( x ) STRINGIZE_A( x )
#define STRINGIZE_A( x ) #x

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

//#include <platform/config.h>
#include <assert.h>
#include "mem.h"
