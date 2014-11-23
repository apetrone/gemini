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
#pragma once

#include "renderer.h"

#include <gemini/typedefs.h>
#include <gemini/util/datastream.h>

const unsigned int MAX_RENDERER_STREAM_BYTES = 8192 * 8;
const unsigned int MAX_RENDERER_STREAM_COMMANDS = 2048 * 8;

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
	util::MemoryStream stream;
	
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