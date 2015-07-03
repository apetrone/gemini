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


#include <renderer/renderer.h>

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
texture_map(0),
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
	// generate the white texture we'll use for solid colors
	white_texture = assets::textures()->allocate_asset();
	white_texture->image.create(4, 4, 3);
	white_texture->image.fill(Color(255, 255, 255));
//	image::generate_checker_pattern(white_texture->image, Color(255, 0, 0), Color(0, 255, 0));
	white_texture->image.flags = image::F_WRAP | image::F_RGB | image::F_CLAMP_BORDER;
	white_texture->texture = ::renderer::driver()->texture_create(white_texture->image);
	assets::textures()->take_ownership("gui/white_texture", white_texture);


	

	this->compositor = c;
	stream.desc.add(::renderer::VD_FLOAT3);
	stream.desc.add(::renderer::VD_UNSIGNED_BYTE4);
	stream.desc.add(::renderer::VD_FLOAT2);
	stream.create(64, 64, ::renderer::DRAW_TRIANGLES);
	
	// load shader
	shader = assets::shaders()->load_from_path("shaders/gui");
	
	texture_map = assets::materials()->allocate_asset();
	if (texture_map)
	{
		::renderer::MaterialParameter parameter;
		parameter.type = ::renderer::MP_SAMPLER_2D;
		parameter.name = "diffusemap";
		parameter.texture_unit = assets::texture_unit_for_map("diffusemap");
		parameter.texture = white_texture->texture;
//		parameter.texture = assets::textures()->get_default()->texture;
		
		
		texture_map->add_parameter(parameter);
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
//	//		gui::Size size = bounds.size;
//	//		glm::vec3 start = glm::vec3( bounds.origin.x, bounds.origin.y, 0.0f );
//	//		glm::vec3 end = start + glm::vec3( size.width, size.height, 0.0f );
//	//		debugdraw::line( start, end, Color( 255, 0, 255 ) );
//	//		debugdraw::point( glm::vec3( bounds.origin.x + size.width, bounds.origin.y + size.height, 0.0f ), Color(255, 255, 255) );
//	
//	float div = 1.0f/255.0f;
//	solid_color->parameters[0].vector_value = glm::vec4( (color.r() * div), (color.g() * div), (color.b() * div), (color.a() * div) );
//	//		debugdraw::box( start, end, Color(rgba[0], rgba[1], rgba[2], rgba[3]), 0.0f );
//	
//	stream.reset();
//	
//	::renderer::RenderStream rs;
//	
//	if ( stream.has_room(4, 6) )
//	{
//		VertexType* v = (VertexType*)stream.request(4);
//		
//		gui::Size size = bounds.size;
//		v[0].position = glm::vec3( bounds.origin.x, bounds.origin.y, 0.0f );
//		v[1].position = v[0].position + glm::vec3( 0.0f, size.height, 0.0f );
//		v[2].position = v[0].position + glm::vec3( size.width, size.height, 0.0f );
//		v[3].position = v[0].position + glm::vec3( size.width, 0.0f, 0.0f );
//		
//		// lower left corner is the origin in OpenGL
//		v[0].uv = glm::vec2(0, 0);
//		v[1].uv = glm::vec2(0, 1);
//		v[2].uv = glm::vec2(1, 1);
//		v[3].uv = glm::vec2(1, 0);
//		
//		//v[0].color = v[1].color = v[2].color = v[3].color = Color(rgba[0], rgba[1], rgba[2], rgba[3]);
//		
//		::renderer::IndexType indices[] = { 0, 1, 2, 2, 3, 0 };
//		stream.append_indices( indices, 6 );
//	}
//	else
//	{
//		LOGV( "buffer be full\n" );
//	}
//	rs.add_state(::renderer::STATE_BLEND, 1 );
//	rs.add_blendfunc(::renderer::BLEND_SRC_ALPHA, ::renderer::BLEND_ONE_MINUS_SRC_ALPHA );
//	rs.add_state(::renderer::STATE_DEPTH_TEST, 0);
//	rs.add_state(::renderer::STATE_DEPTH_WRITE, 0);
//	
//	rs.run_commands();
//	this->render_buffer(stream, shader, solid_color);
}

