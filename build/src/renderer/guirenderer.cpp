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

#include <runtime/filesystem.h>

#include <renderer/renderer.h>
#include <renderer/font.h>


struct GUIVertex
{
	glm::vec2 position;
	float color[4];
	glm::vec2 uv;

	void set_position(float x, float y)
	{
		position.x = x;
		position.y = y;
	}

	void set_color(float red, float green, float blue, float alpha)
	{
		assert(red >= 0.0f && red <= 1.0f);
		assert(green >= 0.0f && green <= 1.0f);
		assert(blue >= 0.0f && blue <= 1.0f);
		assert(alpha >= 0.0f && alpha <= 1.0f);
		color[0] = red;
		color[1] = green;
		color[2] = blue;
		color[3] = alpha;
	}

	void set_uv(float u, float v)
	{
		uv[0] = u;
		uv[1] = v;
	}
};

const size_t MAX_VERTICES = 4096;


void GUIRenderer::increment_depth()
{
	current_depth += 1.0f;
}

void GUIRenderer::startup(gui::Compositor* compositor)
{
	this->compositor = compositor;
	vertices.resize(MAX_VERTICES);

	this->vertex_buffer = device->create_vertex_buffer(MAX_VERTICES*sizeof(GUIVertex));

	// standard gui pipeline
	render2::PipelineDescriptor desc;
	desc.shader = device->create_shader("gui");
	desc.vertex_description.add("in_position", render2::VD_FLOAT, 2);
	desc.vertex_description.add("in_color", render2::VD_FLOAT, 4);
	desc.vertex_description.add("in_uv", render2::VD_FLOAT, 2);
	desc.input_layout = device->create_input_layout(desc.vertex_description, desc.shader);
	desc.enable_blending = true;
	desc.blend_source = render2::BlendOp::SourceAlpha;
	desc.blend_destination = render2::BlendOp::OneMinusSourceAlpha;
	gui_pipeline = device->create_pipeline(desc);


	// font pipeline
	render2::PipelineDescriptor fontdesc;
	fontdesc.shader = device->create_shader("font");
	fontdesc.vertex_description.add("in_position", render2::VD_FLOAT, 2);
	fontdesc.vertex_description.add("in_color", render2::VD_FLOAT, 4);
	fontdesc.vertex_description.add("in_uv", render2::VD_FLOAT, 2);
	fontdesc.input_layout = device->create_input_layout(fontdesc.vertex_description, fontdesc.shader);
	fontdesc.enable_blending = true;
	fontdesc.blend_source = render2::BlendOp::SourceAlpha;
	fontdesc.blend_destination = render2::BlendOp::OneMinusSourceAlpha;
	font_pipeline = device->create_pipeline(fontdesc);

	render2::Image white_image;
	white_image.create(4, 4, 3);
	white_image.filter = image::FILTER_NONE;
	white_image.flags = image::F_CLAMP_BORDER;
	white_image.fill(core::Color(255, 255, 255));
	white_texture = device->create_texture(white_image);
}

void GUIRenderer::shutdown(gui::Compositor* c)
{
	device->destroy_texture(white_texture);
	device->destroy_buffer(vertex_buffer);
	device->destroy_pipeline(gui_pipeline);
	device->destroy_pipeline(font_pipeline);
	
	vertices.clear();
}

void GUIRenderer::begin_frame(gui::Compositor* c)
{
	current_depth = 0.0f;
//		::renderer::RenderStream rs;
//
//		rs.add_state(::renderer::STATE_BLEND, 1 );
//		rs.add_blendfunc(::renderer::BLEND_SRC_ALPHA, ::renderer::BLEND_ONE_MINUS_SRC_ALPHA );
//		rs.add_state(::renderer::STATE_DEPTH_TEST, 0);
//		rs.add_state(::renderer::STATE_DEPTH_WRITE, 0);
//
//		rs.run_commands();
}

