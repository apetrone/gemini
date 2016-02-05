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

#include <ui/compositor.h>

#include <runtime/filesystem.h>

#include <renderer/renderer.h>
#include <renderer/font.h>


struct GUIVertex
{
	glm::vec2 position;
	gemini::Color color;
	glm::vec2 uv;

	void set_position(float x, float y)
	{
		position.x = x;
		position.y = y;
	}

	void set_uv(float u, float v)
	{
		uv[0] = u;
		uv[1] = v;
	}
};

const size_t MAX_VERTICES = 8192;


void GUIRenderer::increment_depth()
{
	current_depth += 1.0f;
}

void GUIRenderer::startup(gui::Compositor* target_compositor)
{
	compositor = target_compositor;
	vertex_cache.resize(MAX_VERTICES);

	vertex_buffer = device->create_vertex_buffer(MAX_VERTICES * sizeof(GUIVertex));

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
	desc.primitive_type = render2::PrimitiveType::Triangles;
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
	white_image.fill(gemini::Color::from_rgba(255, 255, 255, 255));
	white_texture = device->create_texture(white_image);
}

void GUIRenderer::shutdown(gui::Compositor* c)
{
	if (white_texture)
	{
		device->destroy_texture(white_texture);
	}

	if (vertex_buffer)
	{
		device->destroy_buffer(vertex_buffer);
	}

	if (gui_pipeline)
	{
		device->destroy_pipeline(gui_pipeline);
	}

	if (font_pipeline)
	{
		device->destroy_pipeline(font_pipeline);
	}

	vertex_cache.clear();
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
	font::Handle fonthandle = font::load_from_memory(&fontdata[0], fontdata.size(), 16);
	assert(fonthandle.is_valid());
	handle = gui::FontHandle(fonthandle);
	return gui::FontResult_Success;
}

void GUIRenderer::font_destroy(const gui::FontHandle& handle)
{
	// nothing really to do in our system
	font::Handle fonthandle(handle);
	// TODO: implement this
//		font::destroy_font(fonthandle);
}

gui::FontResult GUIRenderer::font_measure_string(const gui::FontHandle& handle, const char* string, size_t string_length, gui::Rect& bounds)
{
	glm::vec2 bounds_min, bounds_max;
	font::get_string_metrics(font::Handle(handle), string, string_length, bounds_min, bounds_max);

	bounds.set(bounds_min.x, bounds_min.y, bounds_max.x, bounds_max.y);
	return gui::FontResult_Success;
}

void GUIRenderer::font_metrics(const gui::FontHandle& handle, size_t& height, int& ascender, int& descender)
{
	font::Metrics metrics;
	font::get_font_metrics(font::Handle(handle), metrics);

	height = metrics.height;
	ascender = metrics.ascender;
	descender = metrics.descender;
}

size_t GUIRenderer::font_draw(const gui::FontHandle& handle, const char* string, size_t string_length, const gui::Rect& bounds, const gemini::Color& color, gui::render::Vertex* buffer, size_t buffer_size)
{
	font::Handle font_handle(handle);

	const size_t vertices_required = font::count_vertices(font_handle, string_length);
	vertex_cache.resize(vertices_required);
	font::draw_string(font_handle, &vertex_cache[0], string, string_length, color);

	// todo: this seems counter-intuitive
	// copy back to the buffer
	for (size_t index = 0; index < vertex_cache.size(); ++index)
	{
		font::FontVertex& v = vertex_cache[index];
		gui::render::Vertex& out = buffer[index];
		out.x = v.position.x + bounds.origin.x;
		out.y = v.position.y + bounds.origin.y;
		out.uv[0] = v.uv.x;
		out.uv[1] = v.uv.y;
		out.color = color;
	}

	return vertex_cache.size();
}

size_t GUIRenderer::font_count_vertices(const gui::FontHandle& handle, size_t string_length)
{
	font::Handle font_handle(handle);
	return font::count_vertices(font_handle, string_length);
}

void GUIRenderer::draw_commands(gui::render::CommandList* command_list, Array<gui::render::Vertex>& vertex_array)
{
	size_t total_vertices = vertex_array.size();

	// temp limit
	assert(total_vertices < MAX_VERTICES);
	projection_matrix = glm::ortho(0.0f, (float)this->compositor->width, (float)this->compositor->height, 0.0f, -1.0f, 1.0f);

	diffuse_texture = 0;
	gui_pipeline->constants().set("projection_matrix", &projection_matrix);
	gui_pipeline->constants().set("diffuse", &diffuse_texture);

	font_pipeline->constants().set("projection_matrix", &projection_matrix);
	font_pipeline->constants().set("diffuse", &diffuse_texture);

	GUIVertex vertices[MAX_VERTICES];
	memset(vertices, 0, sizeof(GUIVertex) * MAX_VERTICES);

	// loop through all vertices in the source vertex_buffer
	// and convert them to our buffer
	for (size_t index = 0; index < total_vertices; ++index)
	{
		gui::render::Vertex* gv = &vertex_array[index];
		GUIVertex& vt = vertices[index];
		vt.set_position(gv->x, gv->y);
		vt.color = gv->color;
		vt.set_uv(gv->uv[0], gv->uv[1]);
	}

	device->buffer_upload(vertex_buffer, vertices, sizeof(GUIVertex) * total_vertices);

	// setup the pass and queue the draw
	render2::Pass pass;
	pass.target = device->default_render_target();
	pass.clear_color = false;
	pass.clear_depth = false;
	pass.depth_test = false;

	render2::CommandQueue* queue = device->create_queue(pass);
	render2::CommandSerializer* serializer = device->create_serializer(queue);

	serializer->vertex_buffer(vertex_buffer);

	for (gui::render::Command& command : command_list->commands)
	{
		render2::Pipeline* pipeline = nullptr;
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
	}

	device->queue_buffers(queue, 1);

	device->destroy_serializer(serializer);
}



