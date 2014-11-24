// -------------------------------------------------------------
// Copyright (C) 2004- Adam Petrone

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
#include <stdlib.h>

#include <platform/typedefs.h>
#include <core/filesystem.h>

#include <slim/xstr.h>
#include <slim/xlog.h>

#include "font.h"
#include "renderer.h"
#include "renderstream.h"
#include "fontstash.h"



namespace font
{
	const unsigned int FONT_MAX_VERTICES = 1024;

	namespace internal
	{	
		renderer::VertexStream _vertexstream;
		struct sth_stash * _stash;
		renderer::ShaderProgram * _shader;
		
		struct StashData
		{
			renderer::Texture* texture;
			int render_width;
			int render_height;
			
			StashData() : texture(nullptr), render_width(0), render_height(0) {}
		};
		
		uint32_t _uniforms[3];
	}; // namespace internal
	

	
	void* f_generate_texture( int width, int height, void * pixels )
	{
		renderer::IRenderDriver * driver = renderer::driver();
		renderer::Texture* texture = nullptr;
				
		image::Image image;
		image.flags |= image::F_ALPHA;
		image.width = width;
		image.height = height;
		image.pixels = (uint8_t*)pixels;
		
		texture = driver->texture_create(image);
		
		LOGV( "[font] texture dimensions %i x %i\n", width, height );
		return texture;
	}
	
	void f_delete_texture(void* userdata)
	{
		internal::StashData* data = static_cast<internal::StashData*>(userdata);
		renderer::IRenderDriver * driver = renderer::driver();
		driver->texture_destroy(data->texture);
	}
	
	void f_update_texture(void* userdata, int origin_x, int origin_y, int width, int height, void * pixels)
	{
		renderer::IRenderDriver * driver = renderer::driver();
		internal::StashData* data = static_cast<internal::StashData*>(userdata);

		image::Image image;
		image.flags |= image::F_ALPHA;
		image.width = width;
		image.height = height;
		image.pixels = (unsigned char*)pixels;
		
		gemini::Recti area(origin_x, origin_y, width, height);
		
		driver->texture_update(data->texture, image, area);
	}

	void f_draw_with_texture(void* userdata, void * vertex_data, int uv_offset, int color_offset, int stride, int vertex_count)
	{
		renderer::VertexStream & vs = internal::_vertexstream;
		internal::StashData* data = static_cast<internal::StashData*>(userdata);
		
		if ( !vs.has_room( vertex_count, 0 ) )
		{
			LOGE( "Unable to draw font: vertexstream has no room! (%i)\n", vs.total_vertices );
		}
		else
		{
			internal::_vertexstream.fill_data( (renderer::VertexType*)vertex_data, vertex_count, 0, 0 );
			internal::_vertexstream.update();

			glm::mat4 modelview_matrix = glm::mat4(1.0f);
			glm::mat4 projection_matrix;

			assert(0);
			float w = 0; float h = 0;
			w = (real)data->render_width;
			h = (real)data->render_height;
			projection_matrix = glm::ortho(0.0f, w, 0.0f, h, -1.0f, 1.0f );
			
			RenderStream rs;
			
			// activate shader
			rs.add_shader( internal::_shader );
			
			// setup uniforms
			rs.add_uniform_matrix4(internal::_uniforms[0], &modelview_matrix );
			rs.add_uniform_matrix4(internal::_uniforms[1], &projection_matrix );
			rs.add_sampler2d(internal::_uniforms[2], 0, data->texture);
			
			// add draw call for vertexbuffer
			rs.add_draw_call( internal::_vertexstream.vertexbuffer );
			
			// run all commands
			rs.run_commands();
			
			vs.reset();
		}
	}


