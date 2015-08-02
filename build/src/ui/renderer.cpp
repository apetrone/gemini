/*
Copyright (c) 2015, <Adam Petrone>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "ui/renderer.h"

namespace gui { namespace render {
	void CommandList::reset()
	{
		commands.resize(0);
		vertex_buffer.resize(0);
		write_pointer = nullptr;
	}

	void CommandList::clear()
	{
		commands.clear();
		vertex_buffer.clear();
		write_pointer = nullptr;
	}
	
	void CommandList::push_clip_rect(const Rect& clip_rect)
	{
		
	}
	
	void CommandList::pop_clip_rect()
	{

	}
	
	void CommandList::add_drawcall()
	{
		Command command;
		command.vertex_count = 0;
		command.clip_rect = Rect(0, 0, 0, 0);
		commands.push_back(command);
	}
	
	
	// ---------------------------------------------------------------------
	// primitives
	// ---------------------------------------------------------------------
	void CommandList::primitive_reserve(size_t count)
	{
		Command& command = commands.back();
		command.vertex_count += count;
		
		size_t current_vertex_count = vertex_buffer.size();
		vertex_buffer.resize(current_vertex_count + count);
		write_pointer = &vertex_buffer[current_vertex_count];
	}
	
	void CommandList::primitive_quad(const gui::Point &p0, const gui::Point &p1, const gui::Point &p2, const gui::Point &p3, const TextureHandle& texture, const gui::Color &color)
	{
		primitive_reserve(6);
		Command& command = commands.back();
		command.id = 0;
		command.texture = texture;
		
		// origin is in the upper left, with Y+ towards the bottom of the screen
		
		write_pointer[0].x = p0.x;
		write_pointer[0].y = p0.y;
		write_pointer[0].color = color;
		write_pointer[0].uv[0] = 0;
		write_pointer[0].uv[1] = 0;
				
		write_pointer[1].x = p1.x;
		write_pointer[1].y = p1.y;
		write_pointer[1].color = color;
		write_pointer[1].uv[0] = 0;
		write_pointer[1].uv[1] = 1;
		
		write_pointer[2].x = p2.x;
		write_pointer[2].y = p2.y;
		write_pointer[2].color = color;
		write_pointer[2].uv[0] = 1;
		write_pointer[2].uv[1] = 1;
		
		write_pointer[3].x = p2.x;
		write_pointer[3].y = p2.y;
		write_pointer[3].color = color;
		write_pointer[3].uv[0] = 1;
		write_pointer[3].uv[1] = 1;
		
		write_pointer[4].x = p3.x;
		write_pointer[4].y = p3.y;
		write_pointer[4].color = color;
		write_pointer[4].uv[0] = 1;
		write_pointer[4].uv[1] = 0;
		
		write_pointer[5].x = p0.x;
		write_pointer[5].y = p0.y;
		write_pointer[5].color = color;
		write_pointer[5].uv[0] = 0;
		write_pointer[5].uv[1] = 0;
	}
	
	void CommandList::add_line(const Point& start, const Point& end, const gui::Color& color)
	{
//		add_drawcall();
//		primitive_reserve(6);
//		Command& command = commands.back();
//		command.id = 2;
//		command.texture = 0;
	}
	
	void CommandList::add_rectangle(const Point& p0, const Point& p1, const Point& p2, const Point& p3, const TextureHandle& texture, const gui::Color& color)
	{
		add_drawcall();
		primitive_reserve(6);
		Command& command = commands.back();
		command.id = 2;
		command.texture = texture;
		
		// origin is in the upper left, with Y+ towards the bottom of the screen
		
		write_pointer[0].x = p0.x;
		write_pointer[0].y = p0.y;
		write_pointer[0].color = color;
		write_pointer[0].uv[0] = 0;
		write_pointer[0].uv[1] = 0;
		
		write_pointer[1].x = p1.x;
		write_pointer[1].y = p1.y;
		write_pointer[1].color = color;
		write_pointer[1].uv[0] = 0;
		write_pointer[1].uv[1] = 1;
		
		write_pointer[2].x = p2.x;
		write_pointer[2].y = p2.y;
		write_pointer[2].color = color;
		write_pointer[2].uv[0] = 1;
		write_pointer[2].uv[1] = 1;
		
		write_pointer[3].x = p2.x;
		write_pointer[3].y = p2.y;
		write_pointer[3].color = color;
		write_pointer[3].uv[0] = 1;
		write_pointer[3].uv[1] = 1;
		
		write_pointer[4].x = p3.x;
		write_pointer[4].y = p3.y;
		write_pointer[4].color = color;
		write_pointer[4].uv[0] = 1;
		write_pointer[4].uv[1] = 0;
		
		write_pointer[5].x = p0.x;
		write_pointer[5].y = p0.y;
		write_pointer[5].color = color;
		write_pointer[5].uv[0] = 0;
		write_pointer[5].uv[1] = 0;
	}
	
	void CommandList::add_font(const FontHandle& font, const char* utf8, const Rect& bounds, const TextureHandle& texture, const gui::Color& color)
	{
//		add_drawcall();
//		primitive_reserve(6);
//		Command& command = commands.back();
//		command.id = 2;
//		command.texture = texture;
	}
} }
