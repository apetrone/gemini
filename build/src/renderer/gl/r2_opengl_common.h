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

#include "gemgl.h"

#include <core/fixedarray.h>
#include <core/typedefs.h>

namespace render2
{
	const size_t RENDERER_MAX_COMMAND_QUEUES = 256;

	static size_t type_to_bytes(const GLenum& type)
	{
		switch(type)
		{
			case GL_INT: return sizeof(int);
			case GL_UNSIGNED_INT: return sizeof(unsigned int);

			case GL_FLOAT_VEC2: return sizeof(GLfloat) * 2;
			case GL_FLOAT_VEC3: return sizeof(GLfloat) * 3;
			case GL_FLOAT_VEC4: return sizeof(GLfloat) * 4;

			case GL_FLOAT_MAT4: return sizeof(GLfloat) * 16;
			case GL_SAMPLER_2D: return sizeof(GLuint);

			default: break;
		}

		// If you reach this, you're missing an OpenGL type from the above switch
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
			byte_size = static_cast<GLint>(size * type_to_bytes(type));
		}
	};

	class GLInputLayout : public InputLayout
	{
	public:
		struct Description
		{
			GLint location;
			GLenum type;
			GLint element_count;
			GLboolean normalized;
			size_t offset;
			size_t size;
		};

		size_t vertex_stride;
		FixedArray<Description> items;

		GLInputLayout(gemini::Allocator& allocator)
			: items(allocator)
		{
		}
	}; // GLInputLayout

	struct GLShader : public Shader
	{
		GLShader(gemini::Allocator& allocator);
		virtual ~GLShader();

		bool compile_shader(gemini::Allocator& allocator, GLuint shader, const char* source, const char* preprocessor_defines, const char* version);
		void query_program_info_log(gemini::Allocator& allocator, renderer::GLObject handle);
		void query_shader_info_log(gemini::Allocator& allocator, renderer::GLObject handle);

		int build_from_source(gemini::Allocator& allocator, const char *vertex_shader, const char *fragment_shader, const char* preprocessor, const char* version);
		GLint get_attribute_location(const char* name);
		GLint get_uniform_location(const char* name);

		GLuint id;
		FixedArray<shader_variable> uniforms;
		FixedArray<shader_variable> attributes;
	}; // GLShader

	struct GLPipeline : public Pipeline
	{
		GLShader* program;
		VertexDescriptor vertex_description;
		GLInputLayout* input_layout;

		bool enable_blending;
		GLenum blend_source;
		GLenum blend_destination;
		GLenum draw_type;

		GLPipeline(gemini::Allocator& allocator, const PipelineDescriptor& descriptor);
		virtual ~GLPipeline();
	}; // GLPipeline

	struct GLTexture : public Texture
	{
		GLTexture(const Image& image);
		virtual ~GLTexture();

		void bind(bool activate = true);
		void unbind();

		void set_parameters(const Image& image);

		GLuint texture_id;
		GLenum texture_type;
		uint8_t unpack_alignment;
		GLenum internal_format;

		// texture dimensions
		uint32_t width;
		uint32_t height;
	}; // GLTexture

	struct GLRenderTarget : public RenderTarget
	{
		GLRenderTarget(uint32_t _width, uint32_t _height, bool _is_default = false);
		virtual ~GLRenderTarget();

		void bind(bool activate = true);
		void activate();
		void deactivate();
		bool is_complete() const;
		void attach_texture(GLTexture* texture);
		void resize(uint32_t width, uint32_t height);

	private:
		GLuint framebuffer;
//		GLuint renderbuffer;

		GLTexture* attached_texture;
	};	// GLRenderTarget

	struct VertexDataTypeToGL
	{
		GLenum type;
		GLenum normalized;
		uint32_t element_size;

		VertexDataTypeToGL(GLenum _type = GL_INVALID_ENUM, GLenum _normalized = GL_INVALID_ENUM, uint32_t _element_size = 0) :
			type(_type),
			normalized(_normalized),
			element_size(_element_size)
		{
		}
	};

	void setup_input_layout(GLInputLayout* layout, const VertexDescriptor& descriptor, GLShader* shader);
	void setup_pipeline(GLPipeline* pipeline, const PipelineDescriptor& descriptor);
	void populate_vertexdata_table();
	VertexDataTypeToGL* get_vertexdata_table();

	// returns 0 on success
	int load_gl_symbols();
	void unload_gl_symbols();

	GLenum convert_blendstate(BlendOp op);

	RenderTarget* common_create_render_target(gemini::Allocator& allocator, Texture* texture);
	void common_destroy_render_target(gemini::Allocator& allocator, RenderTarget* render_target);
	void common_resize_render_target(RenderTarget* target, uint32_t width, uint32_t height);
	void common_render_target_read_pixels(RenderTarget* target, Image& image);

	void common_push_render_target(RenderTarget* render_target);
	void common_pop_render_target(RenderTarget* render_target);

	void common_queue_buffers(CommandQueue* queue_list, size_t total_queues, Array<CommandQueue*>& queued_buffers);
	void common_resize_backbuffer(int width, int height, RenderTarget* target);
	CommandQueue* common_create_queue(const Pass& render_pass, CommandQueue* next_queue);
	void common_pass_setup(const Pass* pass);

	GLShader* common_create_shader(gemini::Allocator& allocator, const char* subfolder, const char* name, GLShader* reuse_shader, const char* preprocessor, const char* version);

	// for use with glTexImage
	GLenum image_to_source_format(const Image& image);
	GLint image_to_internal_format(const Image& image);
	GLenum texture_type_from_image(const Image& image);
	GLTexture* common_create_texture(gemini::Allocator& allocator, const Image& image);
	void common_update_texture(GLTexture* texture, const Image& image, const glm::vec2& origin, const glm::vec2& dimensions);
	void common_destroy_texture(gemini::Allocator& allocator, Texture* texture);

	void common_setup_uniforms(GLShader* shader, ConstantBuffer& constants);
} // namespace render2
