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
#include "renderer/renderer.h"
#include "factory.h"

#include "renderer/gemgl.h"
#include "opengl_common.h"

class GLCore32 : public renderer::IRenderDriver
{
	DECLARE_FACTORY_CLASS( GLCore32, renderer::IRenderDriver );


	GLObject last_shader;
	bool enable_gamma_correct;
	get_internal_image_format image_to_internal_format;
	
	// metrics
//	unsigned int shader_changes;
//	unsigned int texture_changes;

public:
	GLCore32();
	~GLCore32();
	
	virtual const char * description() { return "OpenGL Core 3.2"; }
	
	virtual void init_with_settings(const renderer::RenderSettings& settings);
	
	virtual void run_command( renderer::DriverCommandType command, MemoryStream & stream );
	virtual void post_command( renderer::DriverCommandType command, MemoryStream & stream );
	virtual void setup_drawcall( renderer::VertexBuffer * vertexbuffer, MemoryStream & stream );
			
	// texture
	virtual bool upload_texture_2d( renderer::TextureParameters & parameters );
	virtual bool generate_texture( renderer::TextureParameters & parameters );
	virtual bool destroy_texture( renderer::TextureParameters & parameters );
	virtual bool is_texture( renderer::TextureParameters & parameters );
	virtual bool texture_update( renderer::TextureParameters & parameters );
	virtual renderer::Texture* texture_create(image::Image& image, renderer::TextureParameters& parameters);
	virtual void texture_destroy(renderer::Texture* texture);
	
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
	virtual void shaderprogram_bind_uniform_block(renderer::ShaderProgram shader_program, renderer::ShaderParameters& parameters, const char* block_name);
	virtual bool shaderprogram_link_and_validate( renderer::ShaderProgram shader_program, renderer::ShaderParameters & parameters );
	virtual void shaderprogram_activate( renderer::ShaderProgram shader_program );
	virtual void shaderprogram_deactivate( renderer::ShaderProgram shader_program );
	
	virtual renderer::RenderTarget* render_target_create(uint16_t width, uint16_t height);
	virtual void render_target_destroy(renderer::RenderTarget* rt);
	virtual void render_target_activate(renderer::RenderTarget* rt);
	virtual void render_target_deactivate(renderer::RenderTarget* rt);
}; // GLCore32