void GUIRenderer::end_frame()
{
//		::renderer::RenderStream rs;
//
//		rs.add_state(::renderer::STATE_BLEND, 0 );
//		rs.add_state(::renderer::STATE_DEPTH_TEST, 1);
//		rs.add_state(::renderer::STATE_DEPTH_WRITE, 1);
//		rs.run_commands();
}


void GUIRenderer::draw_bounds(const gui::Rect& bounds, const gui::Color& color) {}
void GUIRenderer::draw_textured_bounds(const gui::Rect& bounds, const gui::TextureHandle& handle) {}
void GUIRenderer::draw_line(const gui::Point& start, const gui::Point& end, const gui::Color& color) {}

gui::TextureResult GUIRenderer::texture_create(const char* path, gui::TextureHandle& handle)
{
//		assets::Texture * tex = assets::textures()->load_from_path((char*)path);
//		if ( !tex )
//		{
//			return gui::TextureResult_Failed;
//		}
//
//		handle = tex->Id();

	return gui::TextureResult_Success;
}

void GUIRenderer::texture_destroy(const gui::TextureHandle& handle)
{
	// nothing really to do in our system
}

gui::TextureResult GUIRenderer::texture_info(const gui::TextureHandle& handle, uint32_t& width, uint32_t& height, uint8_t& channels)
{
//		assets::Texture * tex = assets::textures()->find_with_id( handle );
//		if ( !tex )
//		{
//			return gui::TextureResult_Failed;
//		}
//
	return gui::TextureResult_Success;
}

gui::FontResult GUIRenderer::font_create(const char* path, gui::FontHandle& handle)
{
	Array<unsigned char> fontdata;
	core::filesystem::instance()->virtual_load_file(fontdata, path);
	render2::font::Handle fonthandle = render2::font::load_from_memory(&fontdata[0], fontdata.size(), 16);
	assert(fonthandle.is_valid());
	handle = gui::FontHandle(fonthandle);
	return gui::FontResult_Success;
}

void GUIRenderer::font_destroy(const gui::FontHandle& handle)
{
	// nothing really to do in our system
	render2::font::Handle fonthandle(handle);
	// TODO: implement this
//		render2::font::destroy_font(fonthandle);
}

gui::FontResult GUIRenderer::font_measure_string(const gui::FontHandle& handle, const char* string, gui::Rect& bounds)
{
	glm::vec2 bounds_min, bounds_max;
	render2::font::get_string_metrics(render2::font::Handle(handle), string, bounds_min, bounds_max);

	bounds.set(bounds_min.x, bounds_min.y, bounds_max.x, bounds_max.y);
	return gui::FontResult_Success;
}

void GUIRenderer::font_metrics(const gui::FontHandle& handle, size_t& height, int& ascender, int& descender)
{
	render2::font::Metrics metrics;
	render2::font::get_font_metrics(render2::font::Handle(handle), metrics);

	height = metrics.height;
	ascender = metrics.ascender;
	descender = metrics.descender;
}

size_t GUIRenderer::font_draw(const gui::FontHandle& handle, const char* string, const gui::Rect& bounds, const gui::Color& color, gui::render::Vertex* buffer, size_t buffer_size)
{
	vertices.resize(0);
	render2::font::Handle font_handle(handle);
	render2::font::draw_string(font_handle, vertices, string, core::Color(color.r(), color.g(), color.b(), color.a()));

	// todo: this seems counter-intuitive
	// copy back to the buffer
	for (size_t index = 0; index < vertices.size(); ++index)
	{
		render2::font::FontVertex& v = vertices[index];
		gui::render::Vertex& out = buffer[index];
		out.x = v.position.x + bounds.origin.x;
		out.y = v.position.y + bounds.origin.y;
		out.uv[0] = v.uv.x;
		out.uv[1] = v.uv.y;
		out.color = color;
	}

	return vertices.size();
}

size_t GUIRenderer::font_count_vertices(const gui::FontHandle& handle, const char* string)
{
	return core::str::len(string) * 6;
}

gui::TextureHandle GUIRenderer::font_get_texture(const gui::FontHandle& handle)
{
	render2::font::Handle font_handle(handle);
	render2::Texture* texture = render2::font::get_font_texture(font_handle);
	assert(texture);


	gui::TextureHandle th(1);
	return th;
}

