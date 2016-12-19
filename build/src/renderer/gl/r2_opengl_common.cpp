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
#include <core/logging.h>

using namespace renderer;

// Enable this to perform extra debug checks when setting up input layouts.
#define VERIFY_VERTEX_DATA_MATCHES_SHADER_DATA 1

namespace render2
{
	// ---------------------------------------------------------------------
	// GLInputLayout
	// ---------------------------------------------------------------------


	// ---------------------------------------------------------------------
	// GLShader
	// ---------------------------------------------------------------------
	GLShader::GLShader(gemini::Allocator& allocator)
		: uniforms(allocator)
		, attributes(allocator)
	{
		id = gl.CreateProgram();
		gl.CheckError("CreateProgram");
	}

	GLShader::~GLShader()
	{
		uniforms.clear();
		attributes.clear();
		gl.DeleteProgram(id);
		gl.CheckError("DeleteProgram");
	}


	bool GLShader::compile_shader(GLuint shader, const char* source, const char* preprocessor_defines, const char* version)
	{
		GLint is_compiled = 0;
		const char* shader_source[3] = {
			version,
			preprocessor_defines,
			source
		};

		assert(gl.IsShader(shader));

		gl.ShaderSource(shader, 3, (GLchar**)shader_source, 0);
		gl.CheckError("ShaderSource");

		gl.CompileShader(shader);
		gl.CheckError("CompileShader");

		gl.GetShaderiv(shader, GL_COMPILE_STATUS, &is_compiled);
		gl.CheckError("GetShaderiv");


		if (!is_compiled)
		{
			query_shader_info_log(shader);
		}

		assert(is_compiled);

		return (is_compiled == 1);
	} // compile_shader

	void GLShader::query_program_info_log(GLObject handle)
	{
		int log_length = 0;
		char* logbuffer = 0;

		char buffer[128] = {0};

		gl.GetProgramiv(id, GL_INFO_LOG_LENGTH, &log_length);
		if (log_length == 0)
		{
			log_length = 128;
			logbuffer = buffer;
		}
		else
		{
			logbuffer = (char*)MEMORY_ALLOC(static_cast<size_t>(log_length + 1), core::memory::global_allocator());
		}

		memset(logbuffer, 0, static_cast<size_t>(log_length));

		gl.GetProgramInfoLog(handle, log_length, &log_length, logbuffer);
		gl.CheckError("GetProgramInfoLog");

		if (log_length > 0)
		{
			LOGW("program info log:\n");
			LOGW("%s\n", logbuffer);
		}
		else
		{
			MEMORY_DEALLOC(logbuffer, core::memory::global_allocator());
		}

	} // query_program_info_log


	void GLShader::query_shader_info_log(GLObject handle)
	{
		GLint log_length = 0;
		char* logbuffer = 0;

		// due to log_length returning 0 even when there's data;
		// this will be used as a fallback
		char buffer[128] = {0};

		gl.GetShaderiv(id, GL_INFO_LOG_LENGTH, &log_length);
		if (log_length == 0)
		{
			log_length = 128;
			logbuffer = buffer;
		}
		else
		{
			logbuffer = (char*)MEMORY_ALLOC(static_cast<size_t>(log_length + 1), core::memory::global_allocator());
		}


		memset(logbuffer, 0, static_cast<size_t>(log_length));

		gl.GetShaderInfoLog(handle, log_length, &log_length, logbuffer);
		gl.CheckError("GetShaderInfoLog");

		if (log_length > 0)
		{
			LOGW("shader info log:\n");
			LOGW("%s\n", logbuffer);
		}

		if (logbuffer != buffer)
		{
			MEMORY_DEALLOC(logbuffer, core::memory::global_allocator());
		}
	} // query_shader_info_log