void GUIRenderer::draw_textured_bounds(const gui::Bounds& bounds, const gui::TextureHandle& handle)
{
//	stream.reset();
//	::renderer::RenderStream rs;
//	assets::Texture * tex = assets::textures()->find_with_id( handle );
//	if ( !tex )
//	{
//		return;
//	}
//	
//	texture_map->parameters[0].int_value = handle;
//	texture_map->parameters[0].texture_unit = 0;
//	
//	if (stream.has_room(4, 6))
//	{
//		VertexType* v = (VertexType*)stream.request(4);
//		
//		gui::Size size = bounds.size;
//		v[0].position = glm::vec3( bounds.origin.x, bounds.origin.y, 0.0f );
//		v[1].position = v[0].position + glm::vec3( 0.0f, size.height, 0.0f );
//		v[2].position = v[0].position + glm::vec3( size.width, size.height, 0.0f );
//		v[3].position = v[0].position + glm::vec3( size.width, 0.0f, 0.0f );
//		
//		// lower left corner is the origin in OpenGL
//		v[0].uv = glm::vec2(0, 0);
//		v[1].uv = glm::vec2(0, 1);
//		v[2].uv = glm::vec2(1, 1);
//		v[3].uv = glm::vec2(1, 0);
//		
//		v[0].color = v[1].color = v[2].color = v[3].color = Color(255, 255, 255, 255);
//		
//		::renderer::IndexType indices[] = { 0, 1, 2, 2, 3, 0 };
//		stream.append_indices( indices, 6 );
//	}
//	
//	this->render_buffer(stream, shader, texture_map);
}

