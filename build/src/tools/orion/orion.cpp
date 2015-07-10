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

#include <renderer/debug_draw.h>
#include <renderer/gl/gemgl.h>

#include <runtime/core.h>
#include <runtime/logging.h>

#include <platform/platform.h>
#include <platform/window.h>
#include <platform/kernel.h>
#include <platform/input.h>

#include <core/hashset.h>
#include <core/stackstring.h>
#include <core/fixedarray.h>

#include <runtime/filesystem.h>

#include <nom/nom.hpp>
#include <nom/compositor.hpp>
#include <nom/graph.hpp>
#include <nom/button.hpp>

// when defined; uses the old method for creating windows
//#define USE_WINDOW_LIBRARY

#if defined(PLATFORM_SDL2_SUPPORT)
	#include <platform/windowlibrary.h>
#endif

using namespace platform;
using namespace renderer;

namespace render2
{
	typedef core::StackString<128> param_string;
	typedef HashSet<param_string, param_string, core::util::hash> RenderParameters;
	
	enum ClearFlags
	{
		ClearFlag_Invalid,
		ClearFlag_ColorBuffer,
		ClearFlag_DepthBuffer
	};
	
	
	// 1. uploading all buffer data to GPU in one call
	// 2. uploading a part of the buffer data to the GPU in one call
	// 3. retrieving all or part of the buffer data via function to populate it
	struct Buffer
	{
		enum
		{
			BUFFER_IS_DIRTY = 1
		};

		uint32_t max_size;
		uint32_t flags;
		
		Buffer()
		{
			max_size = 0;
			flags = 0;
		}
		
		virtual ~Buffer() {}
		
		size_t max_size_bytes() const { return max_size; }
		
		bool is_dirty() const
		{
			return flags & BUFFER_IS_DIRTY;
		}
		
		void clear_flag(uint32_t flag)
		{
			flags &= ~flag;
		}
		
	};
	
	struct Region
	{
		uint32_t x;
		uint32_t y;
		uint32_t width;
		uint32_t height;
	};

	struct Shader
	{
	};

	struct TextureDescriptor
	{
		uint32_t min_filter;
		uint32_t mag_filter;
		uint32_t s_address_mode;
		uint32_t t_address_mode;
	};

	struct Textre
	{
	};

	struct Image
	{
		// color components
		// ordering
		// component size
		// presence or absence of compression
		uint32_t mip_level;
		uint32_t pixel_format;
	
		
		void update_pixels(const Region& rect, size_t miplevel, void* bytes, size_t bytes_per_row)
		{
			
		}
	};

	struct RenderTarget
	{
		uint32_t width;
		uint32_t height;
	};

	struct Pass
	{
		// color attachments (4)
		// depth attachment
		// stencil attachment
		
		RenderTarget* target;
		float clear_color[4];
		
		void color(float red, float green, float blue, float alpha)
		{
			clear_color[0] = red;
			clear_color[1] = green;
			clear_color[2] = blue;
			clear_color[3] = alpha;
		}
	};
	
	
	enum CommandType
	{
		COMMAND_SET_VERTEX_BUFFER,	// change vertex buffer
		COMMAND_DRAW,				// draw from vertex buffer
		COMMAND_DRAW_INDEXED,		// draw from index buffer
		COMMAND_PIPELINE,			// set the rendering pipeline
		COMMAND_VIEWPORT,			// set viewport
		COMMAND_STATE
	};
	
	struct Command
	{
		CommandType type;
		void* data[2];
		size_t params[4];
		
		Command()
		{
			memset(this, 0, sizeof(Command));
		}
	};

	struct CommandQueue
	{
		Pass* pass;
		size_t total_commands;
		Command commands[32];
		
		CommandQueue(Pass* pass)
		{
			this->pass = pass;
			total_commands = 0;
		}
		
		void vertex_buffer(Buffer* buffer)
		{
			Command c;
			memset(&c, 0, sizeof(Command));
			c.type = COMMAND_SET_VERTEX_BUFFER;
			c.data[0] = buffer;
			commands[total_commands++] = c;
		}
		
