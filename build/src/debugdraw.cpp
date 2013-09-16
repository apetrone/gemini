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
#include "renderstream.hpp"
#include <slim/xlog.h>

namespace debugdraw
{
	namespace _internal
	{
		renderer::VertexStream vertex_stream;
		float current_time;
		unsigned int next_primitive = 0;
		unsigned int max_primitives = 0;
		DebugPrimitive * primitive_list = 0;
		assets::Font * debug_font = 0;

		DebugPrimitive * request_primitive()
		{
			// If you hit this, debugdraw was not initialized!
			assert( max_primitives != 0 );
			
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
		_internal::vertex_stream.desc.add( renderer::VD_FLOAT3 );
		_internal::vertex_stream.desc.add( renderer::VD_UNSIGNED_BYTE4 );
		_internal::vertex_stream.create( 4 * max_primitives, 0, renderer::DRAW_LINES, renderer::BUFFER_DYNAMIC );
		
		
		// load the debug font we'll use
		_internal::debug_font = assets::fonts()->load_from_path( DEBUG_FONT_FILE );
	} // startup
	
	void shutdown()
	{
		DESTROY_ARRAY(DebugPrimitive, _internal::primitive_list, _internal::max_primitives);
		_internal::max_primitives = 0;
		_internal::debug_font = 0;
		_internal::next_primitive = 0;

		_internal::vertex_stream.destroy();
	} // shutdown
	
