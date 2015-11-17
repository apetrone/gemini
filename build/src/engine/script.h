// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//      * Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.

//      * Redistributions in binary form must reproduce the above copyright notice,
//      this list of conditions and the following disclaimer in the documentation
//      and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//       SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------
#pragma once

#include <squirrel.h>
#include <sqstdio.h>
#include <sqstdaux.h>
#include <sqstdsystem.h>
#include <sqstdmath.h>

#include <sqrat.h>

namespace script
{
	const int64_t MINIMUM_STACK_SIZE = 128;
	void startup( int64_t stack_size = script::MINIMUM_STACK_SIZE );
	void shutdown();

	const char * string_for_type( int sqtype );
	void print_stack( HSQUIRRELVM vm );

	HSQUIRRELVM get_vm();

	template <class Type>
	void get_variable( const SQChar * name, Type & value )
	{
		Sqrat::Object obj = Sqrat::RootTable( get_vm() ).GetSlot( name );
		if ( !obj.IsNull() )
		{
			value = obj.Cast<Type>();
		}
	} // get_variable

	// run a script file; returns true on success, false on failure/exception
	bool execute_file( const char * filename );

	bool find_function( const SQChar* name, Sqrat::Function & function );
	HSQOBJECT find_member( HSQOBJECT class_obj, const SQChar* name );
	void check_result( SQRESULT result, const char * debug_string );
}; // namespace script
