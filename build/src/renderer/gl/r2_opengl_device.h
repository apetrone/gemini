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



namespace render2
{
	using namespace renderer;
	static size_t type_to_bytes(const GLenum& type)
	{
		switch(type)
		{
			case GL_FLOAT_VEC2: return sizeof(GLfloat) * 2;
			case GL_FLOAT_VEC3: return sizeof(GLfloat) * 3;
			case GL_FLOAT_VEC4: return sizeof(GLfloat) * 4;
				
			case GL_FLOAT_MAT4: return sizeof(GLfloat) * 16;
				
				
			default: break;
		}
		
		
		assert(0);
		return 0;
	}
	
	const size_t MAX_ATTRIBUTE_NAME_LENGTH = 32;
	struct shader_variable
	{
		// location of this variable
		GLint location;
		
		// byte-length of name
		GLsizei length;
		
		// byte-length of the attribute value
		GLint size;
		
		// attribute name (null-terminated string)
		GLchar name[ MAX_ATTRIBUTE_NAME_LENGTH ];
		
		// data type of the attribute
		GLenum type;
		
		// size (in bytes) of this type
		GLint byte_size;
		
		void compute_size()
		{
			// compute byte size for this attribute
			byte_size = (size * type_to_bytes(type));
		}
	};
	
	struct OpenGLDevice : public Device
	{
		struct GLShader : public Shader
		{
			GLint id;
			FixedArray<shader_variable> uniforms;
			FixedArray<shader_variable> attributes;
			
			GLShader()
			{
				id = gl.CreateProgram();
				GLint vert = gl.CreateShader(GL_VERTEX_SHADER);
				GLint frag = gl.CreateShader(GL_FRAGMENT_SHADER);
				
				const char* vertex_shader_source = "\
				precision highp float;\
				uniform mat4 modelview_matrix;\
				uniform mat4 projection_matrix;\
				\
				in vec4 in_position;\
				in vec4 in_color;\
				out vec4 vertex_color;\
				\
				void main()\
				{\
				gl_Position = (projection_matrix * modelview_matrix * in_position);\
				vertex_color = in_color;\
				}";
				
				const char* fragment_shader_source = "\
				precision highp float;\
				in vec4 vertex_color;\
				out vec4 out_color;\
				\
				void main()\
				{\
				out_color = vertex_color;\
				}";
				
				bool result = false;
				result = compile_shader(vert, vertex_shader_source, "", "#version 150 core\n");
				result = compile_shader(frag, fragment_shader_source, "", "#version 150 core\n");
				
				
				// attach shaders
				gl.AttachShader(id, vert); gl.CheckError("AttachShader (vert)");
				gl.AttachShader(id, frag); gl.CheckError("AttachShader (frag)");
				
				
				// bind attributes
				gl.BindFragDataLocation(id, 0, "out_color");
				gl.CheckError("BindFragDataLocation");
				
				// link and activate shader
				gl.LinkProgram(id);
				gl.CheckError("LinkProgram");
				GLint is_linked = 0;
				gl.GetProgramiv(id, GL_LINK_STATUS, &is_linked);
				gl.CheckError("link and activate shader GetProgramiv");
				
				if (!is_linked)
				{
					dump_program_log();
				}
				
				assert(is_linked == 1);
				
				
				// activate program
				gl.UseProgram(id);
				gl.CheckError("activate program UseProgram");
				
				
				
				
				
				
				
				{
					GLint active_attributes = 0;
					gl.GetProgramiv(id, GL_ACTIVE_ATTRIBUTES, &active_attributes);
					gl.CheckError("inspect attributes GetProgramiv");
					
					attributes.allocate(active_attributes);
					
					for (GLint attribute_index = 0; attribute_index < active_attributes; ++attribute_index)
					{
						shader_variable& attribute = attributes[attribute_index];
						gl.GetActiveAttrib(id, attribute_index, MAX_ATTRIBUTE_NAME_LENGTH, &attribute.length, &attribute.size, &attribute.type, attribute.name);
						attribute.location = gl.GetAttribLocation(id, attribute.name);
//						LOGV("attribute: %i, location: %i, name: %s, size: %i, type: %i\n",
//							 attribute_index,
//							 attribute.location,
//							 attribute.name,
//							 attribute.size,
//							 attribute.type);
						
						attribute.compute_size();
					}
				}
				
				// cache uniform locations
				{
					GLint active_uniforms = 0;
					gl.GetProgramiv(id, GL_ACTIVE_UNIFORMS, &active_uniforms);
					
					// allocate data for uniforms
					uniforms.allocate(active_uniforms);
					
					for (GLint uniform_index = 0; uniform_index < active_uniforms; ++uniform_index)
					{
						shader_variable& uniform = uniforms[uniform_index];
						gl.GetActiveUniform(id, uniform_index, MAX_ATTRIBUTE_NAME_LENGTH, &uniform.length, &uniform.size, &uniform.type, uniform.name);
						uniform.location = gl.GetUniformLocation(id, uniform.name);
//						LOGV("attribute: %i, location: %i, name: %s, size: %i, type: %i\n",
//							 uniform_index,
//							 uniform.location,
//							 uniform.name,
//							 uniform.size,
//							 uniform.type);
						
						uniform.compute_size();
					}
				}
				
				
				// deactivate
				gl.UseProgram(0);
				
				
				// detach shaders
				gl.DetachShader(id, vert);
				gl.DetachShader(id, frag);
				
				
				// delete shaders
				gl.DeleteShader(frag);
				gl.DeleteShader(vert);
			}
			