		void draw(size_t initial_offset, size_t total, size_t instance_index = 0, size_t index_count = 1)
		{
			Command c;
			memset(&c, 0, sizeof(Command));
			c.type = COMMAND_DRAW;
			c.params[0] = initial_offset;
			c.params[1] = total;
			c.params[2] = instance_index;
			c.params[3] = index_count;
			commands[total_commands++] = c;
		}
		
		void draw_indexed_primitives(Buffer* index_buffer, size_t total)
		{
			Command c;
			memset(&c, 0, sizeof(Command));
			c.type = COMMAND_DRAW_INDEXED;
			c.data[0] = index_buffer;
			c.params[0] = total;
			c.params[1] = 0;
			c.params[2] = 0;
			c.params[3] = 1;
			commands[total_commands++] = c;
		}
		
		void pipeline(struct Pipeline* pipeline)
		{
			Command c;
			memset(&c, 0, sizeof(Command));
			c.type = COMMAND_PIPELINE;
			c.data[0] = pipeline;
			c.data[1] = 0;
			commands[total_commands++] = c;
		}
		
		void viewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
		{
			Command c;
			c.type = COMMAND_VIEWPORT;
			c.params[0] = x;
			c.params[1] = y;
			c.params[2] = width;
			c.params[3] = height;
			commands[total_commands++] = c;
		}
		
		
		void texture(const Image& texture, uint32_t index) {}
		
	};
	
	struct CommandSerializer
	{
		virtual ~CommandSerializer() {}
		
		virtual void vertex_buffer(Buffer* buffer) = 0;
		virtual void draw(size_t initial_offset, size_t total, size_t instance_index = 0, size_t index_count = 1) = 0;
		virtual void draw_indexed_primitives(Buffer* index_buffer, size_t total) = 0;
		virtual void pipeline(struct Pipeline* pipeline) = 0;
		virtual void viewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
		virtual void texture(const Image& texture, uint32_t index) = 0;
	};
	
	enum VertexDataType
	{
		VD_FLOAT2 = 0,
		VD_FLOAT3,
		VD_FLOAT4,
		VD_INT4,
		VD_UNSIGNED_INT,
		VD_UNSIGNED_BYTE3,
		VD_UNSIGNED_BYTE4,
		VD_TOTAL
	};
	
	const size_t MAX_VERTEX_DESCRIPTORS = 8;
	
	// describes the layout of the vertex stream
	struct VertexDescriptor
	{
		unsigned char id;
		unsigned char total_attributes;
		VertexDataType description[ MAX_VERTEX_DESCRIPTORS ];
		
		static void startup();
		static void map_type(uint32_t type, uint16_t size, uint16_t elements);
		static uint16_t size_table[ VD_TOTAL ];
		static uint16_t elements[ VD_TOTAL ];
		
		VertexDescriptor();
		VertexDescriptor(const VertexDescriptor& other);
		void add(const VertexDataType& type);
		const VertexDataType& operator[](int index) const;
		void reset();
		size_t stride() const;
		size_t size() const;
		
		const VertexDescriptor& operator= (const VertexDescriptor & other);
	}; // VertexDescriptor
	

	uint16_t VertexDescriptor::size_table[ VD_TOTAL ] = {0};
	uint16_t VertexDescriptor::elements[ VD_TOTAL ] = {0};
	
	void VertexDescriptor::startup()
	{
		// clear table
		memset(VertexDescriptor::size_table, 0, sizeof(uint16_t)*VD_TOTAL);
		memset(VertexDescriptor::elements, 0, sizeof(uint16_t)*VD_TOTAL);
		
		// populate table with vertex descriptor types
		map_type(VD_FLOAT2, sizeof(float), 2);
		map_type(VD_FLOAT3, sizeof(float), 3);
		map_type(VD_FLOAT4, sizeof(float), 4);
		
		map_type(VD_INT4, sizeof(int), 4);
		
		map_type(VD_UNSIGNED_INT, sizeof(unsigned int), 1);
		map_type(VD_UNSIGNED_BYTE3, sizeof(unsigned char), 3);
		map_type(VD_UNSIGNED_BYTE4, sizeof(unsigned char), 4);
				
		// validate table
		for (size_t i = 0; i < VD_TOTAL; ++i)
		{
			assert(VertexDescriptor::size_table[i] != 0);
			assert(VertexDescriptor::elements[i] != 0);
		}
	}
	
