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

#include <renderer/gl/gemgl.h>
#include "r2_opengl_common.h"

namespace render2
{
	using namespace renderer;

	struct GLES2Device : public Device
	{
		struct GLBuffer : public Buffer
		{
			// array, element buffer ids
			GLuint vbo;
			const void* buffer_data;
			GLenum type;

			GLBuffer(size_t size_bytes, GLenum buffer_type) :
			type(buffer_type)
			{
				vbo = 0;
				buffer_data = 0;
				max_size = size_bytes;

				// gl.GenBuffers(1, &vbo);
				// gl.CheckError("GLBuffer() - GenBuffers");

				bind();

				upload(0, max_size);

				unbind();
			}

			~GLBuffer()
			{
				// gl.DeleteBuffers(1, &vbo);
				// gl.CheckError("~GLBuffer() - DeleteBuffers");
			}

			void bind()
			{
				// gl.BindBuffer(type, vbo);
				// gl.CheckError("bind -> BindBuffer");
			}

			void unbind()
			{
				// gl.BindBuffer(type, 0);
				// gl.CheckError("unbind -> BindBuffer(0)");
			}

			// upload data to the GPU
			void upload(const void* data, size_t size_bytes)
			{
				assert(max_size >= size_bytes);
				// gl.BufferData(type, size_bytes, 0, GL_STREAM_DRAW);
				// gl.CheckError("upload -> BufferData");
				buffer_data = data;
			}

			void setup(const VertexDescriptor& descriptor)
			{
				//				core::util::MemoryStream ms;
				//				ms.init(pointer, max_size);
			}

			const char* get_data()
			{
				return reinterpret_cast<const char*>(buffer_data);
			}
		};


	public:
		GLES2Device()
		{
			reset();

			populate_vertexdata_table();
			load_gl_symbols();
		}

		~GLES2Device()
		{

		}

	private:
		void reset()
		{
			locked_buffer = 0;
		}

		void draw(
				  GLPipeline* pipeline,
				  GLBuffer* vertex_buffer,
				  size_t initial_offset,
				  size_t total,
				  size_t instance_index,
				  size_t index_count)
		{
			vertex_buffer->bind();
			activate_pipeline(pipeline, vertex_buffer);

			gl.DrawArrays(GL_TRIANGLES, initial_offset, total);
			gl.CheckError("DrawArrays");

			deactivate_pipeline(pipeline);

			vertex_buffer->unbind();
		}


		void draw_indexed(
						  GLPipeline* pipeline,
						  GLBuffer* vertex_buffer,
						  GLBuffer* index_buffer,
						  size_t total)
		{
			activate_pipeline(pipeline, vertex_buffer);

			index_buffer->bind();
			gl.DrawElements(GL_TRIANGLES, total, GL_UNSIGNED_SHORT, 0);
			index_buffer->unbind();

			deactivate_pipeline(pipeline);
		}


	public:

		void activate_pipeline(GLPipeline* pipeline, GLBuffer* vertex_buffer)
		{
			GLShader* shader = pipeline->program;
			gl.UseProgram(shader->id);
			gl.CheckError("activate_pipeline - UseProgram");

			// bind uniforms
			common_setup_uniforms(shader, (unsigned char*)pipeline->constants()->get_data());

			GLInputLayout* layout = static_cast<GLInputLayout*>(pipeline->input_layout);
			for (size_t index = 0; index < layout->items.size(); ++index)
			{
				GLInputLayout::Description& item = layout->items[index];

				assert(item.type != GL_INVALID_ENUM);

				assert(item.element_count >= 1 && item.element_count <= 4);

				gl.EnableVertexAttribArray(index);
				gl.CheckError("EnableVertexAttribArray");

				const char* data = vertex_buffer->get_data();

				gl.VertexAttribPointer(index, item.element_count, item.type, item.normalized, layout->vertex_stride, (void*)(data+item.offset));
				gl.CheckError("VertexAttribPointer");
			}

			// see if we need to enable blending
			if (pipeline->enable_blending)
			{
				gl.Enable(GL_BLEND);
				gl.CheckError("Enable");

				gl.BlendFunc(pipeline->blend_source, pipeline->blend_destination);
				gl.CheckError("BlendFunc");
			}
		}

		void deactivate_pipeline(GLPipeline* pipeline)
		{
			gl.UseProgram(0);
			gl.CheckError("UseProgram(0)");

			if (pipeline->enable_blending)
			{
				gl.Disable(GL_BLEND);
				gl.CheckError("Disable");
			}
		}

