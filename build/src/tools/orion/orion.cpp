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
#include <platform/kernel.h>
#include <platform/windowlibrary.h>
#include <platform/input.h>

#include <core/hashset.h>
#include <core/stackstring.h>
#include <core/fixedarray.h>

#include <runtime/filesystem.h>

#include <nom/nom.hpp>
#include <nom/compositor.hpp>
#include <nom/graph.hpp>
#include <nom/button.hpp>


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
	
	
	struct context
	{
		
	};
	
	struct buffer
	{
		enum
		{
			BUFFER_IS_DIRTY = 1
		};
		
		void* pointer;
		uint32_t max_size;
		uint32_t flags;
		
		buffer()
		{
			pointer = 0;
			max_size = 0;
			flags = 0;
		}
		
		~buffer()
		{
			char* ptr = static_cast<char*>(pointer);
			delete [] ptr;
		}
		
		template <class T>
		T* data()
		{
			return reinterpret_cast<T*>(pointer);
		}
		
		bool is_dirty() const
		{
			return flags & BUFFER_IS_DIRTY;
		}
	};
	
	struct region
	{
		uint32_t x;
		uint32_t y;
		uint32_t width;
		uint32_t height;
	};

	struct shader
	{
	};

	struct sampler_descriptor
	{
		uint32_t min_filter;
		uint32_t mag_filter;
		uint32_t s_address_mode;
		uint32_t t_address_mode;
	};

	struct sampler
	{
	};

	struct image
	{
		// color components
		// ordering
		// component size
		// presence or absence of compression
		uint32_t mip_level;
		uint32_t pixel_format;
	
		
		void update_pixels(const region& rect, size_t miplevel, void* bytes, size_t bytes_per_row)
		{
			
		}
	};

	struct render_target
	{
		uint32_t width;
		uint32_t height;
	};

	struct render_pass
	{
		// color attachments (4)
		// depth attachment
		// stencil attachment
		
		render_target* target;
		float clear_color[4];
		
		void color(float red, float green, float blue, float alpha)
		{
			clear_color[0] = red;
			clear_color[1] = green;
			clear_color[2] = blue;
			clear_color[3] = alpha;
		}
	};
	
	
	enum command_type
	{
		COMMAND_DRAW,
		COMMAND_PIPELINE,
		COMMAND_STATE
	};
	
	struct command
	{
		command_type type;
		void* data[2];
		size_t params[4];
	};

	struct command_queue
	{
		render_pass* pass;
		size_t total_commands;
		command commands[32];
		
		command_queue(render_pass* pass)
		{
			this->pass = pass;
			total_commands = 0;
		}
		
		void draw(buffer* vertex_stream, buffer* index_stream, size_t initial_offset, size_t total, size_t instance_index = 0, size_t index_count = 1)
		{
			command c;
			memset(&c, 0, sizeof(command));
			c.type = COMMAND_DRAW;
			c.data[0] = vertex_stream;
			c.data[1] = index_stream;
			c.params[0] = initial_offset;
			c.params[1] = total;
			c.params[2] = instance_index;
			c.params[3] = index_count;
			commands[total_commands++] = c;
		}
		
		void begin(struct pipeline_state* pipeline)
		{
			command c;
			memset(&c, 0, sizeof(command));
			c.type = COMMAND_PIPELINE;
			c.data[0] = pipeline;
			c.data[1] = 0;
			commands[total_commands++] = c;
		}
		
		void end()
		{
		
		}
		
		
		void texture(const image& texture, uint32_t index) {}
		
	};
	
	enum VertexDescription
	{
		VD_FLOAT2 = 0,
		VD_FLOAT3,
		VD_FLOAT4,
		VD_INT4,
		VD_UNSIGNED_BYTE3,
		VD_UNSIGNED_BYTE4,
		VD_UNSIGNED_INT,
		VD_TOTAL
	};
	
	// describes the layout of the vertex stream
	struct vertex_descriptor
	{
		void add(VertexDescription description) {}
		
		size_t stride() const { return sizeof(float)*7; }
	};


	const uint32_t kMaxPipelineAttachments = 2;
	struct pipeline_descriptor
	{
		shader* shader;
		uint32_t attachments[ kMaxPipelineAttachments ];
		vertex_descriptor vertex_description;
	};



	struct constant_buffer
	{
		size_t max_size;
		void* data;
			
		constant_buffer(size_t total_size)
		{
			data = MEMORY_ALLOC(total_size, platform::memory::global_allocator());
			max_size = total_size;
		}
		
		~constant_buffer()
		{
			MEMORY_DEALLOC(data, platform::memory::global_allocator());
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
	
	struct pipeline_state
	{
	protected:
		constant_buffer* cb;
		
	public:
		constant_buffer* constants() { return cb; }
	};
	

	
	struct device
	{
		virtual ~device() {}
		
		virtual void activate_pipeline(pipeline_state* state) = 0;
		virtual void deactivate_pipeline(pipeline_state* state) = 0;
		
		virtual void queue_buffers(command_queue* const queues, size_t total_queues) = 0;
		
		// submit queue command buffers to GPU
		virtual void submit() = 0;
		
		virtual render_target* default_render_target() = 0;
		
		virtual buffer* create_buffer(size_t size_bytes) = 0;
		virtual void destroy_buffer(buffer* buffer) = 0;
		
		virtual pipeline_state* create_pipeline(const pipeline_descriptor& descriptor) = 0;
		virtual void destroy_pipeline(pipeline_state* pipeline) = 0;
		
		virtual shader* create_shader(const char* name) = 0;
		virtual void destroy_shader(shader* shader) = 0;
		
		virtual void activate_render_target(const render_target& rt) = 0;
		virtual void deactivate_render_target(const render_target& rt) = 0;
		
		virtual void activate_context(const context& c) = 0;
		virtual void swap_context_buffers(const context& c) = 0;
		
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
		GLint index;
	
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
	
	struct glhal : public device
	{

		
		struct gl_shader : public shader
		{
			GLint id;
			FixedArray<shader_variable> uniforms;
			FixedArray<shader_variable> attributes;
			
			gl_shader()
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
				\
				void main()\
				{\
				gl_Position = (projection_matrix * modelview_matrix * in_position);\
				}";
				
				const char* fragment_shader_source = "\
				precision highp float;\
				out vec4 out_color;\
				\
				void main()\
				{\
					out_color = vec4(1.0, 0.0, 1.0, 1.0);\
				}";
				
				bool result = false;
				result = compile_shader(vert, vertex_shader_source, "", "#version 150 core");
				result = compile_shader(frag, fragment_shader_source, "", "#version 150 core");
				
				
				// attach shaders
				gl.AttachShader(id, vert); gl.CheckError("AttachShader (vert)");
				gl.AttachShader(id, frag); gl.CheckError("AttachShader (frag)");
				
				
				// bind attributes
				gl.BindFragDataLocation(id, 0, "out_color");
//				gl.BindAttribLocation(id, 0, "in_position");

				// link and activate shader
				gl.LinkProgram(id);
				GLint is_linked = 0;
				gl.GetProgramiv(id, GL_LINK_STATUS, &is_linked);
				assert(is_linked == 1);
				
				
				// activate program
				gl.UseProgram(id);
				
				
				
				

				
				
				// use introspection
				{
					GLint active_attributes = 0;
					gl.GetProgramiv(id, GL_ACTIVE_ATTRIBUTES, &active_attributes);
					
					attributes.allocate(active_attributes);

					for (size_t attribute_index = 0; attribute_index < active_attributes; ++attribute_index)
					{
						shader_variable& attribute = attributes[attribute_index];
						attribute.index = attribute_index;
						gl.GetActiveAttrib(id, attribute_index, MAX_ATTRIBUTE_NAME_LENGTH, &attribute.length, &attribute.size, &attribute.type, attribute.name);
						LOGV("attribute: %i, name: %s, size: %i, type: %i\n",
							 attribute_index,
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
					
					for (size_t uniform_index = 0; uniform_index < active_uniforms; ++uniform_index)
					{
						shader_variable& uniform = uniforms[uniform_index];
						uniform.index = uniform_index;
						gl.GetActiveUniform(id, uniform_index, MAX_ATTRIBUTE_NAME_LENGTH, &uniform.length, &uniform.size, &uniform.type, uniform.name);
						LOGV("attribute: %i, name: %s, size: %i, type: %i\n",
							 uniform_index,
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
			
			~gl_shader()
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
				gl.CompileShader(shader);
				gl.GetShaderiv(shader, GL_COMPILE_STATUS, &is_compiled);
				assert(is_compiled);
				
				return is_compiled;
			}
		};
	
		struct gl_pipeline : public pipeline_state
		{
			gl_shader* program;

			gl_pipeline(const pipeline_descriptor& descriptor)
			{
				program = (gl_shader*)descriptor.shader;
				
				// compute the size of the constant buffer we'll need
				size_t total_bytes = sizeof(glm::mat4) * 2;
				
				this->cb = MEMORY_NEW(constant_buffer, platform::memory::global_allocator())(total_bytes);
			}
			
			
			~gl_pipeline()
			{
				MEMORY_DELETE(this->cb, platform::memory::global_allocator());
			}
		};
		
	private:
		render2::render_target default_target;
		
		command_queue* queue[4];
		size_t current_queue;
		
		
		
		
		
	public:
		glhal()
		{
			reset();
		}
		
		~glhal()
		{
			
		}
		
	private:
		void reset()
		{
			memset(queue, 0, sizeof(command_queue*)*4);
			current_queue = 0;
		}
		
		void update_stream(buffer* data)
		{
			
		}
		
		void draw(buffer* vertex_stream, buffer* index_stream, size_t initial_offset, size_t total, size_t instance_index, size_t index_count)
		{
			
		}
		
		
	public:
		
		virtual void activate_pipeline(pipeline_state* state)
		{
			LOGV("activate_pipeline %p\n", state);
					
			gl_pipeline* pipeline = static_cast<gl_pipeline*>(state);
			gl_shader* shader = pipeline->program;
			gl.UseProgram(shader->id);
			
			// bind uniforms
			size_t offset = 0;
			char* buffer = (char*)pipeline->constants()->data;
			
			// TODO: dispatch of various uniform types
			for(size_t index = 0; index < shader->uniforms.size(); ++index)
			{
				shader_variable& uniform = shader->uniforms[index];
				gl.UniformMatrix4fv(uniform.index, uniform.size, GL_FALSE, (GLfloat*)(buffer+offset));
				offset += uniform.byte_size;
			}
		}
		
		virtual void deactivate_pipeline(pipeline_state* state)
		{
			LOGV("deactivate_pipeline %p\n", state);
			
//			gl_pipeline* pipeline = static_cast<gl_pipeline*>(state);
//			gl_shader* shader = pipeline->program;
			gl.UseProgram(0);
		}
		
		virtual void queue_buffers(command_queue* const queues, size_t total_queues)
		{
			for (size_t index = 0; index < total_queues; ++index)
				queue[current_queue++] = &queues[index];
		}
		
		// submit queue command buffers to GPU
		virtual void submit()
		{
			for (size_t q = 0; q < current_queue; ++q)
			{
				command_queue* cq = queue[q];
				
				// setup pass
				render_pass* pass = cq->pass;
				
				gl.ClearColor(pass->clear_color[0], pass->clear_color[1], pass->clear_color[2], pass->clear_color[3]);
				gl.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				
				// TODO: execute commands
				
				
				pipeline_state* current_pipeline = nullptr;
				pipeline_state* last_pipeline = nullptr;
				buffer* vertex_stream = nullptr;
				buffer* index_stream = nullptr;
				
				for (size_t index = 0; index < cq->total_commands; ++index)
				{
					command* command = &cq->commands[index];
					if (command->type == COMMAND_DRAW)
					{
						vertex_stream = (buffer*)command->data[0];
						index_stream = (buffer*)command->data[1];
						if (vertex_stream->is_dirty())
						{
							update_stream(vertex_stream);
						}
						
						if (index_stream && index_stream->is_dirty())
						{
							update_stream(index_stream);
						}
						
						draw(vertex_stream,
							index_stream,
							command->params[0],
							command->params[1],
							command->params[2],
							command->params[3]
						);
					}
					else if (command->type == COMMAND_PIPELINE)
					{
						current_pipeline = (pipeline_state*)command->data[0];
						
						if (current_pipeline != last_pipeline)
						{
							last_pipeline = current_pipeline;
							
							if (last_pipeline != 0)
							{
								deactivate_pipeline(last_pipeline);
							}
							
							activate_pipeline(current_pipeline);
						}
					}
					
				}
			}
		
		
			reset();
		}
		
		virtual render_target* default_render_target()
		{
			return &default_target;
		}
		
		virtual buffer* create_buffer(size_t size_bytes)
		{
			buffer* b = MEMORY_NEW(buffer, platform::memory::global_allocator());
			b->pointer = new char[size_bytes];
			b->max_size = size_bytes;
			
			return b;
		}
		
		virtual void destroy_buffer(buffer* buffer)
		{
			MEMORY_DELETE(buffer, platform::memory::global_allocator());
		}
		
		virtual pipeline_state* create_pipeline(const pipeline_descriptor& descriptor)
		{
			gl_pipeline* pipeline = MEMORY_NEW(gl_pipeline, platform::memory::global_allocator())(descriptor);
	
		
			return pipeline;
		}
		
		virtual void destroy_pipeline(pipeline_state* pipeline)
		{
			gl_pipeline* glp = static_cast<gl_pipeline*>(pipeline);
			destroy_shader(glp->program);

			MEMORY_DELETE(glp, platform::memory::global_allocator());
		}
		
		virtual shader* create_shader(const char* name)
		{
			gl_shader* s = new gl_shader();
			return s;
		}
		
		virtual void destroy_shader(shader* shader)
		{
			gl_shader* s = static_cast<gl_shader*>(shader);
			delete s;
		}
		
		virtual void activate_render_target(const render_target& rt)
		{
			
		}
		
		virtual void deactivate_render_target(const render_target& rt)
		{
			
		}
		
		virtual void activate_context(const context& c)
		{
			
		}
		
		virtual void swap_context_buffers(const context& c)
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
	
	device* create_device(const RenderParameters& params)
	{
		// determine the renderer
		assert(params.has_key("renderer"));
		
		const param_string& renderer = params["renderer"];
		LOGV("create renderer '%s'\n", renderer());
	
	
		return new glhal();
	}
	
	void destroy_device(device* device)
	{
		delete device;
	}
	
	
	
		
}







class EditorKernel : public kernel::IKernel,
public kernel::IEventListener<kernel::KeyboardEvent>
{
private:
	bool active;
	platform::IWindowLibrary* window_interface;
	platform::NativeWindow* main_window;
	
	render2::device* device;
	
	render2::buffer* triangle_stream;
	render2::pipeline_state* pipeline;
	
	GLsync fence;
	
public:
	EditorKernel() :
		active(true),
		window_interface(0)
	{
	}
	
	virtual ~EditorKernel() {}
	
	virtual bool is_active() const { return active; }
	virtual void set_active(bool isactive) { active = isactive; }
	
	virtual void resolution_changed(int width, int height)
	{
		LOGV("resolution_changed %i %i\n", width, height);
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
		{
			window_interface = platform::create_window_library();
			window_interface->startup(kernel::parameters());
		
			platform::WindowParameters window_params;
			window_params.window_width = 800;
			window_params.window_height = 600;
			window_params.window_title = "orion";
//			window_params.enable_fullscreen = true;
			main_window = window_interface->create_window(window_params);
			window_interface->show_mouse(true);
		}
		
		// old renderer initialize
		{
			renderer::RenderSettings render_settings;
			render_settings.gamma_correct = true;
			renderer::startup(renderer::OpenGL, render_settings);
			
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
			params["renderer"] = "opengl";
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
			
			// setup debug draw (example)
			if (1)
			{
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
			
				// setup shaders
				render2::pipeline_descriptor desc;
				desc.shader = device->create_shader("test");
				desc.vertex_description.add(render2::VD_FLOAT3); // position
				desc.vertex_description.add(render2::VD_FLOAT4); // color
				pipeline = device->create_pipeline(desc);
							
				// populate buffer
				assert(desc.vertex_description.stride() == sizeof(MyVertex));

				size_t total_bytes = desc.vertex_description.stride() * 3;
				triangle_stream = device->create_buffer(total_bytes);
				MyVertex* vertex = triangle_stream->data<MyVertex>();
				
				vertex[0].set_position(0, 0, 0);
				vertex[0].set_color(1.0f, 0.0f, 0.0f, 1.0f);
				
				vertex[1].set_position(0, 100, 0);
				vertex[1].set_color(0.0f, 1.0f, 0.0f, 1.0f);
				
				vertex[2].set_position(100, 100, 0);
				vertex[2].set_color(0.0f, 0.0f, 1.0f, 1.0f);

#if 0
				// setup render target and clear color
				render2::render_pass pass;
				pass.target = device->default_render_target();
				pass.color(0.0f, 0.0f, 1.0f, 1.0f);
				
//				render2::image tex0; // load this
				
				render2::command_queue queue(&pass);
				queue.begin(pipeline);
//				queue.texture(tex0, 0);
				queue.draw(triangle_stream, 0, 0, 3);
				queue.draw(triangle_stream, 0, 3, 6);
				queue.draw(triangle_stream, 0, 12, 15);
				queue.end();
				
				device->queue_buffers(&queue, 1);
				
				// buffers are queued; submit all buffers for drawing
				
				device->submit();
#endif
			}
			

			
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
		window_interface->process_events();
		
				
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
		cb.projection_matrix = glm::ortho(0.0f, 800.0f, 600.0f, 0.0f, 0.1f, 1.0f);
		pipeline->constants()->assign(&cb, sizeof(leconstants));
						
		// clear the screen
		{
			render2::render_pass render_pass;
			render_pass.target = device->default_render_target();
			render_pass.color(value, value, value, 1.0f);
			
			render2::command_queue queue(&render_pass);
			queue.begin(pipeline);
			queue.draw(triangle_stream, 0, 0, 3);
			queue.end();
			
			device->queue_buffers(&queue, 1);
			device->submit();
		}

		
//		glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 2000);
		
		window_interface->swap_buffers(main_window);
	}
	
	virtual void shutdown()
	{
		// shutdown the render device
		
		device->destroy_buffer(triangle_stream);
		device->destroy_pipeline(pipeline);
		
		
		destroy_device(device);
		
		glDeleteSync(fence);
		
		renderer::shutdown();
	
		window_interface->destroy_window(main_window);
		window_interface->shutdown();
		platform::destroy_window_library();
		core::shutdown();
	}
	
	
	virtual void event(kernel::KeyboardEvent& event)
	{
		if (event.key == input::KEY_ESCAPE && event.is_down)
		{
			set_active(false);
		}
	}
	
};



PLATFORM_MAIN
{
	int return_code;
	return_code = platform::run_application(new EditorKernel());
	return return_code;
}