	int GLShader::build_from_source(const char *vertex_shader, const char *fragment_shader, const char* preprocessor, const char* version)
	{
		GLuint vert = gl.CreateShader(GL_VERTEX_SHADER);
		gl.CheckError("CreateShader");
		GLuint frag = gl.CreateShader(GL_FRAGMENT_SHADER);
		gl.CheckError("CreateShader");

		compile_shader(vert, vertex_shader, preprocessor, version);
		compile_shader(frag, fragment_shader, preprocessor, version);

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
			query_program_info_log(id);
		}

		assert(is_linked == 1);


		// activate program
		gl.UseProgram(id);
		gl.CheckError("activate program UseProgram");


		{
			GLint active_attributes = 0;
			gl.GetProgramiv(id, GL_ACTIVE_ATTRIBUTES, &active_attributes);
			gl.CheckError("inspect attributes GetProgramiv");

			attributes.allocate(static_cast<size_t>(active_attributes));

			for (size_t attribute_index = 0; attribute_index < static_cast<size_t>(active_attributes); ++attribute_index)
			{
				shader_variable& attribute = attributes[attribute_index];
				gl.GetActiveAttrib(id,
					static_cast<GLuint>(attribute_index),
					MAX_ATTRIBUTE_NAME_LENGTH,
					&attribute.length,
					&attribute.size,
					&attribute.type,
					attribute.name
				);
				attribute.location = gl.GetAttribLocation(id, attribute.name);
//				LOGV("attribute: %i, location: %i, name: %s, size: %i, type: %i\n",
//					 attribute_index,
//					 attribute.location,
//					 attribute.name,
//					 attribute.size,
//					 attribute.type);

				attribute.compute_size();
			}
		}