	void VertexDescriptor::map_type(uint32_t type, uint16_t size, uint16_t elements)
	{
		VertexDescriptor::size_table[type] = size*elements;
		VertexDescriptor::elements[type] = elements;
	}
	
	VertexDescriptor::VertexDescriptor()
	{
		id = 0;
		reset();
		memset(description, 0, sizeof(VertexDataType) * MAX_VERTEX_DESCRIPTORS);
	}
	
	VertexDescriptor::VertexDescriptor(const VertexDescriptor& other)
	{
		*this = other;
	}
	
	void VertexDescriptor::add(const VertexDataType& desc)
	{
		description[ id ] = desc;
		++id;
		
		if ( id >= MAX_VERTEX_DESCRIPTORS-1 )
		{
			printf( "Reached MAX_DESCRIPTORS. Resetting\n" );
			id = 0;
		}
		
		total_attributes = id;
	} // add
	
	const VertexDataType& VertexDescriptor::operator[](int index) const
	{
		return description[ index ];
	} // operator[]
	
	void VertexDescriptor::reset()
	{
		if ( id > 0 )
		{
			total_attributes = id;
		}
		id = 0;
	} // reset
	
	size_t VertexDescriptor::stride() const
	{
		size_t size = 0;
		VertexDataType type;
		for(size_t index = 0; index < total_attributes; ++index)
		{
			type = description[index];
			size += VertexDescriptor::size_table[ type ];
		}
		
		return size;
	} // stride
	
	size_t VertexDescriptor::size() const
	{
		return total_attributes;
	} // size
	
	const VertexDescriptor& VertexDescriptor::operator= (const VertexDescriptor & other)
	{
		this->total_attributes = other.total_attributes;
		this->id = other.id;
		
		for( unsigned int i = 0; i < VD_TOTAL; ++i )
		{
			this->size_table[i] = other.size_table[i];
			this->elements[i] = other.elements[i];
		}
		
		for( unsigned int id = 0; id < MAX_VERTEX_DESCRIPTORS; ++id )
		{
			this->description[id] = other.description[id];
		}
		
		return *this;
	} // operator=
	
	


	const uint32_t kMaxPipelineAttachments = 2;
	struct PipelineDescriptor
	{
		Shader* shader;
		uint32_t attachments[ kMaxPipelineAttachments ];
		VertexDescriptor vertex_description;
	};

	struct ConstantBuffer
	{
		size_t max_size;
		void* data;
			
		ConstantBuffer(size_t total_size)
		{
			data = MEMORY_ALLOC(total_size, core::memory::global_allocator());
			max_size = total_size;
		}
		
		~ConstantBuffer()
		{
			MEMORY_DEALLOC(data, core::memory::global_allocator());
			max_size = 0;
		}
		
		void assign(const void* src, const size_t bytes)
		{
			assert(bytes <= max_size);
			memcpy(data, src, bytes);
		}
	};

	// this will describe
	// - the shaders to use
	// - the vertex description
	// - uniform
	
	struct Pipeline
	{
	protected:
		ConstantBuffer* cb;
		
	public:
		ConstantBuffer* constants() { return cb; }
	};
	
	// Most calls to the device should be executed synchronously. These are not
	// deferred.
	//
	
	
	
	struct Device
	{
		virtual ~Device() {}
		
		virtual void queue_buffers(CommandQueue* const queues, size_t total_queues) = 0;
		
		// submit queue command buffers to GPU
		virtual void submit() = 0;
		
		virtual RenderTarget* default_render_target() = 0;
		
		virtual Buffer* create_vertex_buffer(size_t size_bytes) = 0;
		virtual Buffer* create_index_buffer(size_t size_bytes) = 0;
		virtual void destroy_buffer(Buffer* buffer) = 0;
		
		
		// retrieves a pointer to buffer's data; locks it for write
		virtual void* buffer_lock(Buffer* buffer) = 0;
		
		// unlock a previously locked buffer
		virtual void buffer_unlock(Buffer* buffer) = 0;
		
		// upload data to a buffer (should not exceed buffer's max size)
		virtual void buffer_upload(Buffer* buffer, void* data, size_t data_size) = 0;
//		virtual void buffer_upload(Buffer* buffer, size_t offset, void* data, size_t data_size) = 0;
		
		
		
