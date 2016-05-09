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
#include "ui/renderer.h"
#include "ui/compositor.h"

namespace gui
{

namespace render
{
	CommandList::CommandList(Compositor* compositor_instance, Array<Vertex>* buffer) :
		vertex_buffer(buffer),
		compositor(compositor_instance)
	{
	}

	void CommandList::reset()
	{
		commands.resize(0);
		write_pointer = nullptr;
	}

	void CommandList::clear()
	{
		commands.clear();
		write_pointer = nullptr;
	}

	void CommandList::push_clip_rect(const Rect& /*clip_rect*/)
	{

	}

	void CommandList::pop_clip_rect()
	{

	}

	void CommandList::add_drawcall()
	{
		Command command;
		command.type = CommandType_Generic;
		command.vertex_offset = 0;
		command.vertex_count = 0;
		command.clip_rect = Rect(0, 0, 0, 0);
		commands.push_back(command);
	}


	// ---------------------------------------------------------------------
	// primitives
	// ---------------------------------------------------------------------
	void CommandList::primitive_reserve(size_t count)
	{
		assert(vertex_buffer);

		size_t current_vertex_offset = vertex_buffer->size();
		Command& command = commands.back();
		command.vertex_offset = current_vertex_offset;
		command.vertex_count = static_cast<uint16_t>(count);

		vertex_buffer->resize(current_vertex_offset + count);
		write_pointer = &(*vertex_buffer)[current_vertex_offset];
	}

	void quad_from_corners(gui::render::Vertex* vertices, const gui::Point& p0, const gui::Point& p1, const gui::Point& p2, const gui::Point& p3, const gemini::Color& color)
	{
		// origin is in the upper left, with Y+ towards the bottom of the screen
		// these are counter-clock wise
		vertices[0].x = p0.x;
		vertices[0].y = p0.y;
		vertices[0].color = color;
		vertices[0].uv[0] = 0;
		vertices[0].uv[1] = 1;

		vertices[1].x = p1.x;
		vertices[1].y = p1.y;
		vertices[1].color = color;
		vertices[1].uv[0] = 0;
		vertices[1].uv[1] = 0;

		vertices[2].x = p2.x;
		vertices[2].y = p2.y;
		vertices[2].color = color;
		vertices[2].uv[0] = 1;
		vertices[2].uv[1] = 0;

		vertices[3].x = p2.x;
		vertices[3].y = p2.y;
		vertices[3].color = color;
		vertices[3].uv[0] = 1;
		vertices[3].uv[1] = 0;

		vertices[4].x = p3.x;
		vertices[4].y = p3.y;
		vertices[4].color = color;
		vertices[4].uv[0] = 1;
		vertices[4].uv[1] = 1;

		vertices[5].x = p0.x;
		vertices[5].y = p0.y;
		vertices[5].color = color;
		vertices[5].uv[0] = 0;
		vertices[5].uv[1] = 1;
	}

	void CommandList::primitive_quad(const gui::Point& p0, const gui::Point& p1, const gui::Point& p2, const gui::Point& p3, const TextureHandle& texture, const gemini::Color& color)
	{
		primitive_reserve(6);
		Command& command = commands.back();
		command.texture = texture;

		quad_from_corners(&write_pointer[0], p0, p1, p2, p3, color);
	}

	void quad_from_line(gui::render::Vertex* vertices, const Point& start, const Point& end, const gemini::Color& color, float thickness)
	{
		Point corners[4];

		// use the cross product to find a perpendicular axis
		// to the line
		const glm::vec3 zaxis(0.0f, 0.0f, 1.0f);
		const glm::vec3 direction = glm::vec3(end.x - start.x, end.y - start.y, 0.0f);
		const glm::vec3 normalized_direction = glm::normalize(direction);

		glm::vec3 perpendicular_axis = glm::cross(zaxis, normalized_direction);
		glm::vec3 reflected_axis = -glm::reflect(glm::normalize(perpendicular_axis), normalized_direction);
		glm::vec2 axis(perpendicular_axis.x, perpendicular_axis.y);
		glm::vec2 raxis(reflected_axis.x, reflected_axis.y);

		const float half_thickness = (thickness * 0.5f);

		corners[0] = start + (raxis * half_thickness);
		corners[1] = start + (axis * half_thickness);
		corners[2] = end + (axis * half_thickness);
		corners[3] = end + (raxis * half_thickness);

		quad_from_corners(vertices, corners[0], corners[1], corners[2], corners[3], color);
	}

	void CommandList::add_line(const Point& start, const Point& end, const gemini::Color& color, float thickness)
	{
		add_drawcall();
		primitive_reserve(6);
		Command& command = commands.back();
		command.texture = TextureHandle();
		quad_from_line(&write_pointer[0], start, end, color, thickness);
	}

	void CommandList::add_lines(size_t line_count, const Point* vertices, const gemini::Color* colors, float thickness)
	{
		add_drawcall();
		primitive_reserve(line_count * 6);

		for (size_t index = 0; index < line_count; ++index)
		{
			quad_from_line(
				&write_pointer[(index * 6)],
				vertices[(index * 2)],
				vertices[(index * 2) + 1],
				colors[index],
				thickness
			);
		}
	}


	void CommandList::add_rectangle(const Point& p0, const Point& p1, const Point& p2, const Point& p3, const TextureHandle& texture, const gemini::Color& color)
	{
		add_drawcall();
		primitive_quad(p0, p1, p2, p3, texture, color);
	}

	void CommandList::add_font(const FontHandle& font, const char* utf8, size_t string_length, const Rect& bounds, const gemini::Color& color)
	{
		add_drawcall();

		const size_t max_vertices = 1024;

		size_t required_primitives = compositor->get_renderer()->font_count_vertices(font, string_length);
		primitive_reserve(required_primitives);

//		LOGV("font prims: %i\n", required_primitives);
		TextureHandle texture = compositor->get_resource_cache()->texture_for_font(font);
		Command& command = commands.back();
		command.texture = texture;
		command.type = CommandType_Font;
		size_t vertices_drawn = compositor->get_renderer()->font_draw(font, utf8, string_length, bounds, color, write_pointer, max_vertices);
		assert(vertices_drawn > 0);
	}
} // namespace render
} // namespace gui
