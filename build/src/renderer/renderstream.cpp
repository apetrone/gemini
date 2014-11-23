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
#include <gemini/typedefs.h>
#include "renderstream.h"
#include "renderer.h"

#include <string.h> // for memset
#include <stdio.h> // for printf
#include <slim/xlog.h>
	
RenderStream::RenderStream( unsigned int max_bytes, unsigned int max_commands )
{
	num_commands = 0;
	stream.init( buffer, MAX_RENDERER_STREAM_BYTES );
}

void RenderStream::save_offset( long & offset )
{
	offset = stream.current_offset();
} // save_offset

void RenderStream::load_offset( long offset )
{
	stream.seek( offset, true );
} // load_offset

void RenderStream::rewind()
{
	stream.rewind();
//	stream.clear();
	num_commands = 0;
}

RenderState * RenderStream::new_render_state()
{
	if ( num_commands >= (MAX_RENDERER_STREAM_COMMANDS) )
	{
		LOGW( "Too many renderstates! make this dynamically resizable!\n" );
		return 0;
	}
	
	RenderState * rs;
	rs = &commands[ num_commands++ ];
	return rs;
}

void RenderStream::run_commands()
{
	RenderState * renderstate;
	renderer::IRenderDriver * driver = renderer::driver();
	if ( !driver )
	{
		return;
	}

	for( int state_id = 0; state_id < num_commands; state_id++ )
	{
		renderstate = &commands[ state_id ];
		
		// setup the stream and run the command
		stream.seek( renderstate->offset, 1 );
		driver->run_command( (renderer::DriverCommandType)renderstate->type, stream );
	}
#if 0
	for( int state_id = 0; state_id < num_commands; state_id++ )
	{
		renderstate = &commands[ state_id ];
		
		// run the post (cleanup) command
		stream.seek( renderstate->offset, 1 );
		driver->post_command( (renderer::DriverCommandType)renderstate->type, stream );
	}
#endif
	
} // run_commands

void RenderStream::add_command( int type )
{
	RenderState * state = new_render_state();
	state->type = type;
	state->offset = stream.current_offset();
}

void RenderStream::add_clearcolor( float r, float g, float b, float a )
{
	add_command( renderer::DC_CLEARCOLOR );
	stream.write( r );
	stream.write( g );
	stream.write( b );
	stream.write( a );
}

void RenderStream::add_clear( unsigned int bitflags )
{
	add_command( renderer::DC_CLEAR );
	stream.write( bitflags );
}

void RenderStream::add_cullmode( renderer::CullMode mode )
{
	add_command( renderer::DC_CULLMODE );
	stream.write( mode );
}

void RenderStream::add_viewport( int x, int y, int width, int height )
{
	add_command( renderer::DC_VIEWPORT );
	stream.write( x );
	stream.write( y );
	stream.write( width );
	stream.write( height );
}

void RenderStream::add_uniform1i( int uniform_location, int value )
{
	add_command( renderer::DC_UNIFORM1i );
	stream.write( uniform_location );
	stream.write( value );
}

void RenderStream::add_sampler2d( int uniform_location, int texture_unit, renderer::Texture* texture )
{
	assert(texture != 0);
	add_command( renderer::DC_UNIFORM_SAMPLER_2D );
	stream.write( uniform_location );
	stream.write( texture_unit );
	stream.write( texture );
}

void RenderStream::add_state( renderer::DriverState state, int enable )
{
	add_command( renderer::DC_STATE );
	stream.write( state );
	stream.write( enable );
}

void RenderStream::add_blendfunc( renderer::RenderBlendType source, renderer::RenderBlendType destination )
{
	add_command( renderer::DC_BLENDFUNC );
	stream.write( source );
	stream.write( destination );
}

void RenderStream::add_shader( renderer::ShaderProgram * shader )
{
	add_command( renderer::DC_SHADER );
	stream.write( shader->object );
}

void RenderStream::add_uniform3f( int location, const glm::vec3 * data )
{
	add_command( renderer::DC_UNIFORM3f );
	stream.write( location );
	stream.write( data );
}

void RenderStream::add_uniform4f( int location, const glm::vec4 * data )
{
	add_command( renderer::DC_UNIFORM4f );
	stream.write( location );
	stream.write( data );
}

void RenderStream::add_uniform_matrix4( int location, const glm::mat4 * data, uint8_t count )
{
	add_command( renderer::DC_UNIFORMMATRIX4 );
	stream.write( data );
	stream.write( location );
	stream.write( count );
}

void RenderStream::add_draw_call( renderer::VertexBuffer * vertexbuffer )
{
	assert( vertexbuffer != 0 );
	add_command( renderer::DC_DRAWCALL );
	renderer::driver()->setup_drawcall( vertexbuffer, this->stream );
}

void RenderStream::add_material( renderer::Material * material, renderer::ShaderProgram * shader )
{
	assert( material != 0 );
	assert( shader != 0 );

	// setup shader parameters
	assert(0);
#if 0
	renderer::Material::Parameter * parameter;
	for( int p = 0; p < material->parameters.size(); ++p )
	{
		parameter = &material->parameters[ p ];
		int renderstate = assets::material_parameter_type_to_render_state( parameter->type );
		int uniform_location = shader->get_uniform_location( parameter->name.c_str() );
		
		// this needs to be converted to a table of function pointers...
		if ( renderstate == renderer::DC_UNIFORM1i )
		{
			add_uniform1i( uniform_location, parameter->intValue );
		}
		else if ( renderstate == renderer::DC_UNIFORM3f )
		{
			assert(renderstate == renderer::DC_UNIFORM3f);
			add_uniform3f( uniform_location, (glm::vec3*)&parameter->vecValue );
		}
		else if ( renderstate == renderer::DC_UNIFORM4f )
		{
			add_uniform4f( uniform_location, (glm::vec4*)&parameter->vecValue );
		}
		else if ( renderstate == renderer::DC_UNIFORM_SAMPLER_2D )
		{
			assets::Texture * texture = assets::textures()->find_with_id(parameter->intValue);
			if ( texture )
			{
				add_sampler2d( uniform_location, parameter->texture_unit, texture->texture );
			}
		}
		else if ( renderstate == renderer::DC_UNIFORM_SAMPLER_CUBE )
		{
			// ...
		}

	}
#endif
}