			~GLShader()
			{
				uniforms.clear();
				attributes.clear();
				gl.DeleteProgram(id);
			}
			
			
			bool compile_shader(GLint shader, const char* source, const char* preprocessor_defines, const char* version)
			{
				GLint is_compiled = 0;
				const char* shader_source[3] = {
					version,
					preprocessor_defines,
					source
				};
				
				gl.ShaderSource(shader, 3, (GLchar**)shader_source, 0);
				gl.CheckError("ShaderSource");
				
				gl.CompileShader(shader);
				gl.CheckError("CompileShader");
				
				gl.GetShaderiv(shader, GL_COMPILE_STATUS, &is_compiled);
				gl.CheckError("GetShaderiv");
				
				assert(is_compiled);
				
				return is_compiled;
			}
			
			char* query_program_info_log(GLObject handle)
			{
				int log_length = 0;
				char* logbuffer = 0;
				gl.GetProgramiv(id, GL_INFO_LOG_LENGTH, &log_length);
				if (log_length > 0)
				{
					logbuffer = (char*)MEMORY_ALLOC(log_length+1, core::memory::global_allocator());
					memset(logbuffer, 0, log_length);
					
					gl.GetProgramInfoLog(handle, log_length, &log_length, logbuffer);
					if (log_length > 0)
					{
						return logbuffer;
					}
					else
					{
						MEMORY_DEALLOC(logbuffer, core::memory::global_allocator());
					}
				}
				
				return 0;
			} // query_program_info_log
			
			void dump_program_log()
			{
				char* logbuffer = query_program_info_log(id);
				if (logbuffer)
				{
//					LOGW("Program Info Log:\n");
//					LOGW("%s\n", logbuffer);
					MEMORY_DEALLOC(logbuffer, core::memory::global_allocator());
				}
			}
		};
		
		struct GLPipeline : public Pipeline
		{
			GLShader* program;
			VertexDescriptor vertex_description;
			
			GLPipeline(const PipelineDescriptor& descriptor)
			{
				program = (GLShader*)descriptor.shader;
				
				// compute the size of the constant buffer we'll need
				size_t total_bytes = sizeof(glm::mat4) * 2;
				
				this->cb = MEMORY_NEW(ConstantBuffer, core::memory::global_allocator())(total_bytes);
			}
			
			
			~GLPipeline()
			{
				MEMORY_DELETE(this->cb, core::memory::global_allocator());
			}
		};
		
		
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
				
				max_size = size_bytes;
				
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
				gl.BufferData(type, size_bytes, data, GL_STREAM_DRAW);
				gl.CheckError("upload -> BufferData");
			}
			