	void update(float delta_msec)
	{
		// update internal time
		_internal::current_time += delta_msec;
			
		// run an update for each primitive
		for( unsigned int i = 0; i < _internal::max_primitives; ++i )
		{
			// if timeleft has expired, reset it and disable the primitive by
			// setting an invalid type
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
	
	
	void flush_streams( RenderStream & rs, renderer::VertexStream * vs )
	{
		long offset;
		rs.save_offset( offset );
		vs->update();
		rs.add_draw_call( vs->vertexbuffer );
		rs.run_commands();
		rs.load_offset( offset );
		vs->reset();
	} // flush_streams
	
	
	typedef void (*buffer_primitive_fn)( RenderStream & rs, DebugPrimitive * primitive, renderer::VertexStream * vs );
		
	void buffer_box( RenderStream & rs, DebugPrimitive * primitive, renderer::VertexStream * vs )
	{
		if ( !vs->has_room(24, 0) )
		{
			flush_streams( rs, vs );
		}
	
		glm::vec3 mins = primitive->start;
		glm::vec3 maxs = primitive->end;
		
		DebugDrawVertex * vertices = (DebugDrawVertex*)vs->request(24);
		for( int i = 0; i < 24; ++i )
		{
			vertices[i].color = primitive->color;
		}
		
		// -Z face
		vertices[0].position = mins;
		vertices[1].position = glm::vec3( maxs[0], mins[1], mins[2] );
		vertices[2].position = glm::vec3( maxs[0], mins[1], mins[2] );
		vertices[3].position = glm::vec3( maxs[0], maxs[1], mins[2] );
		vertices[4].position = glm::vec3( maxs[0], maxs[1], mins[2] );
		vertices[5].position = glm::vec3( mins[0], maxs[1], mins[2] );
		vertices[6].position = glm::vec3( mins[0], maxs[1], mins[2] );
		vertices[7].position = mins;
		
		// +Z face
		vertices[8].position = glm::vec3( mins[0], mins[1], maxs[2] );
		vertices[9].position = glm::vec3( maxs[0], mins[1], maxs[2] );
		vertices[10].position = glm::vec3( maxs[0], mins[1], maxs[2] );
		vertices[11].position = glm::vec3( maxs[0], maxs[1], maxs[2] );
		vertices[12].position = glm::vec3( maxs[0], maxs[1], maxs[2] );
		vertices[13].position = glm::vec3( mins[0], maxs[1], maxs[2] );
		vertices[14].position = glm::vec3( mins[0], maxs[1], maxs[2] );
		vertices[15].position = glm::vec3( mins[0], mins[1], maxs[2] );
		
		// lower left
		vertices[16].position = mins;
		vertices[17].position = glm::vec3( mins[0], mins[1], maxs[2] );
		
		// lower right
		vertices[18].position = glm::vec3( maxs[0], mins[1], mins[2] );
		vertices[19].position = glm::vec3( maxs[0], mins[1], maxs[2] );
		
		// upper right
		vertices[20].position = glm::vec3( maxs[0], maxs[1], mins[2] );
		vertices[21].position = glm::vec3( maxs[0], maxs[1], maxs[2] );
		
		// upper left
		vertices[22].position = glm::vec3( mins[0], maxs[1], mins[2] );
		vertices[23].position = glm::vec3( mins[0], maxs[1], maxs[2] );
	} // buffer_box
	
	void buffer_line( RenderStream & rs, DebugPrimitive * primitive, renderer::VertexStream * vs )
	{
		if ( !vs->has_room(2, 0) )
		{
			flush_streams( rs, vs );
		}
		
		DebugDrawVertex * vertices = (DebugDrawVertex*)vs->request(2);
		vertices[0].color = primitive->color;
		vertices[0].position = primitive->start;
		vertices[1].color = primitive->color;
		vertices[1].position = primitive->end;
	} // buffer_line
	
	void buffer_axes( RenderStream & rs, DebugPrimitive * primitive, renderer::VertexStream * vs )
	{
		if ( !vs->has_room(6, 0) )
		{
			flush_streams( rs, vs );
		}
		
		DebugDrawVertex * vertices = (DebugDrawVertex*)vs->request(6);
		vertices[0].position = primitive->start;
		vertices[1].position = primitive->start;
		vertices[1].position[0] = vertices[1].position[0] + primitive->radius;
		vertices[0].color = vertices[1].color = Color( 255, 0, 0 );
		
		vertices[2].position = primitive->start;
		vertices[3].position = primitive->start;
		vertices[3].position[1] = vertices[3].position[1] + primitive->radius;
		vertices[2].color = vertices[3].color = Color( 0, 255, 0 );
		
		vertices[4].position = primitive->start;
		vertices[5].position = primitive->start;
		vertices[5].position[2] = vertices[5].position[2] + primitive->radius;
		vertices[4].color = vertices[5].color = Color( 0, 0, 255 );
	} // buffer_axes
	
	void buffer_sphere( RenderStream & rs, DebugPrimitive * primitive, renderer::VertexStream * vs )
	{
		if ( !vs->has_room(TOTAL_CIRCLE_VERTICES*3, 0) )
		{
			flush_streams( rs, vs );
		}

		DebugDrawVertex * vertices = (DebugDrawVertex*)vs->request( TOTAL_CIRCLE_VERTICES*3 );
		glm::vec3 vlist[ TOTAL_CIRCLE_VERTICES ];
		
		// XY plane
		_internal::generate_circle( primitive->start, vlist, MAX_CIRCLE_SIDES, primitive->radius, 0 );
		for( int i = 0; i < TOTAL_CIRCLE_VERTICES; ++i )
		{
			vertices[0].position = vlist[ i ];
			vertices[0].color = primitive->color;
			++vertices;
		}
		
		// XZ plane
		_internal::generate_circle( primitive->start, vlist, MAX_CIRCLE_SIDES, primitive->radius, 1 );
		for( int i = 0; i < TOTAL_CIRCLE_VERTICES; ++i )
		{
			vertices[0].position = vlist[ i ];
			vertices[0].color = primitive->color;
			++vertices;
		}
		
		// YZ plane
		_internal::generate_circle( primitive->start, vlist, MAX_CIRCLE_SIDES, primitive->radius, 2 );
		for( int i = 0; i < TOTAL_CIRCLE_VERTICES; ++i )
		{
			vertices[0].position = vlist[ i ];
			vertices[0].color = primitive->color;
			++vertices;
		}
	} // buffer_sphere
	
	void render(const glm::mat4 & modelview, const glm::mat4 & projection,
		int viewport_width, int viewport_height)
	{
		unsigned int attribs = 0;
		assets::ShaderString name;
		name = "colors";
		attribs |= assets::find_parameter_mask( name );
		
		assets::Shader * shader = assets::find_compatible_shader( attribs );
		if ( !shader )
		{
			LOGE("debugdraw shader not found!\n" );
			return;
		}

		glm::mat4 object;
		RenderStream rs;
		rs.add_viewport( 0, 0, viewport_width, viewport_height );
		rs.add_state( renderer::STATE_DEPTH_TEST, 0 );
		
		rs.add_shader( shader );
		rs.add_uniform_matrix4( shader->get_uniform_location("modelview_matrix"), &modelview );
		rs.add_uniform_matrix4( shader->get_uniform_location("projection_matrix"), &projection );
		rs.add_uniform_matrix4( shader->get_uniform_location("object_matrix"), &object );
		rs.run_commands();
		rs.rewind();
		
		DebugPrimitive * primitive = 0;
		
		renderer::VertexStream * vs = &_internal::vertex_stream;
		
		vs->reset();
		
		buffer_primitive_fn buffer_primitive_table[] =
		{
			0,
			buffer_box,
			buffer_line,
			buffer_axes,
			buffer_sphere
		};
		
		for( unsigned int i = 0; i < _internal::max_primitives; ++i )
		{
			primitive = &_internal::primitive_list[i];
			if ( primitive->type != 0 )
			{
				buffer_primitive_table[ primitive->type ]( rs, primitive, vs );
			}
		}
		
		vs->update();
		rs.add_draw_call( vs->vertexbuffer );
		
		rs.add_state( renderer::STATE_DEPTH_TEST, 1 );
		rs.run_commands();

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