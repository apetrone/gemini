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

#include "guirenderer.h"

using core::Color;

using namespace gemini;

void GUIRenderer::render_buffer(::renderer::VertexStream& stream, assets::Shader* shader, assets::Material* material)
{
	stream.update();
	
	glm::mat4 modelview;
	glm::mat4 projection = glm::ortho( 0.0f, (float)compositor->width, (float)compositor->height, 0.0f, -0.1f, 256.0f );
	
	::renderer::RenderStream rs;
	rs.add_shader( shader->program );
	rs.add_uniform_matrix4( shader->program->get_uniform_location("modelview_matrix"), &modelview );
	rs.add_uniform_matrix4( shader->program->get_uniform_location("projection_matrix"), &projection );
	
	rs.add_material( material, shader->program );
	
	rs.add_draw_call( stream.vertexbuffer );
	
	rs.run_commands();
	stream.reset();
}


GUIRenderer::GUIRenderer() :
compositor(0),
shader(0),
solid_color(0),
texture_map(0),
vertex_attribs(0),
current_depth(0.0f)
{
}

GUIRenderer::~GUIRenderer()
{
}


void GUIRenderer::increment_depth()
{
	current_depth += 1.0f;
}

void GUIRenderer::startup(gui::Compositor* c)
{
	this->compositor = c;
	stream.desc.add(::renderer::VD_FLOAT3);
	stream.desc.add(::renderer::VD_UNSIGNED_BYTE4);
	stream.desc.add(::renderer::VD_FLOAT2);
	stream.create(64, 64, ::renderer::DRAW_INDEXED_TRIANGLES);
	
	lines.desc.add(::renderer::VD_FLOAT3);
	lines.desc.add(::renderer::VD_UNSIGNED_BYTE4);
	lines.desc.add(::renderer::VD_FLOAT2);

	lines.create(128, 0, ::renderer::DRAW_LINES);
	
	// load shader
	shader = assets::shaders()->load_from_path("shaders/gui");
	
	// setup materials
	solid_color = assets::materials()->allocate_asset();
	if (solid_color)
	{
		::renderer::MaterialParameter parameter;
		parameter.type = ::renderer::MP_VEC4;
		parameter.name = "diffusecolor";
		parameter.vector_value = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		solid_color->add_parameter(parameter);
		
		::renderer::MaterialParameter enable_sampler;
		enable_sampler.type = ::renderer::MP_INT;
		enable_sampler.name = "enable_sampler";
		enable_sampler.int_value = 0;
		solid_color->add_parameter(enable_sampler);
		
		assets::materials()->take_ownership("gui/solid_color", solid_color);
	}
	
	texture_map = assets::materials()->allocate_asset();
	if (texture_map)
	{
		::renderer::MaterialParameter parameter;
		parameter.type = ::renderer::MP_SAMPLER_2D;
		parameter.name = "diffusemap";
		parameter.int_value = assets::textures()->get_default()->Id();
		texture_map->add_parameter(parameter);
		
		::renderer::MaterialParameter enable_sampler;
		enable_sampler.type = ::renderer::MP_INT;
		enable_sampler.name = "enable_sampler";
		enable_sampler.int_value = 1;
		texture_map->add_parameter(enable_sampler);
		
		assets::materials()->take_ownership("gui/texture_map", texture_map);
	}
}

void GUIRenderer::shutdown(gui::Compositor* c)
{
}

void GUIRenderer::begin_frame(gui::Compositor* c)
{
	current_depth = 0.0f;
	::renderer::RenderStream rs;
	
	rs.add_state(::renderer::STATE_BLEND, 1 );
	rs.add_blendfunc(::renderer::BLEND_SRC_ALPHA, ::renderer::BLEND_ONE_MINUS_SRC_ALPHA );
	rs.add_state(::renderer::STATE_DEPTH_TEST, 0);
	rs.add_state(::renderer::STATE_DEPTH_WRITE, 0);
	
	rs.run_commands();
}

void GUIRenderer::end_frame()
{
	::renderer::RenderStream rs;
	
	rs.add_state(::renderer::STATE_BLEND, 0 );
	rs.add_state(::renderer::STATE_DEPTH_TEST, 1);
	rs.add_state(::renderer::STATE_DEPTH_WRITE, 1);
	rs.run_commands();
}


void GUIRenderer::draw_bounds(const gui::Bounds& bounds, const gui::Color& color)
{
	//		gui::Size size = bounds.size;
	//		glm::vec3 start = glm::vec3( bounds.origin.x, bounds.origin.y, 0.0f );
	//		glm::vec3 end = start + glm::vec3( size.width, size.height, 0.0f );
	//		debugdraw::line( start, end, Color( 255, 0, 255 ) );
	//		debugdraw::point( glm::vec3( bounds.origin.x + size.width, bounds.origin.y + size.height, 0.0f ), Color(255, 255, 255) );
	
	float div = 1.0f/255.0f;
	solid_color->parameters[0].vector_value = glm::vec4( (color.r() * div), (color.g() * div), (color.b() * div), (color.a() * div) );
	//		debugdraw::box( start, end, Color(rgba[0], rgba[1], rgba[2], rgba[3]), 0.0f );
	
	stream.reset();
	
	::renderer::RenderStream rs;
	
	if ( stream.has_room(4, 6) )
	{
		VertexType* v = (VertexType*)stream.request(4);
		
		gui::Size size = bounds.size;
		v[0].position = glm::vec3( bounds.origin.x, bounds.origin.y, 0.0f );
		v[1].position = v[0].position + glm::vec3( 0.0f, size.height, 0.0f );
		v[2].position = v[0].position + glm::vec3( size.width, size.height, 0.0f );
		v[3].position = v[0].position + glm::vec3( size.width, 0.0f, 0.0f );
		
		// lower left corner is the origin in OpenGL
		v[0].uv = glm::vec2(0, 0);
		v[1].uv = glm::vec2(0, 1);
		v[2].uv = glm::vec2(1, 1);
		v[3].uv = glm::vec2(1, 0);
		
		//v[0].color = v[1].color = v[2].color = v[3].color = Color(rgba[0], rgba[1], rgba[2], rgba[3]);
		
		::renderer::IndexType indices[] = { 0, 1, 2, 2, 3, 0 };
		stream.append_indices( indices, 6 );
	}
	else
	{
		LOGV( "buffer be full\n" );
	}
	rs.add_state(::renderer::STATE_BLEND, 1 );
	rs.add_blendfunc(::renderer::BLEND_SRC_ALPHA, ::renderer::BLEND_ONE_MINUS_SRC_ALPHA );
	rs.add_state(::renderer::STATE_DEPTH_TEST, 0);
	rs.add_state(::renderer::STATE_DEPTH_WRITE, 0);
	
	rs.run_commands();
	this->render_buffer(stream, shader, solid_color);
}

