// -------------------------------------------------------------
// Copyright (C) 2012- Adam Petrone

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM,OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
// -------------------------------------------------------------

#include "renderer.hpp"
#include "debugdraw.hpp"
#include "vertexstream.hpp"
#include "assets.hpp"
#include "font.hpp"

namespace debugdraw
{
	namespace _internal
	{
		renderer::VertexStream vertex_stream;
		float current_time;
		unsigned int next_primitive = 0;
		unsigned int max_primitives = 0;
		DebugPrimitive * primitive_list = 0;
		font::Handle debug_font = 0;
		
		DebugPrimitive * request_primitive()
		{
			return &primitive_list[ next_primitive++ % max_primitives ];
		} // request_primitive
		
		// PLANE = 0: (XY plane)
		// PLANE = 1: (XZ plane)
		// PLANE = 2: (YZ plane)
		void generate_circle(const glm::vec3 & origin, glm::vec3 * vertices, int num_sides, float radius, int plane)
		{
			float degree_step = (360/(float)num_sides);
			float radians;
			glm::vec3 * vertex;
			glm::vec3 * second = 0;
			int circle_step = 0;
			num_sides *= 2;
			float ca, sa;
			
			for( int v = 0; v < num_sides; ++v )
			{
				vertex = &vertices[v];
				
				if ( second )
				{
					memcpy( vertex, second, sizeof(glm::vec3) );
					second = 0;
					v++;
					vertex = &vertices[v];
				}
				
				radians = DegToRad( circle_step * degree_step );
				ca = (radius * cos(radians));
				sa = (radius * sin(radians));
				if ( plane == 0 )
				{
					vertex->x = ca + origin[0];
					vertex->y = sa + origin[1];
					vertex->z = origin[2];
				}
				else if ( plane == 1 )
				{
					vertex->x = ca + origin[0];
					vertex->y = origin[1];
					vertex->z = sa + origin[2];
				}
				else if ( plane == 2 )
				{
					vertex->x = origin[0];
					vertex->y = ca + origin[1];
					vertex->z = sa + origin[2];
				}
				if ( v > 0 )
				{
					second = vertex;
				}
				circle_step += 1;
			}
		} // generate_circle
	}; // namespace _internal
	
	DebugPrimitive::DebugPrimitive()
	{
		timeleft = 0;
		flags = 0;
		type = 0;
	} // DebugPrimitive
	
	void startup(unsigned int max_primitives)
	{
		_internal::current_time = 0;
		_internal::max_primitives = max_primitives;
		_internal::primitive_list = CREATE_ARRAY(DebugPrimitive, max_primitives);
		
		// cache the shader we'll use
		
		// setup the vertex stream
		_internal::vertex_stream.create( sizeof(DebugDrawVertex), 4 * max_primitives, renderer::DRAW_LINES, renderer::BUFFER_DYNAMIC );
		
		
		// load the debug font we'll use
		_internal::debug_font = font::load_font_from_file(DEBUG_FONT_FILE, DEBUG_FONT_SIZE);
	} // startup
	
	void shutdown()
	{
		DESTROY_ARRAY(DebugPrimitive, _internal::primitive_list, _internal::max_primitives);
		_internal::max_primitives = 0;
		_internal::debug_font = 0;
		_internal::next_primitive = 0;

		_internal::vertex_stream.destroy();
		
		// delete shader?
	} // shutdown
	
	void update(float delta_msec)
	{
		// update internal time
		_internal::current_time += delta_msec;
			
		// run an update for each primitive
		for( unsigned int i = 0; i < _internal::max_primitives; ++i )
		{
			// if timeleft has expired, reset it and disable the primitive by setting an invalid type
			if ( _internal::primitive_list[i].timeleft <= 0 )
			{
				_internal::primitive_list[i].timeleft = 0;
				_internal::primitive_list[i].type = 0;
			}
			
			if ( _internal::primitive_list[i].timeleft > 0 )
			{
				// timeleft has a value, subtract deltatime
				_internal::primitive_list[i].timeleft -= delta_msec;
			}
		}
	} // void update
	
	
	void render(const glm::mat4 & modelview, const glm::mat4 & projection, int viewport_width, int viewport_height)
	{
		
	} // render
	
	void axes( const glm::mat4 & transform, float axis_length, float duration )
	{
		DebugPrimitive * p = _internal::request_primitive();
		if ( p )
		{
			p->type = TYPE_AXES;
			glm::vec4 col = glm::column( transform, 3 );
			p->start = glm::vec3( col );
			p->timeleft = duration;
			p->radius = axis_length;
		}
	} // axes
	
	void line( const glm::vec3 & start, const glm::vec3 & end, const Color & color, float duration )
	{
		DebugPrimitive * p = _internal::request_primitive();
		if ( p )
		{
			p->type = TYPE_LINE;
			p->start = start;
			p->end = end;
			p->color = color;
			p->timeleft = duration;
		}
	} // line
	
	void sphere( const glm::vec3 & center, const Color & color, float radius, float duration )
	{
		DebugPrimitive * p = _internal::request_primitive();
		if ( p )
		{
			p->type = TYPE_SPHERE;
			p->start = center;
			p->radius = radius;
			p->color = color;
			p->timeleft = duration;
		}
	} // sphere
	
	void point( const glm::vec3 & pt, const Color & color, float size, float duration )
	{
		DebugPrimitive * p = _internal::request_primitive();
		if ( p )
		{
			p->type = TYPE_BOX;
			p->start = glm::vec3( pt[0] - size, pt[1] - size, pt[2] - size );
			p->end = glm::vec3( pt[0] + size, pt[1] + size, pt[2] + size );
			p->color = color;
			p->timeleft = duration;
		}
	} // point
		
	void text( int x, int y, const char * string, const Color & color )
	{
		font::draw_string(_internal::debug_font, x, y, string, color);
	} // text
	
//	unsigned int DebugFontID()
//	{
//		return debugFont;
//	}
	
}; // namespace debugdraw