			void setup(const VertexDescriptor& descriptor)
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
		};
		
		class GLCommandSerializer : public CommandSerializer
		{
		public:
			GLCommandSerializer(CommandQueue& command_queue) :
			queue(command_queue)
			{
			}
			
			virtual void vertex_buffer(Buffer* buffer)
			{
				queue.add_command(
								  Command(COMMAND_SET_VERTEX_BUFFER, buffer)
								  );
			}
			
			virtual void draw(size_t initial_offset, size_t total, size_t instance_index, size_t index_count)
			{
				queue.add_command(
								  Command(COMMAND_DRAW, 0, 0, initial_offset, total, instance_index, index_count)
								  );
			}
			
			virtual void draw_indexed_primitives(Buffer* index_buffer, size_t total)
			{
				queue.add_command(
								  Command(COMMAND_DRAW_INDEXED, index_buffer, 0, total, 0, 0, 1)
								  );
			}
			
			virtual void pipeline(Pipeline* pipeline)
			{
				queue.add_command(
								  Command(COMMAND_PIPELINE, pipeline)
								  );
			}
			
			virtual void viewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
			{
				queue.add_command(
								  Command(COMMAND_VIEWPORT, 0, 0, x, y, width, height)
								  );
			}
			
			virtual void texture(const Image& texture, uint32_t index)
			{
			}
			
		private:
			CommandQueue& queue;
		};
		
	private:
		render2::RenderTarget default_target;
		
		CommandQueue* queue[4];
		size_t current_queue;
		
		GLBuffer* locked_buffer;
		
	public:
		OpenGLDevice()
		{
			reset();
		}
		
		~OpenGLDevice()
		{
			
		}
		
	private:
		void reset()
		{
			memset(queue, 0, sizeof(CommandQueue*)*4);
			current_queue = 0;
			locked_buffer = 0;
		}
		
		void draw(
				  GLPipeline* pipeline,
				  GLBuffer* vertex_stream,
				  size_t initial_offset,
				  size_t total,
				  size_t instance_index,
				  size_t index_count)
		{
			activate_pipeline(pipeline, vertex_stream);
			
			vertex_stream->bind_vao();
			gl.DrawArrays(GL_TRIANGLES, initial_offset, total);
			vertex_stream->unbind_vao();
			
			deactivate_pipeline(pipeline);
		}
		
		
		void draw_indexed(
						  GLPipeline* pipeline,
						  GLBuffer* vertex_buffer,
						  GLBuffer* index_buffer,
						  size_t total)
		{
			activate_pipeline(pipeline, vertex_buffer);
			
			
			vertex_buffer->bind_vao();
			
			index_buffer->bind();
			gl.DrawElements(GL_TRIANGLES, total, GL_UNSIGNED_SHORT, 0);
			index_buffer->unbind();
			
			vertex_buffer->unbind_vao();
			
			deactivate_pipeline(pipeline);
		}
		
		
	public:
		
		void activate_pipeline(GLPipeline* pipeline, GLBuffer* vertex_buffer)
		{
			GLShader* shader = pipeline->program;
			gl.UseProgram(shader->id);
			gl.CheckError("activate_pipeline - UseProgram");
			
			// bind uniforms
			size_t offset = 0;
			char* buffer = (char*)pipeline->constants()->get_data();
			
			// TODO: dispatch of various uniform types
			for(size_t index = 0; index < shader->uniforms.size(); ++index)
			{
				shader_variable& uniform = shader->uniforms[index];
				gl.UniformMatrix4fv(uniform.location, uniform.size, GL_FALSE, (GLfloat*)(buffer+offset));
				gl.CheckError("UniformMatrix4fv");
				offset += uniform.byte_size;
			}
			
			// determine if the VAO for the vertex_buffer needs to be built
			if (!vertex_buffer->is_vao_valid())
			{
				vertex_buffer->create_vao();
				
				vertex_buffer->bind_vao();
				
				// mapping tables (from data_type to opengl)
				GLenum gl_type[] = {
					GL_FLOAT,
					GL_FLOAT,
					GL_FLOAT,
					GL_INT,
					GL_UNSIGNED_INT,
					GL_UNSIGNED_BYTE,
					GL_UNSIGNED_BYTE
				};
				
				GLenum gl_normalized[] = {
					GL_FALSE,
					GL_FALSE,
					GL_FALSE,
					GL_TRUE,
					GL_TRUE,
					GL_TRUE,
					GL_TRUE
				};
				
				size_t element_count;
				size_t attribute_size;
				size_t offset = 0;
				
				// If you hit this assert, the shader expects a different number of
				// vertex attributes than the pipeline was configured for.
				assert(pipeline->program->attributes.size() == pipeline->vertex_description.size());
				
				vertex_buffer->bind();
				
				pipeline->vertex_description.reset();
				size_t vertex_stride = pipeline->vertex_description.stride();
				for (size_t index = 0; index < pipeline->vertex_description.size(); ++index)
				{
					VertexDataType type = pipeline->vertex_description[index];
					attribute_size = VertexDescriptor::size_table[ type ];
					element_count = VertexDescriptor::elements[ type ];
					GLenum enum_type = GL_INVALID_ENUM;
					
					enum_type = gl_type[type];
					
					gl.EnableVertexAttribArray(index);
					gl.CheckError( "EnableVertexAttribArray" );
					
					assert(enum_type != GL_INVALID_ENUM);
					
					assert(element_count >= 1 && element_count <= 4);
					
					gl.VertexAttribPointer(index, element_count, enum_type, gl_normalized[type], vertex_stride, (void*)offset);
					gl.CheckError("VertexAttribPointer");
					
					offset += attribute_size;
				}
				
				vertex_buffer->unbind_vao();
				
				vertex_buffer->unbind();
			}
		}
		
		void deactivate_pipeline(GLPipeline* pipeline)
		{
			gl.UseProgram(0);
			gl.CheckError("UseProgram(0)");
		}
		
		virtual void queue_buffers(CommandQueue* const queues, size_t total_queues)
		{
			for (size_t index = 0; index < total_queues; ++index)
				queue[current_queue++] = &queues[index];
		}
		
		// submit queue command buffers to GPU
		virtual void submit()
		{
			// If you hit this assert, there's a buffer locked for writing!
			assert(locked_buffer == nullptr);
			
			for (size_t q = 0; q < current_queue; ++q)
			{
				CommandQueue* cq = queue[q];
				
				// setup pass
				Pass* pass = cq->pass;
				
				gl.ClearColor(pass->clear_color[0], pass->clear_color[1], pass->clear_color[2], pass->clear_color[3]);
				gl.CheckError("ClearColor");
				
				gl.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				gl.CheckError("Clear");
				
				
				GLPipeline* current_pipeline = nullptr;
				GLBuffer* vertex_stream = nullptr;
				GLBuffer* index_stream = nullptr;
				
				for (size_t index = 0; index < cq->total_commands; ++index)
				{
					Command* command = &cq->commands[index];
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
					else if (command->type == COMMAND_VIEWPORT)
					{
						gl.Viewport(command->params[0], command->params[1], command->params[2], command->params[3]);
						gl.CheckError("glViewport");
					}
				}
			}
			
			
			reset();
		}
		
		virtual RenderTarget* default_render_target()
		{
			return &default_target;
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
		
		virtual Pipeline* create_pipeline(const PipelineDescriptor& descriptor)
		{
			GLPipeline* pipeline = MEMORY_NEW(GLPipeline, core::memory::global_allocator())(descriptor);
			pipeline->vertex_description = descriptor.vertex_description;
			
			return pipeline;
		}
		
		virtual void destroy_pipeline(Pipeline* pipeline)
		{
			GLPipeline* glp = static_cast<GLPipeline*>(pipeline);
			destroy_shader(glp->program);
			
			MEMORY_DELETE(glp, core::memory::global_allocator());
		}
		
		virtual Shader* create_shader(const char* name)
		{
			return MEMORY_NEW(GLShader, core::memory::global_allocator());
		}
		
		virtual void destroy_shader(Shader* shader)
		{
			MEMORY_DELETE(shader, core::memory::global_allocator());
		}
		
		virtual void activate_render_target(const RenderTarget& rt)
		{
			
		}
		
		virtual void deactivate_render_target(const RenderTarget& rt)
		{
			
		}
		
		virtual void init(int backbuffer_width, int backbuffer_height)
		{
			default_target.width = backbuffer_width;
			default_target.height = backbuffer_height;
		}
		
		virtual CommandSerializer* create_serializer(CommandQueue& command_queue)
		{
			GLCommandSerializer* serializer = MEMORY_NEW(GLCommandSerializer, core::memory::global_allocator())(command_queue);
			return serializer;
		}
		
		virtual void destroy_serializer(CommandSerializer* serializer)
		{
			MEMORY_DELETE(serializer, core::memory::global_allocator());
		}
	}; // OpenGLDevice
} // namespace render2
