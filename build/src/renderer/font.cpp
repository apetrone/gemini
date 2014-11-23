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
//#include "kernel.h"
#include "fontstash.h"



namespace font
{
	const unsigned int FONT_MAX_VERTICES = 1024;

	namespace internal
	{	
		renderer::VertexStream _vertexstream;
		struct sth_stash * _stash;
		renderer::ShaderProgram * _shader;
		
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
		renderer::Texture* texture = static_cast<renderer::Texture*>(userdata);
		renderer::IRenderDriver * driver = renderer::driver();
		driver->texture_destroy(texture);
	}
	
	void f_update_texture(void* userdata, int origin_x, int origin_y, int width, int height, void * pixels)
	{
		renderer::IRenderDriver * driver = renderer::driver();
		renderer::Texture* texture = static_cast<renderer::Texture*>(userdata);

		image::Image image;
		image.flags |= image::F_ALPHA;
		image.width = width;
		image.height = height;
		image.pixels = (unsigned char*)pixels;
		
		gemini::Recti area(origin_x, origin_y, width, height);
		
		driver->texture_update(texture, image, area);
	}

	void f_draw_with_texture(void* userdata, void * data, int uv_offset, int color_offset, int stride, int vertex_count)
	{
		renderer::VertexStream & vs = internal::_vertexstream;

		renderer::Texture* texture = static_cast<renderer::Texture*>(userdata);

		if ( !vs.has_room( vertex_count, 0 ) )
		{
			LOGE( "Unable to draw font: vertexstream has no room! (%i)\n", vs.total_vertices );
		}
		else
		{
			internal::_vertexstream.fill_data( (renderer::VertexType*)data, vertex_count, 0, 0 );
			internal::_vertexstream.update();
			
			renderer::ShaderProgram* shader = internal::_shader;
			
			glm::mat4 modelview_matrix = glm::mat4(1.0f);
			glm::mat4 projection_matrix;

			assert(0);
			float w = 0; float h = 0;
//			real w = (real)kernel::instance()->parameters().render_width;
//			real h = (real)kernel::instance()->parameters().render_height;
			projection_matrix = glm::ortho(0.0f, w, 0.0f, h, -1.0f, 1.0f );
			
			RenderStream rs;
			
			// activate shader
			rs.add_shader( internal::_shader );
			
			// setup uniforms
			rs.add_uniform_matrix4(internal::_uniforms[0], &modelview_matrix );
			rs.add_uniform_matrix4(internal::_uniforms[1], &projection_matrix );
			rs.add_sampler2d(internal::_uniforms[2], 0, texture);
			
			// add draw call for vertexbuffer
			rs.add_draw_call( internal::_vertexstream.vertexbuffer );
			
			// run all commands
			rs.run_commands();
			
			vs.reset();
		}
	}


	void startup()
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
//		internal::_shader = assets::shaders()->load_from_path("shaders/fontshader");
		assert(internal::_shader != 0);
		
		internal::_uniforms[0] = internal::_shader->get_uniform_location("modelview_matrix");
		internal::_uniforms[1] = internal::_shader->get_uniform_location("projection_matrix");
		internal::_uniforms[2] = internal::_shader->get_uniform_location("diffusemap");
		
		// generate ccw triangles
		sth_set_ccw_triangles();
	} // startup

	void shutdown()
	{	
//		if ( internal::_shader )
//		{
//			assets::destroy_shader( internal::_shader );
//		
//			DESTROY( Shader, internal::_shader );
//		}
	
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
	

	
	void draw_string( renderer::Font * font, int x, int y, const char * utf8, const Color & color )
	{
//		int r_width = kernel::instance()->parameters().render_width;

		assert(0);
		int r_height = 0; //kernel::instance()->parameters().render_height;
		int y_offset = 0; //kernel::instance()->parameters().titlebar_height;
		
		if ( !font )
		{
			return;
		}

		RenderStream rs;

		// setup global rendering state
		rs.add_state( renderer::STATE_DEPTH_TEST, 0 );
		rs.add_state( renderer::STATE_BLEND, 1 );
		rs.add_blendfunc( renderer::BLEND_SRC_ALPHA, renderer::BLEND_ONE_MINUS_SRC_ALPHA );
		rs.run_commands();
				
		// draw
		sth_begin_draw( internal::_stash );
		float width = 0;
		unsigned int vcolor = STH_RGBA(color.r, color.g, color.b, color.a);
		assert(0);
//		sth_draw_text( internal::_stash, font->font_id, font->font_size, x, r_height-y-y_offset, vcolor, utf8, &width );
		sth_end_draw( internal::_stash );
		
		// restore state
		rs.rewind();
		rs.add_state( renderer::STATE_BLEND, 0 );
		rs.add_state( renderer::STATE_DEPTH_TEST, 1 );
		rs.run_commands();
	} // draw_string
	
	
	void dimensions_for_text( renderer::FontHandle handle, const char * utf8, float & minx, float & miny, float & maxx, float & maxy )
	{
		if ( handle > 0 )
		{
			sth_dim_text(internal::_stash, handle, handle, utf8, &minx, &miny, &maxx, &maxy);
		}
	} // dimensions_for_text
	
	unsigned int measure_height( renderer::Font * font, const char * utf8 )
	{
		if ( !font )
		{
			return 0;
		}
		
		float minx = 0, miny = 0, maxx = 0, maxy = 0;
		assert(0);
		//dimensions_for_text( font->font_id, utf8, minx, miny, maxx, maxy );
		return maxy-miny;
	} // measure_height
	
	unsigned int measure_width( renderer::Font * font, const char * utf8 )
	{
		if ( !font )
		{
			return 0;
		}

		float minx = 0, miny = 0, maxx = 0, maxy = 0;
		assert(0);
//		dimensions_for_text( font->font_id, utf8, minx, miny, maxx, maxy );
		return maxx-minx;
	} // measure_width
	
	renderer::FontHandle load_font_from_memory( const void * data, unsigned int data_size, unsigned short point_size )
	{
		assert( internal::_stash != 0 );
		
		renderer::FontHandle result = sth_add_font_from_memory( internal::_stash, (unsigned char*)data );
		if ( result == 0 )
		{
			LOGE( "Unable to load font from memory!\n" );
			return 0;
		}

		return result;
	} // load_font_from_memory

	char * load_font_from_file( const char * path, unsigned short point_size, renderer::FontHandle & handle )
	{
		size_t font_data_size = 0;
		char * font_data = 0;
		font_data = core::filesystem::file_to_buffer( path, 0, &font_data_size );
		
		if ( font_data )
		{
//			LOGV( "font data size: %i bytes\n", font_data_size );
			handle = load_font_from_memory( font_data, font_data_size, point_size );
		}
		else
		{
			LOGE( "Unable to load font from file: '%s'\n", path );
			return 0;
		}
		
		return font_data;
	} // load_font_from_file
}; // namespace font