void GUIRenderer::draw_textured_bounds(const gui::Bounds& bounds, const gui::TextureHandle& handle)
{
	stream.reset();
	::renderer::RenderStream rs;
	assets::Texture * tex = assets::textures()->find_with_id( handle );
	if ( !tex )
	{
		return;
	}
	
	texture_map->parameters[0].int_value = handle;
	texture_map->parameters[0].texture_unit = 0;
	
	if (stream.has_room(4, 6))
	{
		VertexType* v = (VertexType*)stream.request(4);
		
		gui::Size size = bounds.size;
		v[0].position = glm::vec3( bounds.origin.x, bounds.origin.y, 0.0f );
		v[1].position = v[0].position + glm::vec3( 0.0f, size.height, 0.0f );
		v[2].position = v[0].position + glm::vec3( size.width, size.height, 0.0f );
		v[3].position = v[0].position + glm::vec3( size.width, 0.0f, 0.0f );
		
		// lower left corner is the origin in OpenGL
		v[0].uv = glm::vec2(0, 0);
		v[1].uv = glm::vec2(0, 1);
		v[2].uv = glm::vec2(1, 1);
		v[3].uv = glm::vec2(1, 0);
		
		v[0].color = v[1].color = v[2].color = v[3].color = Color(255, 255, 255, 255);
		
		::renderer::IndexType indices[] = { 0, 1, 2, 2, 3, 0 };
		stream.append_indices( indices, 6 );
	}
	
	this->render_buffer(stream, shader, texture_map);
}

void GUIRenderer::draw_line(const gui::Point& start, const gui::Point& end, const gui::Color& color)
{
	lines.reset();
	
	float div = 1.0f/255.0f;
	solid_color->parameters[0].vector_value = glm::vec4( (color.r() * div), (color.g() * div), (color.b() * div), (color.a() * div) );
	
	
	if (lines.has_room(2, 0))
	{
		VertexType* v = (VertexType*)lines.request(2);
		
		v[0].position = glm::vec3(start.x, start.y, 0.0f);
		v[1].position = glm::vec3(end.x, end.y, 0.0f);
		//			v[0].color = v[1].color = Color(255, 255, 255, 255);
	}
	
	this->render_buffer(lines, shader, solid_color);
}

gui::TextureResult GUIRenderer::texture_create(const char* path, gui::TextureHandle& handle)
{
	assets::Texture * tex = assets::textures()->load_from_path((char*)path);
	if ( !tex )
	{
		return gui::TextureResult_Failed;
	}
	
	handle = tex->Id();
	
	return gui::TextureResult_Success;
}

void GUIRenderer::texture_destroy(const gui::TextureHandle& handle)
{
	// nothing really to do in our system
}

gui::TextureResult GUIRenderer::texture_info(const gui::TextureHandle& handle, uint32_t& width, uint32_t& height, uint8_t& channels)
{
	assets::Texture * tex = assets::textures()->find_with_id( handle );
	if ( !tex )
	{
		return gui::TextureResult_Failed;
	}
	
	return gui::TextureResult_Success;
}

gui::FontResult GUIRenderer::font_create(const char* path, gui::FontHandle& handle)
{
	assets::Font* font = assets::fonts()->load_from_path((char*)path);
	if (font == 0)
	{
		return gui::FontResult_Failed;
	}
	
	handle = font->Id();
	
	return gui::FontResult_Success;
}

void GUIRenderer::font_destroy(const gui::FontHandle& handle)
{
	// nothing really to do in our system
}

gui::FontResult GUIRenderer::font_measure_string(const gui::FontHandle& handle, const char* string, gui::Bounds& bounds)
{
	assets::Font* font = assets::fonts()->find_with_id(handle);
	if (font)
	{
		unsigned int width = font::measure_width(font->handle, string);
		unsigned int height = font::measure_height(font->handle, string);
		bounds.set(0, 0, width, height);
		return gui::FontResult_Success;
	}
	
	return gui::FontResult_Failed;
}

void GUIRenderer::font_draw(const gui::FontHandle& handle, const char* string, const gui::Bounds& bounds, const gui::Color& color)
{
	assets::Font* font = assets::fonts()->find_with_id(handle);
	if (font)
	{
		font::draw_string(font->handle, bounds.origin.x, bounds.origin.y, string, Color(color.r(), color.g(), color.b(), color.a()));
	}
}