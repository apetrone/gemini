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
#include "r2_opengl_common.h"
#include "gemgl.h"
#include <runtime/logging.h>

using namespace renderer;

namespace render2
{
	// ---------------------------------------------------------------------
	// GLInputLayout
	// ---------------------------------------------------------------------


	// ---------------------------------------------------------------------
	// GLShader
	// ---------------------------------------------------------------------
	GLShader::GLShader()
	{
		id = gl.CreateProgram();
	}
	
	GLShader::~GLShader()
	{
		uniforms.clear();
		attributes.clear();
		gl.DeleteProgram(id);
		gl.CheckError("DeleteProgram");
	}
	
	
	bool GLShader::compile_shader(GLint shader, const char* source, const char* preprocessor_defines, const char* version)
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
	} // compile_shader
	
	char* GLShader::query_program_info_log(GLObject handle)
	{
		int log_length = 0;
		char* logbuffer = 0;
		gl.GetProgramiv(id, GL_INFO_LOG_LENGTH, &log_length);
		if (log_length > 0)
		{
			logbuffer = (char*)MEMORY_ALLOC(log_length+1, core::memory::global_allocator());
			memset(logbuffer, 0, log_length);
			
			gl.GetProgramInfoLog(handle, log_length, &log_length, logbuffer);
			gl.CheckError("GetProgramInfoLog");

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
	
	void GLShader::dump_program_log()
	{
		char* logbuffer = query_program_info_log(id);
		if (logbuffer)
		{
			//					LOGW("Program Info Log:\n");
			//					LOGW("%s\n", logbuffer);
			MEMORY_DEALLOC(logbuffer, core::memory::global_allocator());
		}
	} // dump_program_log
	
	int GLShader::build_from_source(const char *vertex_shader, const char *fragment_shader, const char* preprocessor, const char* version)
	{
		GLint vert = gl.CreateShader(GL_VERTEX_SHADER); gl.CheckError("CreateShader");
		GLint frag = gl.CreateShader(GL_FRAGMENT_SHADER); gl.CheckError("CreateShader");
		
		
		bool result = false;
		result = compile_shader(vert, vertex_shader, preprocessor, version);
		result = compile_shader(frag, fragment_shader, preprocessor, version);
		
		
		// attach shaders
		gl.AttachShader(id, vert); gl.CheckError("AttachShader (vert)"); gl.CheckError("AttachShader");
		gl.AttachShader(id, frag); gl.CheckError("AttachShader (frag)"); gl.CheckError("AttachShader");
		
#if defined(PLATFORM_OPENGL_SUPPORT)
		// bind attributes
		gl.BindFragDataLocation(id, 0, "out_color");
		gl.CheckError("BindFragDataLocation");
#endif
		
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
		
		return 0;
	}
	
	GLint GLShader::get_attribute_location(const char* name)
	{
		return gl.GetAttribLocation(id, name);
	}
	
	GLint GLShader::get_uniform_location(const char* name)
	{
		return gl.GetUniformLocation(id, name);
	}
	
	// ---------------------------------------------------------------------
	// GLPipeline
	// ---------------------------------------------------------------------
	GLPipeline::GLPipeline(const PipelineDescriptor& descriptor) :
		enable_blending(false),
		blend_source(GL_ONE),
		blend_destination(GL_ZERO)
	{
		program = (GLShader*)descriptor.shader;
		
		// compute the size of the constant buffer we'll need
		size_t total_bytes = sizeof(glm::mat4) * 2;
		
		this->cb = MEMORY_NEW(ConstantBuffer, core::memory::global_allocator())(total_bytes);
	}
	
	GLPipeline::~GLPipeline()
	{
		MEMORY_DELETE(this->cb, core::memory::global_allocator());
	}
	
	
	// ---------------------------------------------------------------------
	// implementation
	// ---------------------------------------------------------------------
	VertexDataTypeToGL _vertex_data_to_gl[ VD_TOTAL ];
	#define VERTEXDATA_TO_GL(vdt, gl_type, gl_normalized, element_size) \
		_vertex_data_to_gl[ vdt ] = VertexDataTypeToGL(gl_type, gl_normalized, element_size)


	// ---------------------------------------------------------------------
	// functions
	// ---------------------------------------------------------------------
	void setup_input_layout(GLInputLayout* layout, const VertexDescriptor& descriptor, GLShader* shader)
	{
		// invalid descriptor!
		assert(descriptor.total_attributes > 0);
		
		// descriptor doesn't match shader!
		assert(descriptor.total_attributes == shader->attributes.size());
		
		// allocate enough layout items to hold all the descriptors
		layout->items.allocate(descriptor.total_attributes);
		
		size_t current_offset = 0;
		
		// We need to re-map the source data to the data reported by the
		// driver. The attributes or uniforms may be re-arranged by the driver.
		for (size_t index = 0; index < descriptor.total_attributes; ++index)
		{
			const VertexDescriptor::InputDescription& input = descriptor[index];
			LOGV("input [%i], name = '%s', type = %i, count = %i\n",
				 index,
				 input.name(),
				 input.type,
				 input.element_count
				 );
			
			const VertexDataTypeToGL& gldata = get_vertexdata_table()[input.type];
			
			GLInputLayout::Description target;
			target.location = shader->get_attribute_location(input.name());
			target.type = gldata.type;
			target.normalized = gldata.normalized;
			target.element_count = input.element_count;
			target.size = gldata.element_size * input.element_count;
			target.offset = current_offset;
			
			layout->items[target.location] = target;
			
			// increment the offset pointer
			current_offset += target.size;
		}
		
		layout->vertex_stride = current_offset;
	}

	void setup_pipeline(GLPipeline* pipeline, const PipelineDescriptor& descriptor)
	{
		pipeline->vertex_description = descriptor.vertex_description;
		pipeline->input_layout = static_cast<GLInputLayout*>(descriptor.input_layout);
		assert(pipeline->input_layout != nullptr);
		
		pipeline->enable_blending = descriptor.enable_blending;
		pipeline->blend_source = convert_blendstate(descriptor.blend_source);
		pipeline->blend_destination = convert_blendstate(descriptor.blend_destination);
	}

	void populate_vertexdata_table()
	{
		// populate mapping table
		VERTEXDATA_TO_GL(VD_FLOAT, GL_FLOAT, GL_FALSE, sizeof(float));
		VERTEXDATA_TO_GL(VD_INT, GL_INT, GL_TRUE, sizeof(int));
		VERTEXDATA_TO_GL(VD_UNSIGNED_INT, GL_UNSIGNED_INT, GL_TRUE, sizeof(unsigned int));
		VERTEXDATA_TO_GL(VD_UNSIGNED_BYTE, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(unsigned char));
	}
	
	VertexDataTypeToGL* get_vertexdata_table()
	{
		return _vertex_data_to_gl;
	}
	
	int load_gl_symbols()
	{
		int result = gemgl_startup(gl);
		if (result != 0)
		{
			PLATFORM_LOG(platform::LogMessageType::Error, "load of gl symbols failed!\n");
			return 1;
		}
		
		// parse the GL_VERSION string and determine which renderer to use.
		gemgl_config config;
		gemgl_parse_version(config.major_version, config.minor_version);
		
		gemgl_load_symbols(gl);
		
		return 0;
	}


	// ---------------------------------------------------------------------
	// common functions shared between devices
	// ---------------------------------------------------------------------

	GLenum convert_blendstate(BlendOp op)
	{
		// for now, if we use another blend state; hard crash!
		GLenum blend_table[] = {
			GL_ZERO,
			GL_ONE,
			
			GL_SRC_ALPHA,
			GL_ONE_MINUS_SRC_ALPHA,
			
//			GL_SRC_COLOR,
//			GL_ONE_MINUS_SRC_COLOR,
//			GL_DST_COLOR,
//			GL_ONE_MINUS_DST_COLOR,
//			GL_SRC_ALPHA,
//			GL_ONE_MINUS_SRC_ALPHA,
//			GL_ONE_MINUS_DST_ALPHA,
//			GL_CONSTANT_COLOR,
//			GL_ONE_MINUS_CONSTANT_COLOR,
//			GL_CONSTANT_ALPHA,
//			GL_ONE_MINUS_CONSTANT_ALPHA,
//			GL_SRC_ALPHA_SATURATE,
			
#if 0 // OpenGL 4.x +
			GL_SRC1_COLOR,
			GL_ONE_MINUS_SRC1_COLOR,
			GL_SRC1_ALPHA,
			GL_ONE_MINUS_SRC1_ALPHA
#endif
		};
		
		return blend_table[static_cast<size_t>(op)];
	}

	void common_queue_buffers(CommandQueue* queue_list, size_t total_queues, Array<CommandQueue*>& queued_buffers)
	{
		for (size_t index = 0; index < total_queues; ++index)
		{
			CommandQueue* q = &queue_list[index];
			queued_buffers.push_back(q);
		}
	}

	void common_resize_backbuffer(int width, int height, RenderTarget* target)
	{
		target->width = width;
		target->height = height;
	}

	CommandQueue* common_create_queue(const Pass& render_pass, CommandQueue* next_queue)
	{
		assert(next_queue != nullptr);
		next_queue->pass = render_pass;
		return next_queue;
	}

	void common_pass_setup(const Pass* pass)
	{
		assert(pass->target->width > 0 && pass->target->height > 0);
		gl.Viewport(0, 0, pass->target->width, pass->target->height);

		GLuint clear_flags = 0;

		if (pass->clear_color)
		{
			clear_flags |= GL_COLOR_BUFFER_BIT;
			gl.ClearColor(pass->target_color[0], pass->target_color[1], pass->target_color[2], pass->target_color[3]);
			gl.CheckError("ClearColor");
		}

#if PLATFORM_OPENGL_SUPPORT
		if (pass->clear_depth)
		{
			clear_flags |= GL_DEPTH_BUFFER_BIT;
			gl.ClearDepth(1.0f);
			gl.CheckError("ClearDepth");
		}
#else
#warning gl.ClearDepth needs support on ths platform
#endif

		if (pass->clear_stencil)
		{
			clear_flags |= GL_STENCIL_BUFFER_BIT;
			gl.ClearStencil(0.0f);
			gl.CheckError("ClearStencil");
		}

		if (clear_flags != 0)
		{
			gl.Clear(clear_flags);
			gl.CheckError("Clear");
		}
	}
} // namespace render2
