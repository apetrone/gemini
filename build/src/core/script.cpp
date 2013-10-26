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

#include "util.hpp"
#include "debugdraw.hpp"

void Color::set_color( unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a )
{
	r = _r;
	g = _g;
	b = _b;
	a = _a;
	LOGV( "color: %i %i %i %i\n", r, g, b, a );
}

namespace script
{
	HSQUIRRELVM _sqvm = 0;


	// internal bind functions for operators, etc.
	namespace bind
	{
		float random_float()
		{
			return ::util::random_range(0.0f, 1.0f);
		} // random_float
	
		SQInteger vec2_add( HSQUIRRELVM v )
		{
			if ( sq_gettop(v) == 2 )
			{
				Sqrat::Var<const glm::vec2&> self(v, 1);
				Sqrat::Var<const glm::vec2&> other(v, 2);
				if ( !Sqrat::Error::Instance().Occurred(v) )
				{
					glm::vec2 result = self.value + other.value;
					Sqrat::PushVar( v, result );
					return 1;
				}
				Sqrat::Error::Instance().Clear(v);
				
				
				LOGE( "ERROR vec2_add\n" );
				
				return 1;
			}
			else
			{
				return 0;
			}
		} // vec2_add
	
		SQInteger vec2_multiply( HSQUIRRELVM v )
		{
			if ( sq_gettop(v) == 2 )
			{
				Sqrat::Var<const glm::vec2&> self(v, 1);
				Sqrat::Var<const glm::vec2&> other(v, 2);
				if ( !Sqrat::Error::Instance().Occurred(v) )
				{
					glm::vec2 result = self.value * other.value;
					Sqrat::PushVar( v, result );
					return 1;
				}
				Sqrat::Error::Instance().Clear(v);
				
				
				Sqrat::Var<float> fother(v,2);
				if ( !Sqrat::Error::Instance().Occurred(v) )
				{
					glm::vec2 r = self.value * fother.value;
					Sqrat::PushVar( v, r );
					return 1;
				}
				Sqrat::Error::Instance().Clear(v);
				
				LOGE( "ERROR vec2_multiply\n" );
				
				return 1;
			}
			else
			{
				return 0;
			}
		} // vec2_multiply
	
		SQInteger vec3_add( HSQUIRRELVM v )
		{
			if ( sq_gettop(v) == 2 )
			{
				Sqrat::Var<const glm::vec3&> self(v, 1);
				Sqrat::Var<const glm::vec3&> other(v, 2);
				if ( !Sqrat::Error::Instance().Occurred(v) )
				{
					glm::vec3 result = self.value + other.value;
					Sqrat::PushVar( v, result );
					return 1;
				}
				Sqrat::Error::Instance().Clear(v);
				
				
				LOGE( "ERROR vec3_add\n" );
				
				return 1;
			}
			else
			{
				return 0;
			}
		} // vec3_add
		
		SQInteger vec3_subtract( HSQUIRRELVM v )
		{
			if ( sq_gettop(v) == 2 )
			{
				Sqrat::Var<const glm::vec3&> self(v, 1);
				Sqrat::Var<const glm::vec3&> other(v, 2);
				if ( !Sqrat::Error::Instance().Occurred(v) )
				{
					glm::vec3 result = self.value - other.value;
					Sqrat::PushVar( v, result );
					return 1;
				}
				Sqrat::Error::Instance().Clear(v);
				
				
				LOGE( "ERROR vec3_subtract\n" );
				
				return 1;
			}
			else
			{
				return 0;
			}
		} // vec3_subtract
		
		SQInteger vec3_multiply( HSQUIRRELVM v )
		{
			if ( sq_gettop(v) == 2 )
			{
				Sqrat::Var<const glm::vec3&> self(v, 1);
				Sqrat::Var<const glm::vec3&> other(v, 2);
				if ( !Sqrat::Error::Instance().Occurred(v) )
				{
					glm::vec3 result = self.value * other.value;
					Sqrat::PushVar( v, result );
					return 1;
				}
				Sqrat::Error::Instance().Clear(v);
				
				
				Sqrat::Var<float> fother(v,2);
				if ( !Sqrat::Error::Instance().Occurred(v) )
				{
					glm::vec3 r = self.value * fother.value;
					Sqrat::PushVar( v, r );
					return 1;
				}
				Sqrat::Error::Instance().Clear(v);
				
				LOGE( "ERROR vec3_multiply\n" );
				
				return 1;
			}
			else
			{
				return 0;
			}
		} // vec3_multiply
		