void GUIRenderer::draw_line(const gui::Point& start, const gui::Point& end, const gui::Color& color)
{
//	lines.reset();
//	
//	float div = 1.0f/255.0f;
//	solid_color->parameters[0].vector_value = glm::vec4( (color.r() * div), (color.g() * div), (color.b() * div), (color.a() * div) );
//	
//	
//	if (lines.has_room(2, 0))
//	{
//		VertexType* v = (VertexType*)lines.request(2);
//		
//		v[0].position = glm::vec3(start.x, start.y, 0.0f);
//		v[1].position = glm::vec3(end.x, end.y, 0.0f);
//		//			v[0].color = v[1].color = Color(255, 255, 255, 255);
//	}
//	
//	this->render_buffer(lines, shader, solid_color);
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

gui::FontResult GUIRenderer::font_fetch_texture(const gui::FontHandle &handle, gui::TextureHandle &texture)
{
	return gui::FontResult_Failed;
}

void GUIRenderer::draw_command_lists(gui::render::CommandList** command_lists, size_t total_lists)
{
	size_t total_vertices = 0;
	for (size_t index = 0; index < total_lists; index++)
	{
		total_vertices += command_lists[index]->vertex_buffer.size();
	}
	
	

//	float div = 1.0f/255.0f;
//	solid_color->parameters[0].vector_value = glm::vec4( (color.r() * div), (color.g() * div), (color.b() * div), (color.a() * div) );

	stream.reset();
	
	
//	if (stream.has_room(total_vertices, 0))
	{
//		LOGV("command_queues: %i, total_vertices: %i\n", total_lists, total_vertices);
#if 0
		VertexType* vertex = (VertexType*)stream.request(6);
		{
			vertex[0].position.x = 0;
			vertex[0].position.y = 100;
			vertex[0].color = core::Color(255, 0, 0);
			vertex[0].uv = glm::vec2(0, 0);

			vertex[1].position.x = 100;
			vertex[1].position.y = 100;
			vertex[1].color = core::Color(0, 255, 0);
			vertex[1].uv = glm::vec2(1, 0);
			
			vertex[2].position.x = 100;
			vertex[2].position.y = 0;
			vertex[2].color = core::Color(0, 0, 255);
			vertex[2].uv = glm::vec2(1, 1);
			
			vertex[3].position.x = 100;
			vertex[3].position.y = 0;
			vertex[3].color = core::Color(0, 0, 255);
			vertex[3].uv = glm::vec2(1, 1);
			
			vertex[4].position.x = 0;
			vertex[4].position.y = 0;
			vertex[4].color = core::Color(255, 255, 255);
			vertex[4].uv = glm::vec2(0, 1);

			vertex[5].position.x = 0;
			vertex[5].position.y = 100;
			vertex[5].color = core::Color(255, 0, 0);
			vertex[5].uv = glm::vec2(0, 0);
			
			stream.update();
			glm::mat4 modelview;
			glm::mat4 projection = glm::ortho( 0.0f, (float)compositor->width, (float)compositor->height, 0.0f, -0.1f, 256.0f );
			
			::renderer::RenderStream rs;
			rs.add_shader( shader->program );
			rs.add_uniform_matrix4( shader->program->get_uniform_location("modelview_matrix"), &modelview );
			rs.add_uniform_matrix4( shader->program->get_uniform_location("projection_matrix"), &projection );
			
			rs.add_material(texture_map, shader->program);
			
			rs.add_draw_call(stream.vertexbuffer, 0, 3);
			
			rs.run_commands();
		}
#else
		VertexType* vertex = (VertexType*)stream.request(total_vertices);
		for (size_t index = 0; index < total_lists; ++index)
		{
			gui::render::CommandList* commandlist = command_lists[index];
			
//			for (size_t command_id = 0; command_id < commandlist->commands.size(); ++command_id)
//			{
//				LOGV("command: %i\n", commandlist->commands[command_id].id);
//			}
			
			for (size_t v = 0; v < commandlist->vertex_buffer.size(); ++v)
			{
				gui::render::Vertex* gv = &commandlist->vertex_buffer[v];
				vertex[v].position.x = gv->x;
				vertex[v].position.y = gv->y;
				core::Color c = core::Color(gv->color.r(), gv->color.g(), gv->color.b(), gv->color.a());
				vertex[v].color = c;
				
				vertex[v].uv.x = gv->uv[0];
				vertex[v].uv.y = gv->uv[1];
			}
			


			
			//offset += commandlist->vertex_buffer.size();
		}
		
		stream.update();
		if (stream.last_vertex > 0)
		{
			
			
			glm::mat4 modelview;
			glm::mat4 projection = glm::ortho( 0.0f, (float)compositor->width, (float)compositor->height, 0.0f, -0.1f, 256.0f );
			
			::renderer::RenderStream rs;
			rs.add_shader( shader->program );
			rs.add_uniform_matrix4( shader->program->get_uniform_location("modelview_matrix"), &modelview );
			rs.add_uniform_matrix4( shader->program->get_uniform_location("projection_matrix"), &projection );
			
			rs.add_material(texture_map, shader->program);
			
			rs.add_draw_call(stream.vertexbuffer);
			
			rs.run_commands();
			stream.reset();
		}
#endif
//		::renderer::RenderStream rs;
//		rs.add_state(::renderer::STATE_BLEND, 1 );
//		rs.add_blendfunc(::renderer::BLEND_SRC_ALPHA, ::renderer::BLEND_ONE_MINUS_SRC_ALPHA );
//		rs.add_state(::renderer::STATE_DEPTH_TEST, 0);
//		rs.add_state(::renderer::STATE_DEPTH_WRITE, 0);
//
//		rs.run_commands();
//		this->render_buffer(stream, shader, solid_color);
	}
//	assets::Texture * tex = assets::textures()->find_with_id( handle );
//	if ( !tex )
//	{
//		return;
//	}
//	
//	texture_map->parameters[0].int_value = handle;
//	texture_map->parameters[0].texture_unit = 0;
	
//	if (stream.has_room(4, 6))
//	{
//		VertexType* v = (VertexType*)stream.request(4);
//		
//		gui::Size size = bounds.size;
//		v[0].position = glm::vec3( bounds.origin.x, bounds.origin.y, 0.0f );
//		v[1].position = v[0].position + glm::vec3( 0.0f, size.height, 0.0f );
//		v[2].position = v[0].position + glm::vec3( size.width, size.height, 0.0f );
//		v[3].position = v[0].position + glm::vec3( size.width, 0.0f, 0.0f );
//		
//		// lower left corner is the origin in OpenGL
//		v[0].uv = glm::vec2(0, 0);
//		v[1].uv = glm::vec2(0, 1);
//		v[2].uv = glm::vec2(1, 1);
//		v[3].uv = glm::vec2(1, 0);
//		
//		v[0].color = v[1].color = v[2].color = v[3].color = Color(255, 255, 255, 255);
//		
//		::renderer::IndexType indices[] = { 0, 1, 2, 2, 3, 0 };
//		stream.append_indices( indices, 6 );
//	}
//	
//	this->render_buffer(stream, shader, texture_map);
}