		// submit queue command buffers to GPU
		virtual void submit()
		{
			// If you hit this assert, there's a buffer locked for writing!
			assert(locked_buffer == nullptr);

			for (CommandQueue* cq : queued_buffers)
			{
				// setup pass
				const Pass* pass = &cq->pass;
				common_pass_setup(pass);

				GLPipeline* current_pipeline = nullptr;
				GLBuffer* vertex_stream = nullptr;
				GLBuffer* index_stream = nullptr;
				GLTexture* texture = nullptr;

				for (size_t index = 0; index < cq->commands.size(); ++index)
				{
					const Command* command = &cq->commands[index];
					if (command->type == COMMAND_DRAW)
					{
						draw(current_pipeline, vertex_stream, command->params[0], command->params[1], command->params[2], command->params[3]);
					}
					else if (command->type == COMMAND_DRAW_INDEXED)
					{
						index_stream = static_cast<GLBuffer*>(command->data[0]);
						draw_indexed(current_pipeline, vertex_stream, index_stream, command->params[0]);
					}
					else if (command->type == COMMAND_PIPELINE)
					{
						current_pipeline = static_cast<GLPipeline*>(command->data[0]);
						assert(current_pipeline != 0);
					}
					else if (command->type == COMMAND_SET_VERTEX_BUFFER)
					{
						vertex_stream = static_cast<GLBuffer*>(command->data[0]);
					}
//					else if (command->type == COMMAND_VIEWPORT)
//					{
//						gl.Viewport(command->params[0], command->params[1], command->params[2], command->params[3]);
//						gl.CheckError("glViewport");
//					}
					else if (command->type == COMMAND_TEXTURE)
					{
						if (texture)
						{
							texture->unbind();
						}

						texture = static_cast<GLTexture*>(command->data[0]);
						gl.ActiveTexture(GL_TEXTURE0+command->params[0]);
						gl.CheckError("ActiveTexture");
						texture->bind();

						assert(gl.IsTexture(texture->texture_id));
					}
					else
					{
						assert(0);
					}
				}

				cq->reset();
				if (texture)
				{
					texture->unbind();
				}
			}


			reset();
			queued_buffers.resize(0);
		}


		virtual Buffer* create_vertex_buffer(size_t size_bytes)
		{
			return MEMORY_NEW(GLBuffer, core::memory::global_allocator())(size_bytes, GL_ARRAY_BUFFER);
		}

		virtual Buffer* create_index_buffer(size_t size_bytes)
		{
			return MEMORY_NEW(GLBuffer, core::memory::global_allocator())(size_bytes, GL_ELEMENT_ARRAY_BUFFER);
		}

		virtual void destroy_buffer(Buffer* buffer)
		{
			MEMORY_DELETE(buffer, core::memory::global_allocator());
		}

		virtual void* buffer_lock(Buffer* buffer)
		{
			// GLBuffer* glb = static_cast<GLBuffer*>(buffer);
			// glb->bind();
			// void* pointer = gl.MapBuffer(glb->type, GL_WRITE_ONLY);
			// gl.CheckError("MapBuffer");

			// locked_buffer = glb;

			// return pointer;
			return nullptr;
		}

		virtual void buffer_unlock(Buffer* buffer)
		{
			// GLBuffer* glb = static_cast<GLBuffer*>(buffer);
			// gl.UnmapBuffer(glb->type);
			// gl.CheckError("UnmapBuffer");

			// glb->unbind();

			// locked_buffer = nullptr;
		}

		virtual void buffer_upload(Buffer* buffer, void* data, size_t data_size)
		{
			GLBuffer* glb = static_cast<GLBuffer*>(buffer);
			glb->bind();
			glb->upload(data, data_size);
			glb->unbind();
		}

		virtual InputLayout* create_input_layout(const VertexDescriptor& descriptor, Shader* shader)
		{
			GLInputLayout* gllayout = MEMORY_NEW(GLInputLayout, core::memory::global_allocator());
			GLShader* glshader = static_cast<GLShader*>(shader);

			setup_input_layout(gllayout, descriptor, glshader);

			return gllayout;
		}

		virtual void destroy_input_layout(InputLayout* layout)
		{
			MEMORY_DELETE(layout, core::memory::global_allocator());
		}

		virtual Pipeline* create_pipeline(const PipelineDescriptor& descriptor)
		{
			GLPipeline* pipeline = MEMORY_NEW(GLPipeline, core::memory::global_allocator())(descriptor);
			setup_pipeline(pipeline, descriptor);
			return pipeline;
		}

		virtual void destroy_pipeline(Pipeline* pipeline)
		{
			GLPipeline* glp = static_cast<GLPipeline*>(pipeline);
			destroy_shader(glp->program);
			destroy_input_layout(glp->input_layout);

			MEMORY_DELETE(glp, core::memory::global_allocator());
		}

		virtual Shader* create_shader(const char* name, Shader* reuse_shader);

		virtual void destroy_shader(Shader* shader)
		{
			MEMORY_DELETE(shader, core::memory::global_allocator());
		}

		// ---------------------------------------------------------------------
		// render target
		// ---------------------------------------------------------------------
		virtual void activate_render_target(const RenderTarget& rt);
		virtual void deactivate_render_target(const RenderTarget& rt);
		virtual RenderTarget* default_render_target();

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
		virtual void update_texture(Texture* texture, const Image& image);
		virtual void destroy_texture(Texture* texture);

	private:
		render2::RenderTarget default_target;

		// rotating list of command queues
		CircularBuffer<CommandQueue, RENDERER_MAX_COMMAND_QUEUES> queue;

		// the list of buffers queued this frame
		Array<CommandQueue*> queued_buffers;

		GLBuffer* locked_buffer;
	}; // GLES2Device
} // namespace render2
