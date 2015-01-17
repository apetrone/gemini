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
#include "renderer.h"
#include "factory.h"

class GLESv2 : public renderer::IRenderDriver
{
	DECLARE_FACTORY_CLASS( GLESv2, renderer::IRenderDriver );
	
public:
	GLESv2();
	~GLESv2();
	
	// should use VAOs?
	bool has_oes_vertex_array_object;
	
	// should use VBOs?
	bool has_vbo_support;
	
	virtual const char * description() { return "OpenGL ES 2.0"; }
	
	virtual void run_command( renderer::DriverCommandType command, MemoryStream & stream );
	virtual void post_command( renderer::DriverCommandType command, MemoryStream & stream );
	virtual void setup_drawcall( renderer::VertexBuffer * vertexbuffer, MemoryStream & stream );
	
	// texture
	virtual bool upload_texture_2d( renderer::TextureParameters & parameters );
	virtual bool generate_texture( renderer::TextureParameters & parameters );
	virtual bool destroy_texture( renderer::TextureParameters & parameters );
	virtual bool is_texture( renderer::TextureParameters & parameters );
	virtual bool texture_update( renderer::TextureParameters & parameters );

	// vertexbuffer
	virtual renderer::VertexBuffer * vertexbuffer_create( renderer::VertexDescriptor & descriptor, renderer::VertexBufferDrawType draw_type, renderer::VertexBufferBufferType buffer_type, unsigned int vertex_size, unsigned int max_vertices, unsigned int max_indices );
	virtual void vertexbuffer_destroy( renderer::VertexBuffer * stream );
	virtual void vertexbuffer_upload_data( renderer::VertexBuffer * vertexbuffer, unsigned int vertex_stride, unsigned int vertex_count, renderer::VertexType * vertices, unsigned int index_count, renderer::IndexType * indices );
	
	virtual void vertexbuffer_draw_indices( renderer::VertexBuffer * vertexbuffer, unsigned int num_indices );
	virtual void vertexbuffer_draw( renderer::VertexBuffer * vertexbuffer, unsigned int num_vertices );
	
	virtual renderer::VertexBuffer * vertexbuffer_from_geometry( renderer::VertexDescriptor & descriptor, renderer::Geometry * geometry );
	virtual void vertexbuffer_upload_geometry( renderer::VertexBuffer * vertexbuffer, renderer::Geometry * geometry );
	
	// shaders
	
	virtual renderer::ShaderObject shaderobject_create( renderer::ShaderObjectType shader_type );
	virtual bool shaderobject_compile( renderer::ShaderObject shader_object, const char * shader_source, const char * preprocessor_defines, const char * version );
	virtual void shaderobject_destroy( renderer::ShaderObject shader_object );
	
	virtual renderer::ShaderProgram shaderprogram_create( renderer::ShaderParameters & parameters );
	virtual void shaderprogram_destroy( renderer::ShaderProgram program );
	virtual void shaderprogram_attach( renderer::ShaderProgram shader_program, renderer::ShaderObject shader_object );
	virtual void shaderprogram_detach( renderer::ShaderProgram shader_program, renderer::ShaderObject shader_object );
	virtual void shaderprogram_bind_attributes( renderer::ShaderProgram shader_program, renderer::ShaderParameters & parameters );
	virtual void shaderprogram_bind_uniforms( renderer::ShaderProgram shader_program, renderer::ShaderParameters & parameters );
	virtual bool shaderprogram_link_and_validate( renderer::ShaderProgram shader_program, renderer::ShaderParameters & parameters );
	virtual void shaderprogram_activate( renderer::ShaderProgram shader_program );
	virtual void shaderprogram_deactivate( renderer::ShaderProgram shader_program );
	
}; // GLESv2
