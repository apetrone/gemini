// -------------------------------------------------------------
// Copyright (C) 2004- Adam Petrone

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

namespace args
{
	uint8_t next_param = 0;
	Argument argument_list[ MAX_PARAMS ];
	
	const char* ARG_VALUE_TRUE = "true";
	const char* ARG_VALUE_FALSE = "false";

	Argument* add(const char* name, const char* shortname, const char* longname, int flags, Argument* parent)
	{
		size_t snl = strlen(shortname);
		size_t lnl = 0;
		Argument* param = nullptr;
		
		if (next_param >= MAX_PARAMS)
		{
			fprintf(stderr, "Reached args::MAX_PARAMS of %i!\n", MAX_PARAMS);
			return 0;
		}
		
		if (longname)
		{
			lnl = strlen(longname);
			if (lnl > MAX_LONGNAME)
			{
				fprintf(stderr, "longname is too long\n");
				return 0;
			}
		}
		
		if (snl > MAX_SHORTNAME)
		{
			fprintf(stderr, "shortname is too long\n");
			return 0;
		}
		
		param = &argument_list[next_param++];
		if (param)
		{
			param->shortname = shortname;
			param->longname = longname;
			param->name = name;
			param->flags = flags;
			param->parent = parent;
		}
		
		return param;
	}
	
	bool split_arguments(int argc, char** argv)
	{
		// state is 0 when looking for '-'
		// state is 1 when looking for parameters
		uint8_t state = 0;
		Argument* param = nullptr;
		int index = 1;
		
		for ( ; index < argc; )
		{
			if (argv[index][0] == '-')
			{
				if (state == 1 && !(param->flags & NO_PARAMS))
				{
					fprintf(stderr, "Missing parameter for %s\n", argv[index-1]);
					break;
				}
				
				// loop through all commands and check for a match
				for (int command = 0; command < next_param; ++command)
				{
					param = &argument_list[command];
					if (strncmp(argv[index], param->shortname, strlen(param->shortname)) == 0)
					{
						// this requires no parameters
						if (param->flags & NO_PARAMS)
						{
							// thus, this parameter has been found and it's complete.
							param->flags |= FOUND;
							param->string = ARG_VALUE_TRUE;
							param->integer = 1;
							param->float_value = 1;
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
				if (param)
				{
					if (!(param->flags & FOUND))
					{
						param->flags |= FOUND;
						param->string = argv[index];
						param->integer = atoi(param->string);
						param->float_value = atof(param->string);
					}
					else
					{
						fprintf(stderr, "Option \"%s\" accepts no arguments; ignoring\n", param->name);
					}
					
					state = 0;
				}
				else
				{
					fprintf(stderr, "Unexpected argument encountered!\n");
					state = 0;
				}
			}
			
			++index;
		}
		
		// if we're in this limbo state, there's a missing parameter/option
		if (state == 1 && !(param->flags & NO_PARAMS))
		{
			fprintf(stderr, "Missing parameter for option %s\n", argv[index-1]);
			return false;
		}
		
		return true;
	}
	
	bool validate_arguments(int argc, char** argv)
	{
		// returns true if:
		// 1. All required arguments were found
		
		int index = 0;
		Argument* param = nullptr;
		
		// loop through all params and see if any are missing
		for( ; index < MAX_PARAMS; ++index)
		{
			int found;
			int required;
			param = &argument_list[index];
			if (param->name == 0)
				break;
			
			found = param->flags & FOUND;
			required = !(param->flags & NOT_REQUIRED);
			
			// override/support parent arguments: if this parameter has a parent and the parent is NOT REQUIRED...
			if (param->parent && (param->parent->flags & NOT_REQUIRED))
			{
				// if the parent was NOT found, this parameter is not required
				if (!(param->parent->flags & FOUND))
				{
					required = 0;
				}
			}
			
			if (!found && required)
			{
				if (param->shortname != 0 && param->longname != 0)
				{
					fprintf(stderr, "Missing required option: %s (%s/%s)\n", param->name, param->longname, param->shortname);
				}
				else if (param->shortname != 0)
				{
					fprintf(stderr, "Missing required option: %s (%s)\n", param->name, param->shortname);
				}
				else if (param->longname != 0)
				{
					fprintf(stderr, "Missing required option: %s (%s)\n", param->name, param->longname);
				}
				else
				{
					fprintf(stderr, "Missing required option: %s\n", param->name);
				}
				
				return false;
			}
		}
		
		return true;
	}
	
	
	bool parse_args(int argc, char** argv)
	{
		if (!split_arguments(argc, argv))
		{
			return false;
		}
		
		return validate_arguments(argc, argv);
	}
}