	void startup(renderer::ShaderProgram* fontshader)
	{	
		// initialize the vertex stream
		internal::_vertexstream.desc.add( renderer::VD_FLOAT2 );
		internal::_vertexstream.desc.add( renderer::VD_FLOAT2 );
		internal::_vertexstream.desc.add( renderer::VD_UNSIGNED_BYTE4 );
		
		internal::_vertexstream.create( FONT_MAX_VERTICES, 0, renderer::DRAW_TRIANGLES, renderer::BUFFER_STREAM );
		
		
		sth_render_callbacks cb;
		cb.generate_texture = f_generate_texture;
		cb.update_texture = f_update_texture;
		cb.delete_texture = f_delete_texture;
		cb.draw_with_texture = f_draw_with_texture;
		
		// this must be called before any sth_create commands
		sth_set_render_callbacks( &cb );
		
		internal::_stash = sth_create( 256, 256 );
		internal::_shader = fontshader;
		assert(internal::_shader != 0);
		
		internal::_uniforms[0] = internal::_shader->get_uniform_location("modelview_matrix");
		internal::_uniforms[1] = internal::_shader->get_uniform_location("projection_matrix");
		internal::_uniforms[2] = internal::_shader->get_uniform_location("diffusemap");
		
		// generate ccw triangles
		sth_set_ccw_triangles();
	} // startup

	void shutdown()
	{
		// cleanup used memory here
		internal::_vertexstream.destroy();
		
		if ( internal::_stash )
		{
			// delete internal font stash
			sth_delete( internal::_stash );
			internal::_stash = 0;
		}
		
		sth_reset_internal_data();
	} // shutdown
	

	
	void draw_string(const renderer::Font& font, int x, int y, const char* utf8, const Color& color)
	{
		// TODO: should be passed in from callee
		// y = (render_height-y-titlebar_height);
		
		if (font.handle == 0)
		{
			return;
		}

		RenderStream rs;

		// setup global rendering state
		rs.add_state(renderer::STATE_DEPTH_TEST, 0);
		rs.add_state(renderer::STATE_BLEND, 1);
		rs.add_blendfunc(renderer::BLEND_SRC_ALPHA, renderer::BLEND_ONE_MINUS_SRC_ALPHA);
		rs.run_commands();
				
		// draw
		sth_begin_draw( internal::_stash );
		float width = 0;
		unsigned int vcolor = STH_RGBA(color.r, color.g, color.b, color.a);
		sth_draw_text(internal::_stash, font.handle, font.point_size, x, y, vcolor, utf8, &width);
		sth_end_draw(internal::_stash);
		
		// restore state
		rs.rewind();
		rs.add_state( renderer::STATE_BLEND, 0 );
		rs.add_state( renderer::STATE_DEPTH_TEST, 1 );
		rs.run_commands();
	} // draw_string
	
	
	void dimensions_for_text(const renderer::Font& font, const char* utf8, float& minx, float& miny, float& maxx, float& maxy)
	{
		if (font.handle > 0)
		{
			sth_dim_text(internal::_stash, font.handle, font.handle, utf8, &minx, &miny, &maxx, &maxy);
		}
	} // dimensions_for_text
	
	unsigned int measure_height(const renderer::Font& font, const char * utf8 )
	{
		if (font.handle == 0)
		{
			return 0;
		}
		
		float minx = 0, miny = 0, maxx = 0, maxy = 0;
		dimensions_for_text(font, utf8, minx, miny, maxx, maxy);
		return maxy-miny;
	} // measure_height
	
	unsigned int measure_width(const renderer::Font& font, const char* utf8 )
	{
		if (font.handle == 0)
		{
			return 0;
		}
		
		float minx = 0, miny = 0, maxx = 0, maxy = 0;
		dimensions_for_text(font, utf8, minx, miny, maxx, maxy);
		return maxx-minx;
	} // measure_width
	
	renderer::Font load_font_from_memory(const void* data, unsigned int data_size, unsigned short point_size)
	{
		assert(internal::_stash != 0);
		
		renderer::Font font;
		
		font.handle = sth_add_font_from_memory(internal::_stash, (unsigned char*)data);
		if ( font.handle == 0 )
		{
			LOGE( "Unable to load font from memory!\n" );
			font.handle = 0;
		}

		return font;
	} // load_font_from_memory
}; // namespace font
