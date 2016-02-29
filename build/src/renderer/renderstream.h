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

#include "renderer.h"

#include <core/typedefs.h>
#include <core/datastream.h>

namespace renderer
{
	const unsigned int MAX_RENDERER_STREAM_BYTES = 4096;
	const unsigned int MAX_RENDERER_STREAM_COMMANDS = 512;

	struct RenderState
	{
		int type;
		long offset; // offset into parameters
	}; // RenderState

	struct RenderStream
	{
		char buffer[ MAX_RENDERER_STREAM_BYTES ];
		RenderState commands[ MAX_RENDERER_STREAM_COMMANDS ];
		unsigned int num_commands;
		core::util::MemoryStream stream;

		RenderStream( unsigned int max_bytes = MAX_RENDERER_STREAM_BYTES, unsigned int max_commands = MAX_RENDERER_STREAM_COMMANDS );

		void save_offset( long & offset );
		void load_offset( long offset );

		void rewind();
		RenderState * new_render_state();
		void run_commands();
		void add_command( int type );
		void add_clearcolor( float r, float g, float b, float a );
		void add_clear( unsigned int bitflags );
		void add_cullmode( renderer::CullMode mode );
		void add_viewport( int x, int y, int width, int height );
		void add_uniform1i( int uniform_location, int value );
		void add_sampler2d( int uniform_location, int texture_unit, renderer::Texture* texture );
		void add_state( renderer::DriverState state, int enable );
		void add_blendfunc( renderer::RenderBlendType source, renderer::RenderBlendType destination );
		void add_shader( renderer::ShaderProgram * shader );
		void add_uniform3f( int location, const glm::vec3 * data );
		void add_uniform4f( int location, const glm::vec4 * data );
		void add_uniform_matrix4( int location, const glm::mat4 * data, uint8_t count = 1 );
		void add_draw_call( renderer::VertexBuffer * vertexbuffer );
		void add_material( renderer::Material * material, renderer::ShaderProgram * shader );
	}; // RenderStream
} // namespace renderer
