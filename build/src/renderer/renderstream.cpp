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
#include "renderstream.h"
#include "renderer.h"

#include <core/typedefs.h>
#include <core/logging.h>

#include <string.h> // for memset
#include <stdio.h> // for printf

namespace renderer
{
	RenderStream::RenderStream(char* stream_buffer, size_t stream_buffer_size, RenderState* command_buffer, size_t max_render_commands)
	{
		buffer = stream_buffer;
		commands = command_buffer;
		max_commands = max_render_commands;
		num_commands = 0;
		stream.init(buffer, stream_buffer_size);
	}

	RenderStream::~RenderStream()
	{
	}

	void RenderStream::save_offset(long& offset)
	{
		offset = static_cast<long>(stream.current_offset());
	} // save_offset

	void RenderStream::load_offset(long offset)
	{
		stream.seek(static_cast<size_t>(offset), true);
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
		RenderState* renderstate;
		renderer::IRenderDriver* driver = renderer::driver();
		if (!driver)
		{
			return;
		}

		for(unsigned int state_id = 0; state_id < num_commands; state_id++)
		{
			renderstate = &commands[ state_id ];

			// setup the stream and run the command
			stream.seek(static_cast<size_t>(renderstate->offset), 1);
			driver->run_command((renderer::DriverCommandType)renderstate->type, stream);
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
		state->offset = static_cast<long>(stream.current_offset());
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
		stream.write( shader );
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
		renderer::driver()->setup_material(material, shader, *this);
	}

} // namespace renderer
