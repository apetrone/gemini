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
#if 0
USAGE:
	args::Argument* arg_verbose;
	args::Argument* arg_file;

	arg_verbose = args::add( "verbose", "-v", "--verbosity", ARG_NO_PARAMS | ARG_NOT_REQUIRED );
	arg_file = args::add( "file", "-f", 0, 0 );
	
	if ( args::parse_args( argc, argv ) )
	{
		return 1;
	}

	if ( arg_verbose->integer )
	{
		printf( "Verbosity is ON\n" );
	}

#endif

#pragma once

namespace args
{
	static const int MAX_SHORTNAME = 4;
	static const int MAX_LONGNAME = 16;
	static const int MAX_PARAMS = 64;

	enum
	{
		// indicates this item was found in the argument list
		FOUND			= (1<<0),
		NO_PARAMS		= (1<<1),
		NOT_REQUIRED	= (1<<2)
	};

	struct Argument
	{
		int flags;
		int integer;
		float float_value;
		const char * name;
		const char * string;
		const char * shortname;
		const char * longname;
		Argument* parent;
		
		Argument() : flags(0), integer(0), float_value(0.0f), parent(0)
		{
		}
	};

	Argument* add(const char* name, const char* shortname, const char* longname, int flags = 0, Argument* parent = 0);
	
	bool parse_args(int argc, char** argv);
}
