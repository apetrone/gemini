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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "arg.h"

#ifdef __cplusplus
extern "C" {
#endif

unsigned int next_cmdparam = 0;
arg_t cmd[ ARG_MAX_PARAMS ];

const char * ARG_VALUE_TRUE = "true";
const char * ARG_VALUE_FALSE = "false";


const char * arg_get( const char * name )
{
	int i;
	arg_t * param;

	// loop through all params and see if any are missing
	for( i = 0; i < ARG_MAX_PARAMS; ++i )
	{
		param = &cmd[ i ];
		if ( param->name == 0 )
			break;

		if ( strncmp( param->name, name, strlen(param->name) ) == 0 )
		{
			return param->string;
		}
	}

	return 0;
}

arg_t *arg_add( const char * name, const char * shortname, const char * longname, int flags, arg_t * parent )
{
	size_t snl = strlen(shortname);
	size_t lnl = 0;
	arg_t * param;

	if ( next_cmdparam >= ARG_MAX_PARAMS )
	{
		printf( "Reached ARG_MAX_PARAMS!\n" );
		return 0;
	}

	if (longname)
	{
		lnl = strlen(longname);
		if (lnl > ARG_MAX_LONGNAME)
		{
			printf( "longname is too long\n" );
			return 0;
		}
	}

	if (snl > ARG_MAX_SHORTNAME)
	{
		printf( "shortname is too long\n" );
		return 0;
	}

	param = &cmd[ next_cmdparam++ ];
	if ( param )
	{
		param->shortname = shortname;
		param->longname = longname;
		param->name = name;
		param->flags = flags;
		param->parent = parent;
	}

	return param;
}

int arg_parse( int argc, char ** argv )
{
	int i;
	int j;
	int state = 0;
	// state is 0 when looking for '-'
	// state is 1 when looking for params

	arg_t * param = 0;
	//printf( "Args: %i\n", argc );

	for( i = 1; i < argc; )
	{
		//printf( "arg[%i] = %s\n", i, argv[i] );

		if ( argv[i][0] == '-' )
		{
			if ( state == 1 && !(param->flags & ARG_NO_PARAMS) )
			{
				printf( "Missing parameter for %s\n", argv[i-1] );
				break;
			}

			// loop through all commands and check for a match
			for( j = 0; j < next_cmdparam; ++j )
			{
				param = &cmd[ j ];
				if ( strncmp( argv[i], param->shortname, strlen(param->shortname) ) == 0 )
				{
					// this parameter requires no parameters
					if ( (param->flags & ARG_NO_PARAMS) )
					{
						// thus, this parameter has been found and we're done.
						param->flags |= ARG_FOUND;
						param->string = ARG_VALUE_TRUE;
						param->integer = 1;
						param->value = 1;
					}
					else
					{
						// this option requires additional parameters; look for those now
						state = 1;
						break;
					}
				}
			}
		}
		else
		{
			if ( param )
			{
				if ( !(param->flags & ARG_FOUND) )
				{
					param->flags |= ARG_FOUND;
					param->string = argv[i];
					param->integer = atoi( param->string );
					param->value = atof( param->string );
					//printf( "Value for [%s] is \"%s\"\n", param->name, param->string );
				}
				else
				{
					printf( "Option accepts no arguments; ignoring" );
				}

				state = 0;
			}
			else
			{
				printf( "Looking for parameter for uknown argument!\n" );
				state = 0;
			}
		}

		++i;
	}

	if ( state == 1 && !(param->flags & ARG_NO_PARAMS) )
	{
		printf( "Missing parameter for option %s\n", argv[i-1] );
		return 1;
	}


	// loop through all params and see if any are missing
	for( i = 0; i < ARG_MAX_PARAMS; ++i )
	{
		int found;
		int required;
		param = &cmd[ i ];
		if ( param->name == 0 )
			break;

		found = param->flags & ARG_FOUND;
		required = !(param->flags & ARG_NOT_REQUIRED);
		
		// override/support parent arguments: if this parameter has a parent and the parent is NOT REQUIRED...
		if ( param->parent && (param->parent->flags & ARG_NOT_REQUIRED) )
		{
			// if the parent was NOT found, this parameter is not required
			if ( !(param->parent->flags & ARG_FOUND) )
			{
				required = 0;
			}
		}
		
		
		if ( !found && required )
		{
			if ( param->shortname != 0 && param->longname != 0 )
				printf( "Missing required option: %s (%s/%s)\n", param->name, param->longname, param->shortname );
			else if ( param->shortname != 0 )
				printf( "Missing required option: %s (%s)\n", param->name, param->shortname );
			else if ( param->longname != 0 )
				printf( "Missing required option: %s (%s)\n", param->name, param->longname );
			else
				printf( "Missing required option: %s\n", param->name );

			return 1;
		}
	}

	return 0;
}

#ifdef __cplusplus
}; // extern "C"
#endif