		SQInteger vec3_divide( HSQUIRRELVM v )
		{
			if ( sq_gettop(v) == 2 )
			{
				Sqrat::Var<const glm::vec3&> self(v, 1);
				Sqrat::Var<const glm::vec3&> other(v, 2);
				if ( !Sqrat::Error::Instance().Occurred(v) )
				{
					glm::vec3 result = self.value / other.value;
					Sqrat::PushVar( v, result );
					return 1;
				}
				Sqrat::Error::Instance().Clear(v);
				
				
				Sqrat::Var<float> fother(v,2);
				if ( !Sqrat::Error::Instance().Occurred(v) )
				{
					glm::vec3 r = self.value / fother.value;
					Sqrat::PushVar( v, r );
					return 1;
				}
				Sqrat::Error::Instance().Clear(v);
				
				fprintf( stdout, "ERROR vec3_divide\n" );
				
				return 1;
			}
			else
			{
				return 0;
			}
		} // vec3_divide
		
		
		SQInteger math_normalize( HSQUIRRELVM v )
		{
			if ( sq_gettop(v) == 2 )
			{
				// vec3
				Sqrat::Var<const glm::vec3&> self(v, 2);
				if ( !Sqrat::Error::Instance().Occurred(v) )
				{
					glm::vec3 result = glm::normalize(self.value);
					Sqrat::PushVar(v, result);
					return 1;
				}
				Sqrat::Error::Instance().Clear(v);
				
				// vec4
				Sqrat::Var<const glm::vec4&> selfv4(v, 2);
				if ( !Sqrat::Error::Instance().Occurred(v) )
				{
					glm::vec4 result = glm::normalize(selfv4.value);
					Sqrat::PushVar(v, result);
					return 1;
				}
				Sqrat::Error::Instance().Clear(v);
				
				
				fprintf( stdout, "ERROR Unexpected item for normalize!\n" );
				return 1;
			}
			else
			{
				return 0;
			}
		} // math_normalize
		
		SQInteger math_inverse( HSQUIRRELVM v )
		{
			if ( sq_gettop(v) == 2 )
			{
				// mat3
				Sqrat::Var<const glm::mat3&> self(v, 2);
				if ( !Sqrat::Error::Instance().Occurred(v) )
				{
					glm::mat3 result = glm::inverse(self.value);
					Sqrat::PushVar(v, result);
					return 1;
				}
				Sqrat::Error::Instance().Clear(v);
				
				// mat4
				Sqrat::Var<const glm::mat4&> selfv4(v, 2);
				if ( !Sqrat::Error::Instance().Occurred(v) )
				{
					glm::mat4 result = glm::inverse(selfv4.value);
					Sqrat::PushVar(v, result);
					return 1;
				}
				Sqrat::Error::Instance().Clear(v);
				
				
				fprintf( stdout, "ERROR Unexpected item for inverse!\n" );
				return 1;
			}
			else
			{
				return 0;
			}
		} // math_inverse
		
		SQInteger math_transpose( HSQUIRRELVM v )
		{
			if ( sq_gettop(v) == 2 )
			{
				// mat3
				Sqrat::Var<const glm::mat3&> self(v, 2);
				if ( !Sqrat::Error::Instance().Occurred(v) )
				{
					glm::mat3 result = glm::transpose(self.value);
					Sqrat::PushVar(v, result);
					return 1;
				}
				Sqrat::Error::Instance().Clear(v);
				
				// mat4
				Sqrat::Var<const glm::mat4&> selfv4(v, 2);
				if ( !Sqrat::Error::Instance().Occurred(v) )
				{
					glm::mat4 result = glm::transpose(selfv4.value);
					Sqrat::PushVar(v, result);
					return 1;
				}
				Sqrat::Error::Instance().Clear(v);
				
				
				fprintf( stdout, "ERROR Unexpected item for transpose!\n" );
				return 1;
			}
			else
			{
				return 0;
			}
		} // math_transpose
		
