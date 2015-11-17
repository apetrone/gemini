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
#include "gemgl.h"
#include "opengl_common.h"

namespace renderer
{
	class GLCore32 : public renderer::IRenderDriver
	{

		GLObject last_shader;
		bool enable_gamma_correct;
		get_internal_image_format image_to_internal_format;

		// metrics
	//	unsigned int shader_changes;
	//	unsigned int texture_changes;

		renderer::RenderTarget* default_render_target;

	public:
		GLCore32();
		~GLCore32();

		virtual const char * description() { return "OpenGL Core 3.2"; }

		virtual void init_with_settings(const renderer::RenderSettings& settings);
		virtual void create_default_render_target();

		virtual void run_command( renderer::DriverCommandType command, core::util::MemoryStream & stream );
		virtual void post_command( renderer::DriverCommandType command, core::util::MemoryStream & stream );
		virtual void setup_drawcall( renderer::VertexBuffer * vertexbuffer, core::util::MemoryStream & stream );
		virtual void setup_material( renderer::Material* material, renderer::ShaderProgram* program, RenderStream& stream);

		// texture
		virtual renderer::Texture* texture_create(image::Image& image);
		virtual void texture_destroy(renderer::Texture* texture);
		virtual void texture_update(renderer::Texture* texture, const image::Image& image, const mathlib::Recti& rect);

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

		virtual renderer::ShaderProgram* shaderprogram_create();
		virtual void shaderprogram_destroy( renderer::ShaderProgram* program );
		virtual void shaderprogram_attach( renderer::ShaderProgram* shader_program, renderer::ShaderObject shader_object );
		virtual void shaderprogram_detach( renderer::ShaderProgram* shader_program, renderer::ShaderObject shader_object );
		virtual void shaderprogram_bind_attributes( renderer::ShaderProgram* shader_program );
		virtual void shaderprogram_bind_uniforms( renderer::ShaderProgram* shader_program );
		virtual void shaderprogram_bind_uniform_block(renderer::ShaderProgram* shader_program, const char* block_name);
		virtual bool shaderprogram_link_and_validate( renderer::ShaderProgram* shader_program );
		virtual void shaderprogram_activate( renderer::ShaderProgram* shader_program );
		virtual void shaderprogram_deactivate( renderer::ShaderProgram* shader_program );

		virtual renderer::RenderTarget* render_target_create(uint16_t width, uint16_t height);
		virtual void render_target_destroy(renderer::RenderTarget* rt);
		virtual void render_target_activate(renderer::RenderTarget* rt);
		virtual void render_target_deactivate(renderer::RenderTarget* rt);
		virtual void render_target_set_attachment(renderer::RenderTarget* rt, renderer::RenderTarget::AttachmentType type, uint8_t index, renderer::Texture* texture);

	}; // GLCore32
} // namespace renderer
