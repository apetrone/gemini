// -------------------------------------------------------------
// Copyright (C) 2015- Adam Petrone
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
#include "opengl_device.h"
#include "glcommandserializer.h"
#include "commandbuffer.h"
#include "r2_opengl_common.h"

#include <runtime/assets.h>

namespace render2
{
	// ---------------------------------------------------------------------
	// render target
	// ---------------------------------------------------------------------
	void OpenGLDevice::activate_render_target(const RenderTarget& /*rt*/)
	{

	}

	void OpenGLDevice::deactivate_render_target(const RenderTarget& /*rt*/)
	{

	}

	RenderTarget* OpenGLDevice::default_render_target()
	{
		return default_target;
	}

	RenderTarget* OpenGLDevice::create_render_target(Texture* texture)
	{
		return common_create_render_target(allocator, texture);
	}

	void OpenGLDevice::destroy_render_target(RenderTarget* target)
	{
		return common_destroy_render_target(allocator, target);
	}

	void OpenGLDevice::resize_render_target(RenderTarget* target, uint32_t width, uint32_t height)
	{
		common_resize_render_target(target, width, height);
	}

	void OpenGLDevice::render_target_read_pixels(RenderTarget* target, Image& image)
	{
		common_render_target_read_pixels(target, image);
	}

	// ---------------------------------------------------------------------
	// initialization
	// ---------------------------------------------------------------------
	void OpenGLDevice::init(int backbuffer_width, int backbuffer_height)
	{
		default_target->width = static_cast<uint32_t>(backbuffer_width);
		default_target->height = static_cast<uint32_t>(backbuffer_height);

#if defined(PLATFORM_WINDOWS)
		gl.SwapInterval(1);
#endif
	}


	// ---------------------------------------------------------------------
	// command serializer
	// ---------------------------------------------------------------------
	CommandSerializer* OpenGLDevice::create_serializer(CommandQueue* command_queue)
	{
		static GLCommandSerializer serializer(*command_queue);
		new (&serializer) GLCommandSerializer(*command_queue);
		return &serializer;
	}

	void OpenGLDevice::destroy_serializer(CommandSerializer* /*serializer*/)
	{
	}

	CommandQueue* OpenGLDevice::create_queue(const Pass& render_pass)
	{
		return common_create_queue(render_pass, &queue.next());
	}

	// ---------------------------------------------------------------------
	// command buffers / submission
	// ---------------------------------------------------------------------
	void OpenGLDevice::queue_buffers(CommandQueue* queue_list, size_t total_queues)
	{
		common_queue_buffers(queue_list, total_queues, queued_buffers);
	}

	void OpenGLDevice::backbuffer_resized(int backbuffer_width, int backbuffer_height)
	{
		common_resize_backbuffer(backbuffer_width, backbuffer_height, default_target);
	}


	// ---------------------------------------------------------------------
	// input layout
	// ---------------------------------------------------------------------
	InputLayout* OpenGLDevice::create_input_layout(const VertexDescriptor& descriptor, gemini::AssetHandle shader_handle)
	{
		GLInputLayout* gllayout = MEMORY2_NEW(allocator, GLInputLayout)(allocator);

		GLShader* shader = static_cast<GLShader*>(shader_from_handle(shader_handle));
		assert(shader);
		setup_input_layout(gllayout, descriptor, shader);

		return gllayout;
	}