		SQInteger math_dot( HSQUIRRELVM v )
		{
			if ( sq_gettop(v) == 3 )
			{
				// vec3
				Sqrat::Var<const glm::vec3&> self(v, 2);
				Sqrat::Var<const glm::vec3&> other(v, 3);
				if ( !Sqrat::Error::Instance().Occurred(v) )
				{
					float result = glm::dot(self.value, other.value);
					Sqrat::PushVar(v, result);
					return 1;
				}
				Sqrat::Error::Instance().Clear(v);
				
				fprintf( stdout, "ERROR Unexpected item for glm::dot!\n" );
				return 1;
			}
			else
			{
				return 0;
			}
		} // math_dot
		
		SQInteger math_cross( HSQUIRRELVM v )
		{
			if ( sq_gettop(v) == 3 )
			{
				// vec3
				Sqrat::Var<const glm::vec3&> self(v, 2);
				Sqrat::Var<const glm::vec3&> other(v, 3);
				if ( !Sqrat::Error::Instance().Occurred(v) )
				{
					glm::vec3 result = glm::cross(self.value, other.value);
					Sqrat::PushVar(v, result);
					return 1;
				}
				Sqrat::Error::Instance().Clear(v);
				
				fprintf( stdout, "ERROR Unexpected item for glm::cross!\n" );
				return 1;
			}
			else
			{
				return 0;
			}
		} // math_cross
		
		
		
		
		
		
		
		
		
		SQInteger mat3_multiply( HSQUIRRELVM v )
		{
			if ( sq_gettop(v) == 2 )
			{
				// matrix * matrix
				Sqrat::Var<const glm::mat3&> self(v, 1);
				Sqrat::Var<const glm::mat3&> other(v, 2);
				if ( !Sqrat::Error::Instance().Occurred(v) )
				{
					glm::mat3 result = self.value * other.value;
					Sqrat::PushVar( v, result );
					return 1;
				}
				Sqrat::Error::Instance().Clear(v);
				
				
				// matrix * vec3
				Sqrat::Var<const glm::vec3&> v_other(v, 2);
				if ( !Sqrat::Error::Instance().Occurred(v) )
				{
					glm::vec3 result = self.value * v_other.value;
					Sqrat::PushVar( v, result );
					return 1;
				}
				Sqrat::Error::Instance().Clear(v);
				
				
				LOGE( "mat3_multiply Invalid operands!\n" );
				
				return 1;
			}
			else
			{
				return 0;
			}
		} // mat3_multiply
		
		SQInteger mat4_multiply( HSQUIRRELVM v )
		{
			if ( sq_gettop(v) == 2 )
			{
				// matrix * matrix
				Sqrat::Var<const glm::mat4&> self(v, 1);
				Sqrat::Var<const glm::mat4&> other(v, 2);
				if ( !Sqrat::Error::Instance().Occurred(v) )
				{
					glm::mat4 result = self.value * other.value;
					Sqrat::PushVar(v, result);
					return 1;
				}
				Sqrat::Error::Instance().Clear(v);
				
				
				// matrix * vec4
				Sqrat::Var<const glm::vec4&> v_other(v, 2);
				if ( !Sqrat::Error::Instance().Occurred(v) )
				{
					glm::vec4 result = self.value * v_other.value;
					Sqrat::PushVar(v, result);
					return 1;
				}
				Sqrat::Error::Instance().Clear(v);

				LOGE( "Error mat4_multiply. Invalid operands!\n" );
				
				return 1;
			}
			else
			{
				return 0;
			}
		} // mat4_multiply


