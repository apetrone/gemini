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

#include <gemini/typedefs.h>
#include <gemini/core/filesystem.h>

#include <slim/xstr.h>
#include <slim/xlog.h>

#include "font.h"
#include "renderer/renderer.h"
#include "renderer/renderstream.h"
#include "kernel.h"
#include "fontstash.h"



namespace font
{
	const unsigned int FONT_MAX_VERTICES = 1024;

	namespace internal
	{	
		renderer::VertexStream _vertexstream;
		struct sth_stash * _stash;
		assets::Shader * _shader;
	}; // namespace internal
	

	
	unsigned int f_generate_texture( int width, int height, void * pixels )
	{
		renderer::IRenderDriver * driver = renderer::driver();
		renderer::TextureParameters params;
		
		params.image_flags = image::F_ALPHA;
		params.channels = 1;
		params.width = width;
		params.height = height;
		params.pixels = (unsigned char*)pixels;
		params.alignment = 1;
		
		if ( !driver->generate_texture( params ) )
		{
			LOGE( "[font] generate texture failed!\n" );
		}
		
		if ( !driver->upload_texture_2d( params ) )
		{
			LOGE( "[font] upload of 2d texture failed!\n" );
		}
		
		LOGV( "[font] generate texture %i x %i -> %i\n", width, height, params.texture_id );
		return params.texture_id;
	}
	
	void f_delete_texture( unsigned int texture_id )
	{
		renderer::IRenderDriver * driver = renderer::driver();
		renderer::TextureParameters params;
		params.texture_id = texture_id;
		driver->destroy_texture( params );
	}
	
	void f_update_texture(unsigned int texture_id, int origin_x, int origin_y, int width, int height, void * pixels)
	{
		renderer::IRenderDriver * driver = renderer::driver();
		renderer::TextureParameters params;
		
		params.x = origin_x;
		params.y = origin_y;
		params.width = width;
		params.height = height;
		params.alignment = 1;
		params.image_flags = image::F_ALPHA;
		params.pixels = (unsigned char*)pixels;
		params.texture_id = texture_id;
		
		if ( !driver->texture_update( params ) )
		{
			LOGW( "driver texture_update is failing!\n" );
		}
	}

	void f_draw_with_texture(unsigned int texture_id, void * data, int uv_offset, int color_offset, int stride, int vertex_count)
	{
		renderer::TextureParameters params;
		params.texture_id = texture_id;

		renderer::VertexStream & vs = internal::_vertexstream;

		if ( !vs.has_room( vertex_count, 0 ) )
		{
			LOGE( "Unable to draw font: vertexstream has no room! (%i)\n", vs.total_vertices );
		}
		else
		{
			internal::_vertexstream.fill_data( (renderer::VertexType*)data, vertex_count, 0, 0 );
			internal::_vertexstream.update();
			
			assets::Shader * shader = internal::_shader;
			
			glm::mat4 modelview_matrix = glm::mat4(1.0f);
			glm::mat4 projection_matrix;
			
			real w = (real)kernel::instance()->parameters().render_width;
			real h = (real)kernel::instance()->parameters().render_height;
			projection_matrix = glm::ortho(0.0f, w, 0.0f, h, -1.0f, 1.0f );
			
			RenderStream rs;
			
			// activate shader
			rs.add_shader( internal::_shader );
			
			// setup uniforms
			rs.add_uniform_matrix4( shader->get_uniform_location("modelview_matrix"), &modelview_matrix );
			rs.add_uniform_matrix4( shader->get_uniform_location("projection_matrix"), &projection_matrix );
			
			rs.add_sampler2d( shader->get_uniform_location("diffusemap"), 0, texture_id );
			
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
		internal::_shader = assets::shaders()->load_from_path("shaders/fontshader");
		
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
	

	
	void draw_string( assets::Font * font, int x, int y, const char * utf8, const Color & color )
	{
//		int r_width = kernel::instance()->parameters().render_width;
		int r_height = kernel::instance()->parameters().render_height;
		int y_offset = kernel::instance()->parameters().titlebar_height;
		
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
		sth_draw_text( internal::_stash, font->font_id, font->font_size, x, r_height-y-y_offset, vcolor, utf8, &width );
		sth_end_draw( internal::_stash );
		
		// restore state
		rs.rewind();
		rs.add_state( renderer::STATE_BLEND, 0 );
		rs.add_state( renderer::STATE_DEPTH_TEST, 1 );
		rs.run_commands();
	} // draw_string
	
	
	void dimensions_for_text( assets::FontHandle handle, const char * utf8, float & minx, float & miny, float & maxx, float & maxy )
	{
		if ( handle > 0 )
		{
			sth_dim_text(internal::_stash, handle, handle, utf8, &minx, &miny, &maxx, &maxy);
		}
	} // dimensions_for_text
	
	unsigned int measure_height( assets::Font * font, const char * utf8 )
	{
		if ( !font )
		{
			return 0;
		}
		
		float minx = 0, miny = 0, maxx = 0, maxy = 0;
		dimensions_for_text( font->font_id, utf8, minx, miny, maxx, maxy );
		return maxy-miny;
	} // measure_height
	
	unsigned int measure_width( assets::Font * font, const char * utf8 )
	{
		if ( !font )
		{
			return 0;
		}

		float minx = 0, miny = 0, maxx = 0, maxy = 0;
		dimensions_for_text( font->font_id, utf8, minx, miny, maxx, maxy );
		return maxx-minx;
	} // measure_width
	
	assets::FontHandle load_font_from_memory( const void * data, unsigned int data_size, unsigned short point_size )
	{
		assert( internal::_stash != 0 );
		
		assets::FontHandle result = sth_add_font_from_memory( internal::_stash, (unsigned char*)data );
		if ( result == 0 )
		{
			LOGE( "Unable to load font from memory!\n" );
			return 0;
		}

		return result;
	} // load_font_from_memory

	char * load_font_from_file( const char * path, unsigned short point_size, assets::FontHandle & handle )
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
