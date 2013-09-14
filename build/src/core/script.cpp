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
#include "typedefs.h"
#include "script.hpp"
#include "filesystem.hpp"
#include <stack>
#include <slim/xlog.h>

#define scvprintf vfprintf

#include "mathlib.h"

namespace script
{
	HSQUIRRELVM _sqvm = 0;


	namespace util
	{
		struct StackMonitor
		{
			HSQUIRRELVM _vm;
			bool _print_stack;
			
			void print_top( bool is_exiting )
			{
				const char * prefix;
				
				if ( is_exiting )
				{
					prefix = "[out]";
				}
				else
				{
					prefix = "[in]";
				}
				LOGV( "%s top=%i\n", prefix, (int)sq_gettop( _vm ) );
			} // print_top
			
			StackMonitor( HSQUIRRELVM vm, bool print_stack = false )
			{
				_vm = vm;
				print_top( false );
				_print_stack = print_stack;
				if (_print_stack)
				{
					script::print_stack( _vm );
				}
			} // StackMonitor
			
			~StackMonitor()
			{
				if ( _print_stack )
				{
					script::print_stack( _vm );
				}
				print_top( true );
			} // ~StackMonitor
		}; // StackMonitor
		
		static void print_callback(HSQUIRRELVM v,const SQChar *s,...)
		{
			va_list vl;
			va_start(vl, s);
			scvprintf(stdout, s, vl);
			va_end(vl);
		} // print_callback
		
		static void error_callback(HSQUIRRELVM v,const SQChar *s,...)
		{
			va_list vl;
			va_start(vl, s);
			scvprintf(stderr, s, vl);
			va_end(vl);
		} // error_callback
		
		
		
		

		
		// used for includes to determine the path of the currently loaded script
		typedef std::stack< std::string > StringStack;
		
		StringStack include_paths;
		
		void script_include( const char * path )
		{
			std::string front = include_paths.top();
			front.append(path);

			//execute_file(front.c_str());
		} // script_include
		
		
		void initialize_vm( HSQUIRRELVM vm )
		{
			sq_pushroottable( vm );
			sqstd_seterrorhandlers( vm ); // registers default error handlers
			sq_setprintfunc( vm, print_callback, error_callback );
			
			sqstd_register_iolib( vm );
			sqstd_register_systemlib( vm );
			sqstd_register_mathlib( vm );
			
			sq_pop( vm, 1 );
			
			// bind all functions and classes to make squirrel aware
			Sqrat::RootTable root( vm );
			
			//
			// FUNCTIONS
			root.Func( "include", script_include );
			
			//
			// CLASSES
			Sqrat::Class<glm::vec2> bind_vec2( vm );
			bind_vec2.Ctor<float, float>();
			bind_vec2.Var( "x", &glm::vec2::x );
			bind_vec2.Var( "y", &glm::vec2::y );
			Sqrat::RootTable( vm ).Bind( "vec2", bind_vec2 );
			
			
		} // initialize_vm
		
		void print_script_error( HSQUIRRELVM vm, const char * action )
		{
			sq_getlasterror( vm );
			const SQChar *buffer;
			sq_getstring(vm, -1, &buffer );
			
			LOGW( "ScriptError: (%s) %s\n", action, buffer );
			
			sq_pop( vm, 1 );
		} // print_script_error
		
		
	}; // util
	
	using namespace script::util;
	
	void startup( int64_t stack_size )
	{
		_sqvm = sq_open( stack_size );
		if ( _sqvm )
		{
			util::initialize_vm( _sqvm );
		}
		else
		{
			LOGE( "Unable to setup script environment.\n" );
		}
	} // startup
	
	
	void shutdown()
	{
		if ( _sqvm )
		{
			sq_close( _sqvm );
			_sqvm = 0;
		}
	} // shutdown
	
	const char * string_for_type( int sqtype )
	{
		switch( sqtype )
		{
			case OT_NULL : return "OT_NULL";
			case OT_INTEGER : return "OT_INTEGER";
			case OT_FLOAT : return "OT_FLOAT";
			case OT_BOOL : return "OT_BOOL";
			case OT_STRING : return "OT_STRING";
			case OT_TABLE : return "OT_TABLE";
			case OT_ARRAY : return "OT_ARRAY";
			case OT_USERDATA : return "OT_USERDATA";
			case OT_CLOSURE : return "OT_CLOSURE";
			case OT_NATIVECLOSURE : return "OT_NATIVECLOSURE";
			case OT_GENERATOR : return "OT_GENERATOR";
			case OT_USERPOINTER : return "OT_USERPOINTER";
			case OT_THREAD : return "OT_THREAD";
			case OT_FUNCPROTO : return "OT_FUNCPROTO";
			case OT_CLASS : return "OT_CLASS";
			case OT_INSTANCE : return "OT_INSTANCE";
			case OT_WEAKREF : return "OT_WEAKREF";
			case OT_OUTER : return "OT_OUTER";
			default: return "OT_UNKNOWN";
		}
	} // string_for_type
	
	void print_stack( HSQUIRRELVM vm )
	{
		int top = sq_gettop( vm );
		if ( top > 0 )
		{
			LOGV( "-- STACK, %i item(s)\n", top+1 );
			for( int i = 0, n = -1; i < top; ++i, --n )
			{
				SQObjectType t = sq_gettype(vm, n);
				LOGV( "[%i] [%i] Type: %s\n", (top)-i, n, script::string_for_type(t) );
			}
		}
		else
		{
			LOGV( "-- STACK, 0 items (empty)\n" );
		}
	} // print_stack
	
	HSQUIRRELVM get_vm()
	{
		return _sqvm;
	} // get_vm
	
	
	// NOTE: This only works with ASCII files at the moment.
	// Should be converted to translate UTF-8
	bool execute_file( const char * filename )
	{
		HSQUIRRELVM vm = get_vm();
		
		if ( vm == 0 )
		{
			LOGE( "script system not initialized yet!\n" );
			return false;
		}
		
		
		if ( fs::file_exists( filename ) == false )
		{
			LOGW( "File does not exist: %s\n", filename );
			return false;
		}
		
		// based on the path passed into this function
		// determine what the relative directory is and push that
		// on the include_paths stack.
		std::string path = filename;
		size_t pos = path.find_last_of("/");
		std::string temp = path.substr(0, pos+1);
		include_paths.push(temp);
		
		//fprintf( stdout, "execute: %s\n", filename );
		//SQInteger top = sq_gettop( vm );
		
		// fetch the script from our filesystem
		size_t buffer_length = 0;
		char * buffer = fs::file_to_buffer( filename, 0, &buffer_length );
		StackMonitor sm(vm);
		
		// compile the buffer and execute it in squirrel VM
		if ( SQ_SUCCEEDED( sq_compilebuffer( vm, buffer, buffer_length, "console", 1 )))
		{
			DEALLOC(buffer);
			sq_pushroottable( vm );
			if ( SQ_FAILED( sq_call( vm, 1, 0, 1 ) ) )
			{
				LOGE( "Execution failed for script \"%s\"\n", filename );
				print_script_error( vm, "execute file" );
				sq_pop( vm, 1 );
				return false;
			}
			else
			{
				//fprintf( stdout, "loaded file: %s\n", filename );
				sq_pop( vm, 1 );
				return true;
			}
		}
		else
		{
			DEALLOC(buffer);
			LOGE( "Failed to execute \"%s\"\n", filename );
			print_script_error( vm, "execute file" );
			sq_pop( vm, 1 );
			return false;
		}
		
		include_paths.pop();
		
		return false;
	} // execute_file
}; // namespace script