		SQInteger rotate( HSQUIRRELVM v )
		{
			if ( sq_gettop(v) == 4 )
			{
				Sqrat::Var<const glm::mat4&> mat4_param(v, 2);
				if ( !Sqrat::Error::Instance().Occurred(v) )
				{
					// operation on mat4

					Sqrat::Var<float> angle(v,3);
					Sqrat::Var<const glm::vec3&> axis(v,4);


					glm::mat4 result = glm::rotate( mat4_param.value, angle.value, axis.value );
					Sqrat::PushVar(v, result);
					return 1;
				}
				/*
				Sqrat::Var<const glm::mat4&> other(v, 2);
				if ( !Sqrat::Error::Instance().Occurred(v) )
				{
					glm::mat4 result = self.value * other.value;
					Sqrat::PushVar(v, result);
					return 1;
				}
				Sqrat::Error::Instance().Clear(v);
				
				
				// matrix * vec4
				Sqrat::Var<const glm::vec4&> v_other(v, 2);
				if ( !Sqrat::Error::Instance().Occurred(v) )
				{
					glm::vec4 result = self.value * v_other.value;
					Sqrat::PushVar(v, result);
					return 1;
				}
				Sqrat::Error::Instance().Clear(v);*/
				
				LOGE( "Error rotate. Invalid operands!\n" );
				
				return 1;
			}
			else
			{
				return 0;
			}
		} // rotate
		
		
		SQInteger translate( HSQUIRRELVM v )
		{
			if ( sq_gettop(v) == 3 )
			{
				Sqrat::Var<const glm::mat4&> mat4_param(v, 2);
				if ( !Sqrat::Error::Instance().Occurred(v) )
				{
					// operation on mat4
					Sqrat::Var<const glm::vec3&> vector(v,3);
					
					
					glm::mat4 result = glm::translate( mat4_param.value, vector.value );
					Sqrat::PushVar(v, result);
					return 1;
				}
				/*
				 Sqrat::Var<const glm::mat4&> other(v, 2);
				 if ( !Sqrat::Error::Instance().Occurred(v) )
				 {
				 glm::mat4 result = self.value * other.value;
				 Sqrat::PushVar(v, result);
				 return 1;
				 }
				 Sqrat::Error::Instance().Clear(v);
				 
				 
				 // matrix * vec4
				 Sqrat::Var<const glm::vec4&> v_other(v, 2);
				 if ( !Sqrat::Error::Instance().Occurred(v) )
				 {
				 glm::vec4 result = self.value * v_other.value;
				 Sqrat::PushVar(v, result);
				 return 1;
				 }
				 Sqrat::Error::Instance().Clear(v);*/
				
				LOGE( "Error translate. Invalid operands!\n" );
				
				return 1;
			}
			else
			{
				return 0;
			}
		} // translate
	}; // bind



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
//			va_list vl;
//			va_start(vl, s);
//			scvprintf(stdout, s, vl);
//			va_end(vl);
			LOGV( s );
		} // print_callback
		
		static void error_callback(HSQUIRRELVM v,const SQChar *s,...)
		{
//			va_list vl;
//			va_start(vl, s);
//			scvprintf(stderr, s, vl);
//			va_end(vl);
			LOGE( s );
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
			root.Func( "max", fmax );
			root.Func( "min", fmin );

			Sqrat::Table random( vm );
			random.Func( "random", bind::random_float );
			random.Func( "range", ::util::random_range );
			root.Bind( "random", random );

			//
			// CLASSES
			Sqrat::Class<glm::vec2> bind_vec2( vm );
			bind_vec2.Ctor<float, float>();
			bind_vec2.Var( "x", &glm::vec2::x );
			bind_vec2.Var( "y", &glm::vec2::y );
			bind_vec2.SquirrelFunc( "_add", bind::vec2_add );
			bind_vec2.SquirrelFunc( "_mul", bind::vec2_multiply );
			Sqrat::RootTable( vm ).Bind( "vec2", bind_vec2 );
			
			Sqrat::Class<glm::vec3> bind_vec3( vm );
			bind_vec3.Ctor<float, float, float>();
			bind_vec3.Ctor<glm::vec4>();
			bind_vec3.SquirrelFunc( "_add", bind::vec3_add );
			bind_vec3.SquirrelFunc( "_sub", bind::vec3_subtract );
			bind_vec3.SquirrelFunc( "_mul", bind::vec3_multiply );
			bind_vec3.SquirrelFunc( "_div", bind::vec3_divide );
			bind_vec3.Var( "x", &glm::vec3::x );
			bind_vec3.Var( "y", &glm::vec3::y );
			bind_vec3.Var( "z", &glm::vec3::z );
			Sqrat::RootTable( vm ).Bind( "vec3", bind_vec3 );
			
			root.SquirrelFunc( "normalize", bind::math_normalize );
			root.SquirrelFunc( "inverse", bind::math_inverse );
			root.SquirrelFunc( "transpose", bind::math_transpose );
			root.SquirrelFunc( "dot", bind::math_dot );
			root.SquirrelFunc( "cross", bind::math_cross );
			root.SquirrelFunc( "rotate", bind::rotate );
			root.SquirrelFunc( "translate", bind::translate );
			
			Sqrat::Class<glm::vec4> bind_vec4( vm );
			bind_vec4.Ctor<float, float, float, float>();
			bind_vec4.Ctor<glm::vec3, float>();
			bind_vec4.Var( "x", &glm::vec4::x );
			bind_vec4.Var( "y", &glm::vec4::y );
			bind_vec4.Var( "z", &glm::vec4::z );
			bind_vec4.Var( "w", &glm::vec4::w );
			Sqrat::RootTable( vm ).Bind( "vec4", bind_vec4 );
			
			Sqrat::Class<glm::mat3> bind_mat3( vm );
			bind_mat3.Ctor<glm::vec3, glm::vec3, glm::vec3>();
			bind_mat3.Ctor<glm::mat4>();
			bind_mat3.SquirrelFunc( "_mul", bind::mat3_multiply );
			root.Bind( "mat3", bind_mat3 );
			
			
			Sqrat::Class<glm::mat4> bind_mat4( vm );
			bind_mat4.SquirrelFunc( "_mul", bind::mat4_multiply );
			root.Bind( "mat4", bind_mat4 );
			
			
			Sqrat::Class<Color> color( vm );
			color.Ctor<unsigned char, unsigned char, unsigned char, unsigned char>();
			color.Var( "r", &Color::r );
			color.Var( "g", &Color::g );
			color.Var( "b", &Color::b );
			color.Var( "a", &Color::a );
			color.Func( "set", &Color::set_color );
			root.Bind( "Color", color );
			
			Sqrat::Table debug( vm );
			debug.Func( "axes", debugdraw::axes );
			debug.Func( "point", debugdraw::point );
			debug.Func( "line", debugdraw::line );
			debug.Func( "sphere", debugdraw::sphere );
			
			root.Bind( "debug", debug );
			
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
		
//		StackMonitor sm(vm);
		
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
	
	bool find_function( const char * name, Sqrat::Function & function )
	{
		function = Sqrat::RootTable( get_vm() ).GetFunction( name );
		if ( function.IsNull() )
		{
			LOGV( "Unable to find function: \"%s\"\n", name );
			return false;
		}
		
		return true;
	} // find_function
	
	HSQOBJECT find_member( HSQOBJECT class_obj, const char * name )
	{
//		StackMonitor sm( get_vm() );
		
		bool success = false;
		
		HSQOBJECT obj;
		sq_resetobject( &obj );
		
		Sqrat::Object classobj( class_obj, get_vm() );
		if ( !classobj.IsNull() )
		{
			Sqrat::Function function = classobj.GetSlot( name ).Cast<Sqrat::Function>();
			if ( !function.IsNull() )
			{
				obj = function.GetFunc();
				success = true;
			}
		}
		
		if ( !success )
		{
			LOGW( "Unable to find '%s'\n", name );
		}
		
		return obj;
	} // find_member
	
	void check_result( SQRESULT result, const char * debug_string )
	{
		if ( SQ_FAILED(result) )
		{
			sq_getlasterror( get_vm() );
			const SQChar * sqc;
			sq_getstring( get_vm(), -1, &sqc );
			LOGE( "script_error on %s\n", debug_string );
			LOGE( "script_error: %s\n", sqc );
		}
	} // check_result
	
}; // namespace script