		virtual Pipeline* create_pipeline(const PipelineDescriptor& descriptor) = 0;
		virtual void destroy_pipeline(Pipeline* pipeline) = 0;
		
		virtual Shader* create_shader(const char* name) = 0;
		virtual void destroy_shader(Shader* shader) = 0;
		
		virtual void activate_render_target(const RenderTarget& rt) = 0;
		virtual void deactivate_render_target(const RenderTarget& rt) = 0;
		
		virtual void init(int backbuffer_width, int backbuffer_height) = 0;
	};

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
				result = compile_shader(vert, vertex_shader_source, "", "#version 150 core");
				result = compile_shader(frag, fragment_shader_source, "", "#version 150 core");
				
				
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
						LOGV("attribute: %i, location: %i, name: %s, size: %i, type: %i\n",
							 attribute_index,
							 attribute.location,
							 attribute.name,
							 attribute.size,
							 attribute.type);
							
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
						LOGV("attribute: %i, location: %i, name: %s, size: %i, type: %i\n",
							 uniform_index,
							 uniform.location,
							 uniform.name,
							 uniform.size,
							 uniform.type);
							 
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
					LOGW("Program Info Log:\n");
					LOGW("%s\n", logbuffer);
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
			char* buffer = (char*)pipeline->constants()->data;
			
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
			GLShader* s = new GLShader();
			return s;
		}
		
		virtual void destroy_shader(Shader* shader)
		{
			GLShader* s = static_cast<GLShader*>(shader);
			delete s;
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
	};

	template <class O, class I>
	O convert(const I& input)
	{
		O a;
		return a;
	}
	
	template <>
	int convert(const param_string& s)
	{
		return atoi(s());
	}
	
	Device* create_device(const RenderParameters& params)
	{
		// determine the renderer
		assert(params.has_key("rendering_backend"));
		
		const param_string& renderer = params["rendering_backend"];
		LOGV("create device for rendering_backend '%s'\n", renderer());
	
	
		return new OpenGLDevice();
	}
	
	void destroy_device(Device* device)
	{
		delete device;
	}
	
	
	
		
}

