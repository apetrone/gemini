// -------------------------------------------------------------
// Copyright (C) 2004-2011 Adam Petrone

//	This file is part of the aengine source code.
//
//  aengine is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.

//  This source is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//  GNU General Public License for more details.

//  You should have received a copy of the GNU General Public License
//  along with this source.  If not, see <http://www.gnu.org/licenses/>.
// -------------------------------------------------------------
#if 0
USAGE:
	arg_t * arg_verbose;
	arg_t * arg_file;

	arg_verbose = arg_add( "verbose", "-v", "--verbosity", ARG_NO_PARAMS | ARG_NOT_REQUIRED );
	arg_file = arg_add( "file", "-f", 0, 0 );
	
	if ( arg_parse( argc, argv ) )
	{
		return 1;
	}

	if ( arg_verbose->integer )
	{
		printf( "Verbosity is ON\n" );
	}

#endif

#pragma once

#define ARG_MAX_SHORTNAME 4
#define ARG_MAX_LONGNAME 16
#define ARG_MAX_PARAMS 64

enum
{
	// indicates this item was found in the argument list
	ARG_FOUND			= (1<<0),
	ARG_NO_PARAMS		= (1<<1),
	ARG_NOT_REQUIRED	= (1<<2)
};

typedef struct arg_s
{
	int integer;
	float value;
	const char * name;
	const char * string;
	int flags;
	const char * shortname;
	const char * longname;
	struct arg_s * parent;
} arg_t;

#ifdef __cplusplus
extern "C" {
#endif


extern const char * ARG_VALUE_TRUE;
extern const char * ARG_VALUE_FALSE;


int arg_parse( int argc, char ** argv );
arg_t *arg_add( const char * name, const char * shortname, const char * longname, int flags, arg_t * parent );
const char * arg_get( const char * name );

#ifdef __cplusplus
}; // extern "C"
#endif