// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone
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

#include "debugdraw_interface.h"

#include <renderer/renderer.h>
#include <renderer/vertexstream.h>
#include <renderer/renderstream.h>
#include <renderer/font.h>

#include <core/logging.h>

using namespace gemini::debugdraw;
using namespace renderer;

namespace gemini
{
	const int MAX_CIRCLE_SIDES = 12;
	const int TOTAL_CIRCLE_VERTICES = 2 * MAX_CIRCLE_SIDES;
	
	void flush_streams(RenderStream& rs, VertexStream* vs)
	{
		long offset;
		rs.save_offset(offset);
		vs->update();
		rs.add_draw_call(vs->vertexbuffer);
		rs.run_commands();
		rs.load_offset(offset);
		vs->reset();
	} // flush_streams

	
	typedef void (*buffer_primitive_fn)(DebugDrawInterface* interface, RenderStream& rs, DebugPrimitive* primitive, VertexStream* vs, VertexStream* tri);
	
	void buffer_box(DebugDrawInterface* interface, RenderStream& rs, DebugPrimitive* primitive, VertexStream* vs, VertexStream* tri)
	{
		if ( !vs->has_room(24, 0) )
		{
			flush_streams( rs, vs );
		}
		
		glm::vec3 mins = primitive->start;
		glm::vec3 maxs = primitive->end;
		
		DebugDrawVertex* vertices = (DebugDrawVertex*)vs->request(24);
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
	
	void buffer_line(DebugDrawInterface* interface, RenderStream& rs, DebugPrimitive* primitive, renderer::VertexStream* vs, VertexStream* tri)
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
	
	void buffer_axes(DebugDrawInterface* interface, RenderStream& rs, DebugPrimitive* primitive, renderer::VertexStream* vs, VertexStream* tri)
	{
		if ( !vs->has_room(6, 0) )
		{
			flush_streams( rs, vs );
		}
		
		glm::vec3 right = glm::vec3(primitive->transform * glm::vec4(primitive->radius, 0.0f, 0.0f, 1.0f));
		DebugDrawVertex * vertices = (DebugDrawVertex*)vs->request(6);
		vertices[0].position = primitive->start;
		vertices[1].position = right;
		vertices[0].color = vertices[1].color = core::Color( 255, 0, 0 );
		
		glm::vec3 up = glm::vec3(primitive->transform * glm::vec4(0.0f, primitive->radius, 0.0f, 1.0f));
		vertices[2].position = primitive->start;
		vertices[3].position = up;
		vertices[2].color = vertices[3].color = core::Color( 0, 255, 0 );
		
		glm::vec3 view = glm::vec3(primitive->transform * glm::vec4(0.0f, 0.0f, primitive->radius, 1.0f));
		vertices[4].position = primitive->start;
		vertices[5].position = view;
		vertices[4].color = vertices[5].color = core::Color( 0, 0, 255 );
	} // buffer_axes
	
	void buffer_sphere(DebugDrawInterface* interface, RenderStream& rs, DebugPrimitive* primitive, renderer::VertexStream* vs, VertexStream* tri)
	{
		if ( !vs->has_room(TOTAL_CIRCLE_VERTICES*3, 0) )
		{
			flush_streams( rs, vs );
		}
		
		DebugDrawVertex * vertices = (DebugDrawVertex*)vs->request( TOTAL_CIRCLE_VERTICES*3 );
		glm::vec3 vlist[ TOTAL_CIRCLE_VERTICES ];
		
		// XY plane
		interface->generate_circle( primitive->start, vlist, MAX_CIRCLE_SIDES, primitive->radius, 0 );
		for( int i = 0; i < TOTAL_CIRCLE_VERTICES; ++i )
		{
			vertices[0].position = vlist[ i ];
			vertices[0].color = primitive->color;
			++vertices;
		}
		
		// XZ plane
		interface->generate_circle( primitive->start, vlist, MAX_CIRCLE_SIDES, primitive->radius, 1 );
		for( int i = 0; i < TOTAL_CIRCLE_VERTICES; ++i )
		{
			vertices[0].position = vlist[ i ];
			vertices[0].color = primitive->color;
			++vertices;
		}
		
		// YZ plane
		interface->generate_circle( primitive->start, vlist, MAX_CIRCLE_SIDES, primitive->radius, 2 );
		for( int i = 0; i < TOTAL_CIRCLE_VERTICES; ++i )
		{
			vertices[0].position = vlist[ i ];
			vertices[0].color = primitive->color;
			++vertices;
		}
	} // buffer_sphere
	
	void render_text(DebugDrawInterface* interface, RenderStream& rs, DebugPrimitive* primitive, renderer::VertexStream* vs, VertexStream* tri)
	{
		// This doesn't place the text into a buffer like the other primitives.
		// however, it is deferred to make everything render in order.
		
		font::draw_string(interface->get_debug_font(), primitive->start.x, primitive->start.y, primitive->buffer.c_str(), primitive->color);
	} // render_text
	
	
	void buffer_triangle(DebugDrawInterface* interface, RenderStream& rs, DebugPrimitive* primitive, renderer::VertexStream* vs, VertexStream* tri)
	{
		if (!tri->has_room(3, 0))
		{
			flush_streams(rs, vs);
		}
		
		DebugDrawVertex* vertices = (DebugDrawVertex*)tri->request(3);
		vertices[0].position = primitive->start;
		vertices[1].position = primitive->end;
		vertices[2].position = primitive->alt;
		vertices[0].color = vertices[1].color = vertices[2].color = primitive->color;
		
	} // buffer_triangle

	DebugPrimitive::DebugPrimitive()
	{
		timeleft = 0;
		flags = 0;
		type = 0;
	} // DebugPrimitive

	
	debugdraw::DebugPrimitive* DebugDrawInterface::request_primitive()
	{
		// If you hit this, debugdraw was not initialized!
		assert( max_primitives != 0 );
		
		return &primitive_list[ next_primitive++ % max_primitives ];
	} // request_primitive
	
	
	void DebugDrawInterface::startup(unsigned int in_max_primitives, renderer::ShaderProgram* program, const font::Handle& font)
	{
		next_primitive = 0;
		debug_shader = 0;
		max_primitives = in_max_primitives;
		primitive_list = MEMORY_NEW_ARRAY(DebugPrimitive, max_primitives, platform::memory::global_allocator());
		
		// cache the shader we'll use
		
		// setup the vertex stream
		vertex_stream.desc.add(renderer::VD_FLOAT3);
		vertex_stream.desc.add(renderer::VD_UNSIGNED_BYTE4);
		vertex_stream.create(4 * max_primitives, 0, renderer::DRAW_LINES, renderer::BUFFER_DYNAMIC);
		
		triangle_stream.desc.add(renderer::VD_FLOAT3);
		triangle_stream.desc.add(renderer::VD_UNSIGNED_BYTE4);
		triangle_stream.create(4 * max_primitives, 0, renderer::DRAW_TRIANGLES, renderer::BUFFER_DYNAMIC);
		
		// the debug font we'll use
		debug_font = font;
		
		// debug shader
		debug_shader = program;
		assert(debug_shader != 0);
	}
	
	void DebugDrawInterface::shutdown()
	{
		MEMORY_DELETE_ARRAY(primitive_list, platform::memory::global_allocator());
		max_primitives = 0;
		next_primitive = 0;
		vertex_stream.destroy();
		triangle_stream.destroy();
	}
	
	void DebugDrawInterface::update(float delta_msec)
	{
		// run an update for each primitive
		for( unsigned int i = 0; i < max_primitives; ++i )
		{
			// if timeleft has expired, reset it and disable the primitive by
			// setting an invalid type
			if (primitive_list[i].timeleft < 0)
			{
				primitive_list[i].timeleft = -1;
				primitive_list[i].type = 0;
			}
			
			if (primitive_list[i].timeleft >= 0)
			{
				// timeleft has a value, subtract deltatime
				primitive_list[i].timeleft -= delta_msec;
			}
		}
	}
	
	void DebugDrawInterface::render(const glm::mat4& modelview, const glm::mat4& projection, int x, int y, int viewport_width, int viewport_height)
	{
		
		unsigned int attribs = 0;
		//		renderer::ShaderString name;
		//		name = "colors";
		//		attribs |= assets::find_parameter_mask( name );
		
		glm::mat4 object;
		RenderStream rs;
		rs.add_viewport( x, y, viewport_width, viewport_height );
		rs.add_state( renderer::STATE_DEPTH_TEST, 0 );
		rs.add_state(renderer::STATE_BLEND, 1);
		rs.add_blendfunc(renderer::BLEND_SRC_ALPHA, renderer::BLEND_ONE_MINUS_SRC_ALPHA);
		
		rs.add_shader(debug_shader);
		rs.add_uniform_matrix4(debug_shader->get_uniform_location("modelview_matrix"), &modelview);
		rs.add_uniform_matrix4(debug_shader->get_uniform_location("projection_matrix"), &projection);
		//rs.run_commands();
		//rs.rewind();
		
		DebugPrimitive* primitive = 0;
		
		renderer::VertexStream* vs = &vertex_stream;
		
		vs->reset();
		triangle_stream.reset();
		
		buffer_primitive_fn buffer_primitive_table[] =
		{
			0,
			buffer_box,
			buffer_line,
			buffer_axes,
			buffer_sphere,
			render_text,
			buffer_triangle
		};
		
		for( unsigned int i = 0; i < max_primitives; ++i )
		{
			primitive = &primitive_list[i];
			if ( primitive->type != 0 )
			{
				buffer_primitive_table[ primitive->type ](this, rs, primitive, vs, &triangle_stream);
			}
		}
		
		vs->update();
		
		triangle_stream.update();
		
		rs.add_draw_call( vs->vertexbuffer );
		rs.add_draw_call( triangle_stream.vertexbuffer );
		rs.add_state( renderer::STATE_DEPTH_TEST, 1 );
		rs.add_state(renderer::STATE_BLEND, 0);
		rs.run_commands();
		
		
#if 0
		// on startup; create this descriptor
		
		struct DebugDrawConstantBuffer : public renderer::ConstantBuffer
		{
			DebugDrawConstantBuffer(renderer::ShaderProgram* program) : renderer::ConstantBuffer(program) {}
			
			const glm::mat4* modelview_matrix;
			const glm::mat4* projection_matrix;
		};
		
		renderer::IRenderDriver* driver = renderer::driver();
		
		renderer::ShaderProgram* program = 0;
		//assets::Shader* shader = assets::shaders()->load_from_path("shaders/debug");
		
		DebugDrawConstantBuffer cbo(program);
		cbo.add_uniform_matrix4("modelview_matrix", cbo.modelview_matrix);
		cbo.add_uniform_matrix4("projection_matrix", cbo.projection_matrix);
		
		renderer::PipelineDescriptor desc;
		desc.constant_buffer = &cbo;
		desc.program = program;
		desc.depth_write_enabled = false;
		desc.stencil_write_enabled = false;
		desc.render_target = driver->get_default_render_target();
		
		renderer::PipelineState* debug_render_state = driver->pipelinestate_create(desc);
		
		
		
		
		// each frame
		//		driver->set_render_state(renderer::STATE_DEPTH_TEST, 0);
		cbo.modelview_matrix = &modelview;
		cbo.projection_matrix = &projection;
		
		renderer::CommandBuffer buffer;
		buffer.set_pipeline_state(debug_render_state);
		buffer.draw_vertexbuffer(nullptr);
		
		buffer.commit();
		//		device.set_render_state(renderer::STATE_DEPTH_TEST, 1);
#endif

	}
	
	// PLANE = 0: (XY plane)
	// PLANE = 1: (XZ plane)
	// PLANE = 2: (YZ plane)
	void DebugDrawInterface::generate_circle(const glm::vec3 & origin, glm::vec3 * vertices, int num_sides, float radius, int plane)
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
			
			radians = mathlib::degrees_to_radians(circle_step * degree_step);
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
	
	
	void DebugDrawInterface::axes(const glm::mat4& transform, float axis_length, float duration)
	{
		DebugPrimitive* p = request_primitive();
		if ( p )
		{
			p->type = TYPE_AXES;
			glm::vec4 col = glm::column( transform, 3 );
			p->start = glm::vec3( col );
			p->transform = transform;
			p->timeleft = duration;
			p->radius = axis_length;
		}
	}
	
	void DebugDrawInterface::basis(const glm::vec3& origin, const glm::vec3& basis, float axis_length, float duration)
	{
		DebugPrimitive* p = request_primitive();
		if (p)
		{
			p->type = TYPE_AXES;
			glm::mat4 transform;
			transform[0] = glm::vec4(basis.x, 0, 0, 0);
			transform[1] = glm::vec4(0, basis.y, 0, 0);
			transform[2] = glm::vec4(0, 0, basis.z, 0);
			transform[3] = glm::vec4(origin.x, origin.y, origin.z, 0);
			p->start = origin;
			p->transform = transform;
			p->timeleft = duration;
			p->radius = axis_length;
		}
	}
	
	void DebugDrawInterface::box(const glm::vec3& mins, const glm::vec3& maxs, const core::Color& color, float duration)
	{
		DebugPrimitive* p = request_primitive();
		if ( p )
		{
			p->type = TYPE_BOX;
			p->start = mins;
			p->end = maxs;
			p->color = color;
			p->timeleft = duration;
		}
	}
	
	void DebugDrawInterface::point(const glm::vec3& pt, const core::Color& color, float size, float duration)
	{
		DebugPrimitive* p = request_primitive();
		if ( p )
		{
			p->type = TYPE_BOX;
			p->start = glm::vec3( pt[0] - size, pt[1] - size, pt[2] - size );
			p->end = glm::vec3( pt[0] + size, pt[1] + size, pt[2] + size );
			p->color = color;
			p->timeleft = duration;
		}
	}
	
	void DebugDrawInterface::line(const glm::vec3& start, const glm::vec3& end, const core::Color& color, float duration)
	{
		DebugPrimitive* p = request_primitive();
		if ( p )
		{
			p->type = TYPE_LINE;
			p->start = start;
			p->end = end;
			p->color = color;
			p->timeleft = duration;
		}
	}
	
	void DebugDrawInterface::sphere(const glm::vec3& center, const core::Color& color, float radius, float duration)
	{
		DebugPrimitive* p = request_primitive();
		if ( p )
		{
			p->type = TYPE_SPHERE;
			p->start = center;
			p->radius = radius;
			p->color = color;
			p->timeleft = duration;
		}
	}
	
	void DebugDrawInterface::text(int x, int y, const char* string, const core::Color& color, float duration)
	{
		DebugPrimitive* p = request_primitive();
		if ( p )
		{
			p->type = TYPE_TEXT;
			p->start = glm::vec3(x, y, 0);
			p->color = color;
			p->timeleft = duration;
			p->buffer = string;
		}
	}
	
	void DebugDrawInterface::triangle(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, const core::Color& color, float duration)
	{
		DebugPrimitive* p = request_primitive();
		if (p)
		{
			p->type = TYPE_TRIANGLE;
			p->start = v0;
			p->end = v1;
			p->color = color;
			p->timeleft = duration;
			p->alt = v2;
		}
	}
} // namespace gemini