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
#pragma once

#include <renderer/gl/gemgl.h>
#include <renderer/gl/opengl_common.h>

#include <runtime/asset_handle.h>

#include <core/logging.h>
#include <core/array.h>

namespace render2
{
	using namespace renderer;

	struct OpenGLDevice : public Device
	{
		struct GLBuffer : public Buffer
		{
			// vertex array object id (only used by vertex_buffers)
			GLuint vao;

			// array, element buffer ids
			GLuint vbo;

			GLenum type;

			GLBuffer(size_t size_bytes, GLenum buffer_type) :
			type(buffer_type)
			{
				vbo = vao = 0;

				max_size = static_cast<uint32_t>(size_bytes);

				gl.GenBuffers(1, &vbo);
				gl.CheckError("GLBuffer() - GenBuffers");

				bind();

				upload(0, max_size);

				unbind();
			}

			~GLBuffer()
			{
				gl.DeleteBuffers(1, &vbo);
				gl.CheckError("~GLBuffer() - DeleteBuffers");
			}

			bool is_vao_valid() const { return vao != 0; }

			void bind()
			{
				gl.BindBuffer(type, vbo);
				gl.CheckError("bind -> BindBuffer");
			}

			void unbind()
			{
				gl.BindBuffer(type, 0);
				gl.CheckError("unbind -> BindBuffer(0)");
			}

			// upload data to the GPU
			void upload(const void* data, size_t size_bytes)
			{
				assert(max_size >= size_bytes);
				gl.BufferData(type, static_cast<GLsizeiptr>(size_bytes), data, GL_STREAM_DRAW);
				gl.CheckError("upload -> BufferData");
			}

			void setup(const VertexDescriptor& /*descriptor*/)
			{
				//				core::util::MemoryStream ms;
				//				ms.init(pointer, max_size);
			}

			void create_vao()
			{
				gl.GenVertexArrays(1, &vao);
				gl.CheckError("GenVertexArrays");
			}

			void destroy_vao()
			{
				if (vao == 0)
				{
					return;
				}

				gl.DeleteVertexArrays(1, &vao);
				gl.CheckError("DeleteVertexArrays");
			}

			void bind_vao()
			{
				gl.BindVertexArray(vao);
				gl.CheckError("BindVertexArray");
			}

			void unbind_vao()
			{
				gl.BindVertexArray(0);
				gl.CheckError("BindVertexArray(0)");
			}

			void resize(size_t bytes)
			{
				max_size = static_cast<uint32_t>(bytes);
				bind();
				upload(nullptr, bytes);
				unbind();
			}
		};

	public:
		OpenGLDevice(gemini::Allocator& allocator)
			: allocator(allocator)
			, queue(allocator, CommandQueue(allocator))
			, queued_buffers(allocator)
		{
			reset();

			populate_vertexdata_table();
			load_gl_symbols();

			memset(&parameters, 0, sizeof(GLRenderParameters));

			default_target = MEMORY2_NEW(allocator, GLRenderTarget)(0, 0, true);
		}

		~OpenGLDevice()
		{
			MEMORY2_DELETE(allocator, default_target);
			unload_gl_symbols();
		}

	private:
		gemini::Allocator& allocator;
		GLRenderParameters parameters;

		void reset()
		{
			locked_buffer = 0;
		}

		void draw(
			GLPipeline* pipeline,
			GLBuffer* vertex_stream,
			size_t initial_offset,
			size_t total,
			size_t /*instance_index*/,
			size_t /*index_count*/)
		{
			activate_vertex_buffer(pipeline->input_layout, vertex_stream);

			gl.DrawArrays(pipeline->draw_type, static_cast<GLint>(initial_offset), static_cast<GLsizei>(total));
			gl.CheckError("DrawArrays");

			deactivate_vertex_buffer(vertex_stream);
		}

		void draw_indexed(
						  GLPipeline* pipeline,
						  GLBuffer* vertex_buffer,
						  GLBuffer* index_buffer,
						  size_t total)
		{
			activate_vertex_buffer(pipeline->input_layout, vertex_buffer);

			index_buffer->bind();
			gl.DrawElements(pipeline->draw_type, static_cast<GLsizei>(total), GL_UNSIGNED_SHORT, 0);
			gl.CheckError("DrawElements");
			index_buffer->unbind();

			deactivate_vertex_buffer(vertex_buffer);
		}

	public:

		void activate_pipeline(GLPipeline* pipeline);
		void deactivate_pipeline(GLPipeline* pipeline);

		// submit queue command buffers to GPU
		virtual void submit();

		// ---------------------------------------------------------------------
		// vertex / index buffers
		// ---------------------------------------------------------------------

		virtual Buffer* create_vertex_buffer(size_t size_bytes)
		{
			return MEMORY2_NEW(allocator, GLBuffer)(size_bytes, GL_ARRAY_BUFFER);
		}

		virtual Buffer* create_index_buffer(size_t size_bytes)
		{
			return MEMORY2_NEW(allocator, GLBuffer)(size_bytes, GL_ELEMENT_ARRAY_BUFFER);
		}