		// cache uniform locations
		{
			GLint active_uniforms = 0;
			gl.GetProgramiv(id, GL_ACTIVE_UNIFORMS, &active_uniforms);

			// allocate data for uniforms
			uniforms.allocate(static_cast<size_t>(active_uniforms));

			for (size_t uniform_index = 0; uniform_index < static_cast<size_t>(active_uniforms); ++uniform_index)
			{
				shader_variable& uniform = uniforms[uniform_index];
				gl.GetActiveUniform(id,
					static_cast<GLuint>(uniform_index),
					MAX_ATTRIBUTE_NAME_LENGTH,
					&uniform.length,
					&uniform.size,
					&uniform.type,
					uniform.name
				);
				uniform.location = gl.GetUniformLocation(id, uniform.name);
//				LOGV("uniform: %i, location: %i, name: %s, size: %i, type: %i\n",
//					 uniform_index,
//					 uniform.location,
//					 uniform.name,
//					 uniform.size,
//					 uniform.type);

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
	GLPipeline::GLPipeline(gemini::Allocator& allocator, const PipelineDescriptor& descriptor)
		: Pipeline(allocator)
		, enable_blending(false)
		, blend_source(GL_ONE)
		, blend_destination(GL_ZERO)
	{
		program = (GLShader*)descriptor.shader;

		GLenum primitive_enum[] = {
			GL_LINES,
			GL_TRIANGLES
		};

		draw_type = primitive_enum[static_cast<size_t>(descriptor.primitive_type)];
		assert(draw_type == GL_LINES || draw_type == GL_TRIANGLES);
	}

	GLPipeline::~GLPipeline()
	{
	}


	// ---------------------------------------------------------------------
	// GLTexture
	// ---------------------------------------------------------------------
	GLTexture::GLTexture(const Image& image) :
		texture_id(0),
		texture_type(0),
		unpack_alignment(4),
		width(0),
		height(0)
	{
		// set GL internal texture type based on the image
		texture_type = texture_type_from_image(image);

		glGenTextures(1, &texture_id);
		gl.CheckError("GenTextures");

		if (image.channels == 1 || image.flags & image::F_ALPHA)
		{
			unpack_alignment = 1;
		}
		else
		{
			unpack_alignment = 4;
		}
	}

	GLTexture::~GLTexture()
	{
		glDeleteTextures(1, &texture_id);
		gl.CheckError("DeleteTextures");
	}

	void GLTexture::bind(bool activate)
	{
		GLuint id = activate ? texture_id : 0;
		gl.BindTexture(texture_type, id);
		gl.CheckError("BindTexture");
	}

	void GLTexture::unbind()
	{
		bind(false);
	}

	void GLTexture::set_parameters(const Image& image)
	{
		// set texture wrapping
		GLint wrap_type = GL_REPEAT;

		if (image.flags & image::F_CLAMP)
		{
			wrap_type = GL_CLAMP_TO_EDGE;
		}

		// if we're using GLES2 and the texture isn't a power of two
		// we must use GL_CLAMP_TO_EDGE; otherwise textures will be black.

// not available in GLES2
#if defined(PLATFORM_OPENGL_SUPPORT)
		else if (image.flags & image::F_CLAMP_BORDER)
		{
			wrap_type = GL_CLAMP_TO_BORDER;

			float border[] = {1, 1, 1, 1};
			glTexParameterfv(texture_type, GL_TEXTURE_BORDER_COLOR, border);
		}
#endif

//		gl.TexParameteri(texture_type, GL_TEXTURE_BASE_LEVEL, 0);
//		gl.TexParameteri(texture_type, GL_TEXTURE_MAX_LEVEL, 0);

		gl.TexParameteri(texture_type, GL_TEXTURE_WRAP_S, wrap_type);
		gl.TexParameteri(texture_type, GL_TEXTURE_WRAP_T, wrap_type);




		// set filter type
		GLint min_filter = GL_NEAREST;
		GLint mag_filter = GL_NEAREST;

		if (image.filter == image::FILTER_LINEAR)
		{
			min_filter = GL_LINEAR_MIPMAP_NEAREST;
			mag_filter = GL_NEAREST;
		}
		else if (image.filter == image::FILTER_LINEAR_MIPMAP)
		{
			min_filter = GL_LINEAR_MIPMAP_LINEAR;
			mag_filter = GL_LINEAR;
		}

		// set filtering
		gl.TexParameteri(texture_type, GL_TEXTURE_MIN_FILTER, min_filter);
		gl.TexParameteri(texture_type, GL_TEXTURE_MAG_FILTER, mag_filter);


//		gl.TexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4 );
		width = image.width;
		height = image.height;


	}

	GLRenderTarget::GLRenderTarget(uint32_t _width, uint32_t _height, bool _is_default)
	{
		width = _width;
		height = _height;
		framebuffer = 0;

		if (!_is_default)
		{
			gl.GenFramebuffers(1, &framebuffer);
		}
	}

	GLRenderTarget::~GLRenderTarget()
	{
		if (framebuffer > 0)
		{
			gl.DeleteFramebuffers(1, &framebuffer);
		}
	}

	void GLRenderTarget::bind(bool activate)
	{
		assert(gl.BindFramebuffer);

		GLuint fbo = activate ? framebuffer : 0;
		gl.BindFramebuffer(GL_FRAMEBUFFER, fbo);
	}

	void GLRenderTarget::activate()
	{
		bind(true);
	}

	void GLRenderTarget::deactivate()
	{
		bind(false);
	}

	bool GLRenderTarget::is_complete() const
	{
		// the frame buffer MUST be bound when this is checked
		GLenum status = gl.CheckFramebufferStatus(GL_FRAMEBUFFER);
		gl.CheckError("CheckFramebufferStatus");
		return (status == GL_FRAMEBUFFER_COMPLETE);
	}

	void GLRenderTarget::attach_texture(GLTexture* texture)
	{
		assert(texture != nullptr);

		activate();

		GLuint index = 0;
		gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+index, texture->texture_type, texture->texture_id, 0);
		gl.CheckError("FramebufferTexture2D");

		assert(is_complete());

		deactivate();
	}
	// ---------------------------------------------------------------------
	// implementation
	// ---------------------------------------------------------------------
	VertexDataTypeToGL _vertex_data_to_gl[ VD_TOTAL ];
	#define VERTEXDATA_TO_GL(vdt, gl_type, gl_normalized, element_size) \
		_vertex_data_to_gl[ vdt ] = VertexDataTypeToGL(gl_type, gl_normalized, element_size)


	void convert_type(GLenum attribute_type, VertexDataType& type, size_t& elements)
	{
		switch(attribute_type)
		{
			case GL_FLOAT_VEC2:
				type = VD_FLOAT;
				elements = 2;
				break;

			case GL_FLOAT_VEC3:
				type = VD_FLOAT;
				elements = 3;
				break;

			case GL_FLOAT_VEC4:
				type = VD_FLOAT;
				elements = 4;
				break;

			// unhandled type!
			default:
				LOGE("Unhandled attribute type!: %i\n", attribute_type);
				assert(0);
		}
	}

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
//			LOGV("input [%i], name = '%s', type = %i, count = %i\n",
//				 index,
//				 input.name(),
//				 input.type,
//				 input.element_count
//				 );

			const VertexDataTypeToGL& gldata = get_vertexdata_table()[input.type];

#if defined(VERIFY_VERTEX_DATA_MATCHES_SHADER_DATA)
			// perform verification with types and element counts
			VertexDataType expected_type;
			size_t expected_elements = 0;
			GLint location = shader->get_attribute_location(input.name());
			assert(location >= 0);

			// We need to iterate over the attributes list and find the
			// one corresponding to this location.
			size_t attribute_index = 0;
			for (attribute_index = 0; attribute_index < shader->attributes.size(); ++attribute_index)
			{
				shader_variable& variable = shader->attributes[attribute_index];
				if (variable.location == location)
					break;
			}

			// If you hit this, we haven't found the matching attribute
			assert(index < shader->attributes.size());

			// extract the gl data for attribute types
			convert_type(shader->attributes[attribute_index].type, expected_type, expected_elements);

			// Types should match; otherwise byte offsets will be incorrect.
			// Element counts can vary slightly if the input element size
			// is less than or equal to the shader's type.
			assert(input.type == expected_type && input.element_count <= expected_elements);
#endif

			GLInputLayout::Description target;
			target.location = location;
			target.type = gldata.type;
			target.normalized = static_cast<GLboolean>(gldata.normalized);
			target.element_count = static_cast<GLint>(input.element_count);
			target.size = gldata.element_size * input.element_count;
			target.offset = current_offset;

			layout->items[static_cast<size_t>(target.location)] = target;

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
			LOGE("load of gl symbols failed!\n");
			return 1;
		}