	// ---------------------------------------------------------------------
	// pipeline
	// ---------------------------------------------------------------------
	void OpenGLDevice::activate_pipeline(GLPipeline* pipeline, GLBuffer* vertex_buffer)
	{
		GLShader* shader = static_cast<GLShader*>(shader_from_handle(pipeline->shader));
		assert(shader);
		gl.UseProgram(shader->id);
		gl.CheckError("activate_pipeline - UseProgram");

		// bind uniforms
		common_setup_uniforms(shader, pipeline->constants());

		// determine if the VAO for the vertex_buffer needs to be built
		if (!vertex_buffer->is_vao_valid())
		{
			vertex_buffer->create_vao();

			vertex_buffer->bind_vao();

			vertex_buffer->bind();

			GLInputLayout* layout = static_cast<GLInputLayout*>(pipeline->input_layout);
			for (size_t index = 0; index < layout->items.size(); ++index)
			{
				const GLInputLayout::Description& item = layout->items[index];

				assert(item.type != GL_INVALID_ENUM);

				assert(item.element_count >= 1 && item.element_count <= 4);

				gl.EnableVertexAttribArray(static_cast<GLuint>(index));
				gl.CheckError("EnableVertexAttribArray");

				gl.VertexAttribPointer(static_cast<GLuint>(index),
					item.element_count,
					static_cast<GLenum>(item.type),
					item.normalized,
					static_cast<GLsizei>(layout->vertex_stride),
					(void*)item.offset
				);
				gl.CheckError("VertexAttribPointer");
			}

			vertex_buffer->unbind_vao();

			vertex_buffer->unbind();
		}


		// see if we need to enable blending
		if (pipeline->enable_blending)
		{
			gl.Enable(GL_BLEND);
			gl.CheckError("Enable GL_BLEND");

			gl.BlendFunc(pipeline->blend_source, pipeline->blend_destination);
			gl.CheckError("BlendFunc");
		}
	} // activate_pipeline

	void OpenGLDevice::deactivate_pipeline(GLPipeline* pipeline)
	{
		gl.UseProgram(0);
		gl.CheckError("UseProgram(0)");

		if (pipeline->enable_blending)
		{
			gl.Disable(GL_BLEND);
			gl.CheckError("Disable GL_BLEND");
		}
	} // deactivate_pipeline


	// ---------------------------------------------------------------------
	// texture
	// ---------------------------------------------------------------------
	Texture* OpenGLDevice::create_texture(const Image& image)
	{
		return common_create_texture(allocator, image, parameters);
	}

	void OpenGLDevice::update_texture(Texture* tex, const Image& image, const glm::vec2& origin, const glm::vec2& dimensions)
	{
		GLTexture* texture = static_cast<GLTexture*>(tex);
		common_update_texture(texture, image, parameters, origin, dimensions);
	}

	void OpenGLDevice::destroy_texture(Texture* tex)
	{
		common_destroy_texture(allocator, tex);
	}

	// ---------------------------------------------------------------------
	//
	// ---------------------------------------------------------------------
	void OpenGLDevice::update_parameters(const RenderParameters& render_params)
	{
		if (render_params.has_key("gamma_correct"))
		{
			param_string value = render_params["gamma_correct"];
			if (value == "true")
			{
				parameters.flags |= RF_GAMMA_CORRECT;
			}
		}

		default_target->framebuffer_srgb(parameters.flags & RF_GAMMA_CORRECT);
	}

	size_t OpenGLDevice::compute_vertex_stride(const VertexDescriptor& descriptor)
	{
		size_t stride = 0;
		for (size_t index = 0; index < descriptor.size(); ++index)
		{
			const VertexDescriptor::InputDescription& input = descriptor[index];
			const VertexDataTypeToGL& gldata = get_vertexdata_table()[input.type];

			stride += (gldata.element_size * input.element_count);
		}

		return stride;
	}

	size_t OpenGLDevice::compute_index_stride()
	{
		// If you change this, you need to change calls to DrawElements.
		return sizeof(renderer::IndexType);
	}

	// ---------------------------------------------------------------------
	// shader
	// ---------------------------------------------------------------------
	Shader* OpenGLDevice::create_shader(ShaderSource** sources, uint32_t total_sources)
	{
		GLShader* shader = MEMORY2_NEW(allocator, GLShader)(allocator);
		if (0 == shader->build_from_sources(allocator, sources, total_sources))
		{
			return shader;
		}

		MEMORY2_DELETE(allocator, shader);
		return nullptr;
	} // create_shader
} // namespace render2