		virtual void destroy_buffer(Buffer* buffer)
		{
			MEMORY2_DELETE(allocator, buffer);
		}

		virtual void* buffer_lock(Buffer* buffer)
		{
			assert(locked_buffer == nullptr);
			GLBuffer* glb = static_cast<GLBuffer*>(buffer);
			glb->bind();
			void* pointer = gl.MapBuffer(glb->type, GL_WRITE_ONLY);
			gl.CheckError("MapBuffer");

			locked_buffer = glb;

			return pointer;
		}

		virtual void buffer_unlock(Buffer* buffer)
		{
			GLBuffer* glb = static_cast<GLBuffer*>(buffer);
			gl.UnmapBuffer(glb->type);
			gl.CheckError("UnmapBuffer");

			glb->unbind();

			locked_buffer = nullptr;
		}

		virtual void buffer_upload(Buffer* buffer, void* data, size_t data_size)
		{
			GLBuffer* glb = static_cast<GLBuffer*>(buffer);
			glb->bind();
			glb->upload(data, data_size);
			glb->unbind();
		}

		virtual void buffer_resize(Buffer* buffer, size_t data_size)
		{
			GLBuffer* glb = static_cast<GLBuffer*>(buffer);
			glb->resize(data_size);
		}

		// ---------------------------------------------------------------------
		// input layout
		// ---------------------------------------------------------------------
		virtual InputLayout* create_input_layout(const VertexDescriptor& descriptor, gemini::AssetHandle shader_handle);

		virtual void destroy_input_layout(InputLayout* layout)
		{
			MEMORY2_DELETE(allocator, layout);
		}

		// ---------------------------------------------------------------------
		// pipeline
		// ---------------------------------------------------------------------
		virtual Pipeline* create_pipeline(const PipelineDescriptor& descriptor)
		{
			GLPipeline* pipeline = MEMORY2_NEW(allocator, GLPipeline)(allocator, descriptor);
			setup_pipeline(pipeline, descriptor);
			return pipeline;
		}

		virtual void destroy_pipeline(Pipeline* pipeline)
		{
			GLPipeline* glp = static_cast<GLPipeline*>(pipeline);
			destroy_input_layout(glp->input_layout);

			MEMORY2_DELETE(allocator, glp);
		}

		// ---------------------------------------------------------------------
		// shader
		// ---------------------------------------------------------------------
		virtual Shader* create_shader(ShaderSource** sources, uint32_t total_sources);

		virtual void destroy_shader(Shader* shader)
		{
			MEMORY2_DELETE(allocator, shader);
		}

		// ---------------------------------------------------------------------
		// render target
		// ---------------------------------------------------------------------
		virtual void activate_render_target(const RenderTarget& rt);
		virtual void deactivate_render_target(const RenderTarget& rt);
		virtual RenderTarget* default_render_target();
		virtual RenderTarget* create_render_target(Texture* texture);
		virtual void destroy_render_target(RenderTarget* target);
		virtual void resize_render_target(RenderTarget* target, uint32_t width, uint32_t height);
		virtual void render_target_read_pixels(RenderTarget* target, Image& image);

		// ---------------------------------------------------------------------
		// initialization
		// ---------------------------------------------------------------------
		virtual void init(int backbuffer_width, int backbuffer_height);

		// ---------------------------------------------------------------------
		// command serializer
		// ---------------------------------------------------------------------
		virtual CommandSerializer* create_serializer(CommandQueue* command_queue);
		virtual void destroy_serializer(CommandSerializer* serializer);

		virtual CommandQueue* create_queue(const Pass& render_pass);

		// ---------------------------------------------------------------------
		// command buffers / submission
		// ---------------------------------------------------------------------
		virtual void queue_buffers(CommandQueue* queues, size_t total_queues);
		virtual void backbuffer_resized(int backbuffer_width, int backbuffer_height);

		// ---------------------------------------------------------------------
		// texture
		// ---------------------------------------------------------------------
		virtual Texture* create_texture(const Image& image);
		virtual void update_texture(Texture* texture, const Image& image, const glm::vec2& origin, const glm::vec2& dimensions);
		virtual void destroy_texture(Texture* texture);

		// ---------------------------------------------------------------------
		//
		// ---------------------------------------------------------------------
		virtual void update_parameters(const RenderParameters& render_params);

		virtual size_t compute_vertex_stride(const VertexDescriptor& descriptor);
		virtual size_t compute_index_stride();

	private:

		void activate_vertex_buffer(GLInputLayout* input_layout, GLBuffer* vertex_buffer);
		void deactivate_vertex_buffer(GLBuffer* vertex_buffer);


		GLRenderTarget* default_target;

		// rotating list of command queues
		CircularBuffer<CommandQueue, RENDERER_MAX_COMMAND_QUEUES> queue;

		// the list of buffers queued this frame
		Array<CommandQueue*> queued_buffers;

		GLBuffer* locked_buffer;
	}; // OpenGLDevice
} // namespace render2