		// parse the GL_VERSION string and determine which renderer to use.
		gemgl_config config;
		gemgl_parse_version(config.major_version, config.minor_version);

		gemgl_load_symbols(gl);

		return 0;
	}

	void unload_gl_symbols()
	{
		gemgl_shutdown(gl);
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

	RenderTarget* common_create_render_target(Texture* texture)
	{
		GLTexture* gltexture = static_cast<GLTexture*>(texture);

		GLRenderTarget* rt = MEMORY_NEW(GLRenderTarget, core::memory::global_allocator())(gltexture->width, gltexture->height);

		rt->attach_texture(gltexture);

		return rt;
	}

	void common_destroy_render_target(RenderTarget* render_target)
	{
		GLRenderTarget* rt = static_cast<GLRenderTarget*>(render_target);
		MEMORY_DELETE(rt, core::memory::global_allocator());
	}

	void common_push_render_target(RenderTarget* render_target)
	{
		GLRenderTarget* rt = static_cast<GLRenderTarget*>(render_target);
		rt->activate();
	}

	void common_pop_render_target(RenderTarget* render_target)
	{
		GLRenderTarget* rt = static_cast<GLRenderTarget*>(render_target);
		rt->deactivate();
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
		target->width = static_cast<uint32_t>(width);
		target->height = static_cast<uint32_t>(height);
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
		gl.Viewport(0, 0, static_cast<GLsizei>(pass->target->width), static_cast<GLsizei>(pass->target->height));

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
#warning gl.ClearDepth needs support on this platform
#endif

		if (pass->clear_stencil)
		{
			clear_flags |= GL_STENCIL_BUFFER_BIT;
			gl.ClearStencil(0);
			gl.CheckError("ClearStencil");
		}

		if (clear_flags != 0)
		{
			gl.Clear(clear_flags);
			gl.CheckError("Clear");
		}

		if (pass->depth_test)
		{
			gl.Enable(GL_DEPTH_TEST);
		}
		else
		{
			gl.Disable(GL_DEPTH_TEST);
		}

		if (pass->cull_mode == CullMode::Frontface)
		{
			gl.Enable(GL_CULL_FACE);
			gl.CullFace(GL_FRONT);
		}
		else if (pass->cull_mode == CullMode::Backface)
		{
			gl.Enable(GL_CULL_FACE);
			gl.CullFace(GL_BACK);
		}
		else
		{
			gl.Disable(GL_CULL_FACE);
		}
	}

	GLShader* common_create_shader(gemini::Allocator& allocator, const char* subfolder, const char* name, GLShader* reuse_shader, const char* preprocessor, const char* version)
	{
		// I haven't re-implemented hot loading of shaders; do that if you
		// hit this assert :)
		assert(reuse_shader == nullptr);

		// the path to shaders resembles this:
		// 'shaders/<GLSL version>/<name>.[vert|frag]'

		core::StackString<64> vertex_shader_path = "shaders";
		vertex_shader_path.append(PATH_SEPARATOR_STRING);
		vertex_shader_path.append(subfolder);
		vertex_shader_path.append(PATH_SEPARATOR_STRING);
		vertex_shader_path.append(name);
		vertex_shader_path.append(".vert");

		Array<unsigned char> vertex_shader_source;

		const ResourceProvider* resource_provider = get_resource_provider();

		resource_provider->load_file(vertex_shader_source, vertex_shader_path());
		assert(!vertex_shader_source.empty());

		core::StackString<64> fragment_shader_path = "shaders";
		fragment_shader_path.append(PATH_SEPARATOR_STRING);
		fragment_shader_path.append(subfolder);
		fragment_shader_path.append(PATH_SEPARATOR_STRING);
		fragment_shader_path.append(name);
		fragment_shader_path.append(".frag");

		LOGV("create shader \"%s\"\n", name);

		Array<unsigned char> fragment_shader_source;
		resource_provider->load_file(fragment_shader_source, fragment_shader_path());
		assert(!fragment_shader_source.empty());

		// Tack on a terminator since we're passing these to GL functions
		// which expect a proper C-string.
		vertex_shader_source.push_back('\0');
		fragment_shader_source.push_back('\0');

		GLShader* shader = MEMORY2_NEW(allocator, GLShader)(allocator);
		shader->build_from_source(
		  (char*)&vertex_shader_source[0],
		  (char*)&fragment_shader_source[0],
		  preprocessor,
		  version
		);

		return shader;
	}



	// ---------------------------------------------------------------------
	// image
	// ---------------------------------------------------------------------
	// utility functions
	GLenum image_to_source_format(const Image& image)
	{
		uint32_t num_channels = image.channels;

		if ( num_channels == 3 )
		{
			return GL_RGB;
		}
		else if ( num_channels == 4 )
		{
			return GL_RGBA;
		}
		else if ((num_channels == 1) || (image.flags & image::F_ALPHA))
		{
		// GL_ALPHA is available pre GL 3.2
		// and in GLES 2.
#if defined(PLATFORM_GLES2_SUPPORT)
			return GL_ALPHA;
#else
			return GL_RED;
#endif
		}

		assert(0);
		return GL_RGBA;
	} // image_source_format

	GLint image_to_internal_format(const Image& image)
	{
		//GLenum internalFormat = GL_SRGB8;
		if (image.channels == 3)
		{
			return GL_RGB;
		}
		else if (image.channels == 4)
		{
			return GL_RGBA;
		}
		else if ((image.channels == 1) || (image.flags & image::F_ALPHA))
		{
			// GL_ALPHA is available pre GL 3.2
			// and in GLES 2.
#if defined(PLATFORM_GLES2_SUPPORT)
			return GL_ALPHA;
#else
			return GL_RED;
#endif
		}

		// Unknown internal storage format
		assert(!"Unknown internal storage format");
		return GL_INVALID_ENUM;
	} // image_to_internal_format

	GLenum texture_type_from_image(const Image& image)
	{
		if (image.type == image::TEX_2D)
		{
			return GL_TEXTURE_2D;
		}
		else if (image.type == image::TEX_CUBE)
		{
			return GL_TEXTURE_CUBE_MAP;
		}
		else
		{
			// if you reach this assert, this class cannot deal with the image
			// type you specified.
			assert(0);
		}

		return GL_INVALID_ENUM;
	} // texture_type_from_image

	GLTexture* common_create_texture(const Image& image)
	{
		GLenum source_format = image_to_source_format(image);
		GLint internal_format = image_to_internal_format(image);

		GLTexture* texture = MEMORY_NEW(GLTexture, core::memory::global_allocator())(image);

#if defined(PLATFORM_GLES2_SUPPORT)
		// GL_INVALID_OPERATION is generated if <source_format> does not match <internal_format>
		assert(source_format == internal_format);
#endif

		texture->bind();
		texture->set_parameters(image);


		GLvoid* pixels = nullptr;
		if (!image.pixels.empty())
		{
			pixels = (GLvoid*)&image.pixels[0];
		}


		// TODO@APP: This needs to be re-visited because there's a state problem
		// when you create a cubemap (which succeeds) and then later try to
		// create a normal 2d texture. (that catches a glerror in bind).
		// This tells me there's a gl error created somewhere after this ordeal.
		// Likely in the render target stuff!
		if (image.type == image::TEX_2D)
		{
			// upload image and generate mipmaps
			gl.TexImage2D(texture->texture_type,
				0,
				internal_format,
				static_cast<GLsizei>(image.width),
				static_cast<GLsizei>(image.height),
				0,
				source_format,
				GL_UNSIGNED_BYTE,
				pixels
			);
			gl.CheckError("teximage2d");
		}
		else if (image.type == image::TEX_CUBE)
		{
			for (uint8_t index = 0; index < 6; ++index)
			{
				gl.TexImage2D(static_cast<GLenum>(GL_TEXTURE_CUBE_MAP_POSITIVE_X+index),
					0,
					internal_format,
					static_cast<GLsizei>(image.width),
					static_cast<GLsizei>(image.height),
					0,
					source_format,
					GL_UNSIGNED_BYTE,
					pixels
				);
				gl.CheckError("teximage2d (cube)");
			}
		}

		// generate mipmaps if the image allows us to
		if (!(image.flags & image::F_NO_MIPMAPS))
		{
			gl.GenerateMipmap(texture->texture_type);
			gl.CheckError("generate mipmap");
		}

		texture->unbind();

		return texture;
	}


	void common_update_texture(GLTexture* texture, const Image& image, const glm::vec2& origin, const glm::vec2& dimensions)
	{
		// In ES 2 implementations; we can use GL_EXT_unpack_subimage for GL_UNPACK_ROW_LENGTH
		// If that isn't available; the new image has to be uploaded one row at a time.

		// origin should be non-zero and positive
		assert(origin.x >= 0);
		assert(origin.y >= 0);

		// dimensions should be positive
		assert(dimensions.x > 0 && dimensions.y > 0);

		GLenum internal_format = static_cast<GLenum>(image_to_internal_format(image));

		texture->bind();

#if 0
		// store old items (should be cached by the hal)
		GLint row_length;
		GLint skip_pixels;
		GLint skip_rows;
		gl.GetIntegerv(GL_UNPACK_ROW_LENGTH, &row_length);
		gl.GetIntegerv(GL_UNPACK_SKIP_PIXELS, &skip_pixels);
		gl.GetIntegerv(GL_UNPACK_SKIP_ROWS, &skip_rows);
#endif

		// per the spec; 1, 2, 4, or 8 are valid values
		assert(texture->unpack_alignment == 1 || texture->unpack_alignment == 2 || texture->unpack_alignment == 4 || texture->unpack_alignment == 8);

		// set alignment for this operation
		gl.PixelStorei(GL_UNPACK_ALIGNMENT, static_cast<GLint>(image.alignment));

#if 0
		gl.PixelStorei(GL_UNPACK_ROW_LENGTH, image.width);
		gl.PixelStorei(GL_UNPACK_SKIP_PIXELS, rect.left);
		gl.PixelStorei(GL_UNPACK_SKIP_ROWS, rect.top);
#endif



		GLvoid* pixels = 0;
		if (!image.pixels.empty())
		{
			pixels = (GLvoid*)&image.pixels[0];
		}

		GLint mip_level = 0;
		gl.TexSubImage2D(texture->texture_type,
			mip_level,
			static_cast<GLint>(origin.x),
			static_cast<GLint>(origin.y),
			static_cast<GLsizei>(dimensions.x),
			static_cast<GLsizei>(dimensions.y),
			internal_format,
			GL_UNSIGNED_BYTE,
			pixels
		);
		gl.CheckError("TexSubImage2D");

		texture->unbind();

#if 0
		// restore these parameters
		gl.PixelStorei(GL_UNPACK_ROW_LENGTH, row_length);
		gl.PixelStorei(GL_UNPACK_SKIP_PIXELS, skip_pixels);
		gl.PixelStorei(GL_UNPACK_SKIP_ROWS, skip_rows);
#endif
	}

	void common_destroy_texture(Texture* tex)
	{
		GLTexture* texture = static_cast<GLTexture*>(tex);
		MEMORY_DELETE(texture, core::memory::global_allocator());
	}


	void common_setup_uniforms(GLShader* shader, ConstantBuffer& constants)
	{
		size_t offset = 0;

		// TODO: dispatch of various uniform types
		for(size_t index = 0; index < shader->uniforms.size(); ++index)
		{
			shader_variable& uniform = shader->uniforms[index];
			const void* data = constants.get(uniform.name);

			if (!data)
			{
				LOGW("Missing uniform data for '%s'\n", uniform.name);
				// If you hit this assert the uniform named above wasn't given
				// a value prior to rendering.
				assert(0);
			}

			switch(uniform.type)
			{
				case GL_INT:
				case GL_UNSIGNED_INT:
				{
					const GLint* value = static_cast<const GLint*>(data);
					gl.Uniform1i(uniform.location, (*value));
					gl.CheckError("Uniform1i");
					break;
				}

				case GL_FLOAT_MAT4:
					gl.UniformMatrix4fv(uniform.location, uniform.size, GL_FALSE, (GLfloat*)data);
					gl.CheckError("UniformMatrix4fv");
					break;
				case GL_SAMPLER_2D:
				{
					const GLint* sampler = static_cast<const GLint*>(data);

					// maximum of eight texture units
					assert(*sampler < 8);

					gl.Uniform1i(uniform.location, (*sampler));
					gl.CheckError("Uniform1i");
					break;
				}

				default:
					// If you hit this assert, there's a discrepancy between this function
					// and the 'type_to_bytes' function -- where by an unhandled
					// uniform type was hit to cause this assert.
					assert(0);
				break;
			}
			offset += uniform.byte_size;
		}
	}
} // namespace render2