class EditorKernel : public kernel::IKernel,
public kernel::IEventListener<kernel::KeyboardEvent>,
public kernel::IEventListener<kernel::MouseEvent>,
public kernel::IEventListener<kernel::SystemEvent>
{
private:
	bool active;
#if defined(USE_WINDOW_LIBRARY)
	platform::IWindowLibrary* window_interface;
#endif
	platform::window::NativeWindow* main_window;
	
	render2::Device* device;
	
	render2::Buffer* triangle_stream;
	render2::Buffer* index_buffer;
	render2::Pipeline* pipeline;
	
	GLsync fence;
	
	struct MyVertex
	{
		float position[3];
		float color[4];
		
		void set_position(float x, float y, float z)
		{
			position[0] = x;
			position[1] = y;
			position[2] = z;
		}
		
		void set_color(float red, float green, float blue, float alpha)
		{
			color[0] = red;
			color[1] = green;
			color[2] = blue;
			color[3] = alpha;
		}
	};
	
public:
	EditorKernel() :
		active(true)
#if defined(USE_WINDOW_LIBRARY)
		, window_interface(0)
#endif
	{
	}
	
	virtual ~EditorKernel() {}
	
	virtual bool is_active() const { return active; }
	virtual void set_active(bool isactive) { active = isactive; }
	
	virtual void event(kernel::SystemEvent& event)
	{
		if (event.subtype == kernel::WindowGainFocus)
		{
			LOGV("window gained focus\n");
		}
		else if (event.subtype == kernel::WindowLostFocus)
		{
			LOGV("window lost focus\n");
		}
		else if (event.subtype == kernel::WindowResized)
		{
			LOGV("resolution_changed %i %i\n", event.render_width, event.render_height);
		}
	}
	
	virtual void event(kernel::MouseEvent& event)
	{
		if (event.subtype == kernel::MouseWheelMoved)
		{
			LOGV("wheel direction: %i\n", event.wheel_direction);
		}
		else if (event.subtype == kernel::MouseMoved)
		{
			LOGV("mouse moved: %i %i [%i %i]\n", event.mx, event.my, event.dx, event.dy);
		}
		else if (event.subtype == kernel::MouseButton)
		{
			LOGV("mouse button: %s, %i -> %s\n", event.is_down ? "Yes" : "No", event.button, input::mouse_button_name(event.button));
		}
		else
		{
			LOGV("mouse event: %i\n", event.subtype);
		}
	}

	virtual kernel::Error startup()
	{
		platform::PathString root_path;
		platform::PathString content_path;
		platform::get_program_directory(&root_path[0], root_path.max_size());
		platform::fs_content_directory(content_path, root_path);
		
		platform::PathString application_path = platform::get_user_application_directory("arcfusion.net/orion");
		core::startup_filesystem();
		core::filesystem::instance()->root_directory(root_path);
		core::filesystem::instance()->content_directory(content_path);
		core::filesystem::instance()->user_application_directory(application_path);
		
		core::startup_logging();
		
		// create a platform window
#if USE_WINDOW_LIBRARY
		{
			window_interface = platform::create_window_library();
			window_interface->startup(kernel::parameters());
		
			platform::WindowParameters window_params;
			window_params.window.width = 800;
			window_params.window.height = 600;
			window_params.window_title = "orion";
//			window_params.enable_fullscreen = true;
			main_window = window_interface->create_window(window_params);
			window_interface->show_mouse(true);
		}
#else
		{
			platform::window::startup(platform::window::RenderingBackend_Default);
			
			//	uint32_t rcaps = platform::get_renderinterface_caps();
			//	fprintf(stdout, "RenderBackends: %i\n", rcaps);

			fprintf(stdout, "total screens: %zu\n", platform::window::screen_count());
			
			for (size_t screen = 0; screen < platform::window::screen_count(); ++screen)
			{
				platform::window::Frame frame = platform::window::screen_frame(screen);
				fprintf(stdout, "screen rect: %zu, origin: %i, %i; resolution: %i x %i\n", screen, frame.x, frame.y, frame.width, frame.height);
			}
			
			platform::window::Parameters params;
			params.window.width = 800;
			params.window.height = 600;
			params.window_title = "orion";
			params.target_display = 0;
			main_window = platform::window::create(params);
		}
#endif

		// old renderer initialize
		{
			renderer::RenderSettings render_settings;
			render_settings.gamma_correct = true;
						
			renderer::startup(renderer::OpenGL, render_settings);
			
			// clear errors
			gl.CheckError("before render startup");
			
			// need to setup the mapping tables!
			render2::VertexDescriptor::startup();
			
			fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		}
		
		// initialize the renderer
		{
			using namespace render2;
			RenderParameters params;
			
			// set some options
			params["vsync"] = "true";
			params["double_buffer"] = "true";
			params["depth_size"] = "24";
			params["multisample"] = "4";

			// set opengl specific options
			params["rendering_backend"] = "opengl";
			params["opengl.major"] = "3";
			params["opengl.minor"] = "2";
			params["opengl.profile"] = "core";
			params["opengl.share_context"] = "true";
			
			for (RenderParameters::Iterator it = params.begin(); it != params.end(); ++it)
			{
				const param_string& key = it.key();
				const param_string& value = it.value();
				LOGV("'%s' -> '%s'\n", key(), value());
			}
			
			
			device = create_device(params);
			
			device->init(800, 600);
			
			

			// setup shaders
			render2::PipelineDescriptor desc;
			desc.shader = device->create_shader("test");
			desc.vertex_description.add(render2::VD_FLOAT3); // position
			desc.vertex_description.add(render2::VD_FLOAT4); // color
			pipeline = device->create_pipeline(desc);
						
			// populate buffer
			assert(desc.vertex_description.stride() == sizeof(MyVertex));

			size_t total_bytes = desc.vertex_description.stride() * 4;
			triangle_stream = device->create_vertex_buffer(total_bytes);
			MyVertex* vertex = reinterpret_cast<MyVertex*>(device->buffer_lock(triangle_stream));
			
			
			vertex[0].set_position(0, 600, 0);
			vertex[0].set_color(1.0f, 0.0f, 0.0f, 1.0f);
			
			vertex[1].set_position(800, 600, 0);
			vertex[1].set_color(0.0f, 1.0f, 0.0f, 1.0f);
			
			vertex[2].set_position(400, 0, 0);
			vertex[2].set_color(0.0f, 0.0f, 1.0f, 1.0f);
			
			vertex[3].set_position(0, 0, 0);
			vertex[3].set_color(0.0f, 1.0f, 1.0f, 1.0f);

			device->buffer_unlock(triangle_stream);
			
			
			index_buffer = device->create_index_buffer(sizeof(uint16_t) * 3);
			uint16_t indices[] = {0, 1, 2};
			device->buffer_upload(index_buffer, indices, sizeof(uint16_t)*3);

			
			// load shaders from disk
			
		}
		
		// setup editor assets / content paths
		{
//			fs->add_virtual_path("editor/assets");
		}

		
		// load the gui
		{
			// gui layout

			core::filesystem::IFileSystem* fs = core::filesystem::instance();
			
			platform::File handle = platform::fs_open("ui/main.ui", platform::FileMode_Read);
			if (handle.is_open())
			{
//				core::DataStream* stream = fs->memory_from_file(handle);
			
				// create the gui elements from a file
//				compositor->create_layout_from_memory(stream->get_data(), stream->get_data_size());
			
				platform::fs_close(handle);
			}
			
		}
		
		return kernel::NoError;
	}
	

	
	virtual void tick()
	{
#if defined(USE_WINDOW_LIBRARY)
		if (window_interface)
			window_interface->process_events();
#else
		platform::window::dispatch_events();
#endif
		
				
		static float value = 0.0f;
		static float multiplifer = 1.0f;
		
		value += 0.01f * multiplifer;
		value = glm::clamp(value, 0.0f, 1.0f);
		if (value == 0.0f || value == 1.0f)
			multiplifer *= -1;
		
		
		struct leconstants
		{
			glm::mat4 modelview_matrix;
			glm::mat4 projection_matrix;
		};
		
		leconstants cb;
		cb.modelview_matrix = glm::mat4(1.0f);
		cb.projection_matrix = glm::ortho(0.0f, 800.0f, 600.0f, 0.0f, -1.0f, 1.0f);
		pipeline->constants()->assign(&cb, sizeof(leconstants));


		render2::Pass render_pass;
		render_pass.target = device->default_render_target();
		render_pass.color(value, value, value, 1.0f);

		render2::CommandQueue queue(&render_pass);
		queue.pipeline(pipeline);
		queue.vertex_buffer(triangle_stream);
//		queue.draw_indexed_primitives(index_buffer, 3);
		queue.draw(0, 3);
		
#if !defined(USE_WINDOW_LIBRARY)
		platform::window::begin_rendering(main_window);
#endif
		
		
		device->queue_buffers(&queue, 1);
		device->submit();
		
		
#if !defined(USE_WINDOW_LIBRARY)
		platform::window::end_rendering(main_window);
#endif

//		glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 2000);
		
#if defined(USE_WINDOW_LIBRARY)
		if (window_interface)
		{
			window_interface->swap_buffers(main_window);
		}
#endif
	}
	
	virtual void shutdown()
	{
		// shutdown the render device
		
		device->destroy_buffer(triangle_stream);
		device->destroy_buffer(index_buffer);
		device->destroy_pipeline(pipeline);
		
		
		destroy_device(device);
		
		glDeleteSync(fence);
		
		renderer::shutdown();
#if defined(USE_WINDOW_LIBRARY)
		if (window_interface)
		{
			window_interface->destroy_window(main_window);
			window_interface->shutdown();
			platform::destroy_window_library();
		}
#else
		platform::window::destroy(main_window);
		platform::window::shutdown();
#endif

		core::shutdown();
	}
	
	
	virtual void event(kernel::KeyboardEvent& event)
	{
		if (event.key == input::KEY_ESCAPE && event.is_down)
		{
			set_active(false);
		}
		else
		{
			LOGV("key is_down: '%s', name: '%s', modifiers: %zu\n", event.is_down ? "Yes" : "No", input::key_name(event.key), event.modifiers);
		}
	}

};

PLATFORM_MAIN
{
	int return_code;
	return_code = platform::run_application(new EditorKernel());
	return return_code;
}