gui::FontResult GUIRenderer::font_fetch_texture(const gui::FontHandle &handle, gui::TextureHandle &texture)
{
	return gui::FontResult_Failed;
}

void GUIRenderer::draw_command_lists(gui::render::CommandList** command_lists, size_t total_lists, Array<gui::render::Vertex>& vertex_buffer)
{
	size_t total_vertices = vertex_buffer.size();

	// temp limit
	assert(total_vertices < MAX_VERTICES);
	projection_matrix = glm::ortho(0.0f, (float)this->compositor->width, (float)this->compositor->height, 0.0f, -1.0f, 1.0f);

//		device->buffer_resize(vertex_buffer, sizeof(GUIVertex) * total_vertices);

	diffuse_texture = 0;
	gui_pipeline->constants().set("projection_matrix", &projection_matrix);
	gui_pipeline->constants().set("diffuse", &diffuse_texture);

	font_pipeline->constants().set("projection_matrix", &projection_matrix);
	font_pipeline->constants().set("diffuse", &diffuse_texture);

	assert(total_lists > 0);

	GUIVertex vertices[MAX_VERTICES];
	memset(vertices, 0, sizeof(GUIVertex)*MAX_VERTICES);

	// loop through all vertices in the source vertex_buffer
	// and convert them to our buffer
	for (size_t index = 0; index < total_vertices; ++index)
	{
		gui::render::Vertex* gv = &vertex_buffer[index];
		GUIVertex& vt = vertices[index];
		vt.set_position(gv->x, gv->y);
		vt.set_color(gv->color.r()/255.0f, gv->color.g()/255.0f, gv->color.b()/255.0f, gv->color.a()/255.0f);
		vt.set_uv(gv->uv[0], gv->uv[1]);
	}

	device->buffer_upload(this->vertex_buffer, vertices, sizeof(GUIVertex)*total_vertices);

	size_t command_index = 0;
	for (size_t index = 0; index < total_lists; ++index)
	{
		gui::render::CommandList* commandlist = command_lists[index];


		// setup the pass and queue the draw
		render2::Pass pass;
		pass.target = device->default_render_target();
		pass.clear_color = false;
		pass.clear_depth = false;

		render2::CommandQueue* queue = device->create_queue(pass);
		render2::CommandSerializer* serializer = device->create_serializer(queue);

		serializer->vertex_buffer(this->vertex_buffer);

		for (gui::render::Command& command : commandlist->commands)
		{
			render2::Pipeline* pipeline;
			render2::Texture* texture_pointer = white_texture;

			if (command.type == gui::render::CommandType_Generic)
			{
				pipeline = gui_pipeline;
			}
			else if (command.type == gui::render::CommandType_Font)
			{
				pipeline = font_pipeline;
			}
			else
			{
				// Unable to render this command type
				assert(0);
			}

			serializer->pipeline(pipeline);

			if (!command.texture.is_valid())
			{
				// no valid texture; use default white
				serializer->texture(white_texture, 0);
			}
			else
			{
				// valid texture; look it up
				texture_pointer = resource_cache.handle_to_texture(command.texture);
				serializer->texture(texture_pointer, 0);
			}
			serializer->draw(command.vertex_offset, command.vertex_count);
			++command_index;
		}

		device->queue_buffers(queue, 1);

		device->destroy_serializer(serializer);

	}
}







#if 0
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


void GUIRenderer::draw_bounds(const gui::Rect& bounds, const gui::Color& color)
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

void GUIRenderer::draw_textured_bounds(const gui::Rect& bounds, const gui::TextureHandle& handle)
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

gui::FontResult GUIRenderer::font_measure_string(const gui::FontHandle& handle, const char* string, gui::Rect& bounds)
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

void GUIRenderer::font_draw(const gui::FontHandle& handle, const char* string, const gui::Rect& bounds, const gui::Color& color)
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
	// We cannot render with this yet.
	return;


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

#endif







