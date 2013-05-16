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
#include "typedefs.h"
#include "xstr.h"
#include "log.h"
#include "filesystem.hpp"
//#include "memorystream.hpp"
#include "font.hpp"
#include "renderer.hpp"
#include "renderstream.hpp"
#include <stdlib.h>
#include "kernel.hpp"

#include "fontstash.h"



namespace font
{
	const unsigned int FONT_MAX_VERTICES = 8192;
	const unsigned int FONT_MAX_INDICES = 6172;
	const unsigned int FONT_MAX = 4;
	
	struct SimpleFontHandle
	{
//		bool noaa;
		unsigned short font_size;
	}; // SimpleFontHandle

	namespace internal
	{	
		renderer::VertexStream _vertexstream;
		struct sth_stash * _stash;
		assets::Shader * _shader;
		
		SimpleFontHandle _fonts[ FONT_MAX ];
		
		
		
		SimpleFontHandle * handle_by_id( font::Handle id )
		{
			if ( id >= (FONT_MAX-1) )
			{
				return 0;
			}
			
			return &internal::_fonts[id];
		} // handle_by_id
		
		SimpleFontHandle * find_unused_handle()
		{
			for( unsigned int i = 0; i < FONT_MAX; ++i )
			{
				if (_fonts[i].font_size == 0)
				{
					return &_fonts[i];
				}
			}
			
			return 0;
		} // find_unused_handle
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
		
		driver->generate_texture( params );
		driver->upload_texture_2d( params );
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
		
		if ( !vs.has_room( 128, 0 ) )
		{
			LOGE( "Unable to draw font: vertexstream has no room!\n" );
		}
		else
		{		
			internal::_vertexstream.fill_data( (renderer::VertexType*)data, vertex_count, 0, 0 );
			internal::_vertexstream.update();
			
			assets::Shader * shader = internal::_shader;
			
			glm::mat4 modelview_matrix;
			glm::mat4 projection_matrix;
			projection_matrix = glm::ortho(0.f, (float)kernel::instance()->parameters().render_width, 0.f, (float)kernel::instance()->parameters().render_height, -1.0f, 1.0f );
			
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
		}
	}


	void startup()
	{
		SimpleFontHandle * handle;
		for( unsigned int i = 0; i < FONT_MAX; ++i )
		{
			handle = &internal::_fonts[i];
			handle->font_size = 0;
		}
	
		// initialize the vertex stream
		internal::_vertexstream.desc.add( renderer::VD_FLOAT2 );
		internal::_vertexstream.desc.add( renderer::VD_FLOAT2 );
		internal::_vertexstream.desc.add( renderer::VD_UNSIGNED_BYTE4 );

		
		internal::_vertexstream.create( FONT_MAX_VERTICES, FONT_MAX_INDICES, renderer::DRAW_TRIANGLES, renderer::BUFFER_STREAM );
		
		
		sth_render_callbacks cb;
		cb.generate_texture = f_generate_texture;
		cb.update_texture = f_update_texture;
		cb.delete_texture = f_delete_texture;
		
		
		cb.draw_with_texture = f_draw_with_texture;
		
		
		// this must be called before any sth_create commands
		sth_set_render_callbacks( &cb );
		
		internal::_stash = sth_create( 512, 512 );


		internal::_shader = CREATE( assets::Shader );

		assets::Shader * shader = internal::_shader;
		shader->set_frag_data_location( "out_color" );
		shader->alloc_uniforms( 3 );
		shader->uniforms[0].set_key( "projection_matrix" );
		shader->uniforms[1].set_key( "modelview_matrix" );
		shader->uniforms[2].set_key( "diffusemap" );
		
		shader->alloc_attributes( 3 );
		shader->attributes[0].set_key( "in_position" ); shader->attributes[0].second = 0;
		shader->attributes[1].set_key( "in_uv" ); shader->attributes[1].second = 1;
		shader->attributes[2].set_key( "in_color" ); shader->attributes[2].second = 2;
		
		assets::load_shader( "shaders/fontshader", internal::_shader );
				
	} // startup

	void shutdown()
	{
		assets::destroy_shader( internal::_shader );
		
		DESTROY( Shader, internal::_shader );
	
		// cleanup used memory here
		internal::_vertexstream.destroy();
		
		// delete internal font stash
		sth_delete( internal::_stash );
	} // shutdown
	

	
	void draw_string( font::Handle fontid, int x, int y, const char * utf8, const Color & color )
	{
//		int r_width = kernel::instance()->parameters().render_width;
		int r_height = kernel::instance()->parameters().render_height;
	
		SimpleFontHandle * handle = internal::handle_by_id( fontid );
		if ( !handle )
		{
			return;
		}
	
		RenderStream rs;

		// setup global rendering state
		rs.add_state( renderer::STATE_DEPTH_TEST, 0 );
		rs.add_state( renderer::STATE_BLEND, 1 );
		rs.add_blendfunc( renderer::BLEND_SRC_ALPHA, renderer::BLEND_ONE_MINUS_SRC_ALPHA );
		rs.run_commands();
		
		
		// draw characters
		// ...
		sth_begin_draw( internal::_stash );

		float width = 0;
		unsigned int vcolor = STH_RGBA(color.r, color.g, color.b, color.a);
		sth_draw_text( internal::_stash, fontid, handle->font_size, x, r_height-y, vcolor, utf8, &width );


		sth_end_draw( internal::_stash );
		
		// restore state
		rs.add_state( renderer::STATE_BLEND, 0 );
		rs.rewind();
		
		rs.run_commands();
	} // draw_string
	
	unsigned int measure_height( font::Handle fontid )
	{
		return 0;
	} // measure_height
	
	unsigned int measure_width( font::Handle fontid, const char * str )
	{
		return 0;
	} // measure_width
	
	font::Handle load_font_from_memory( const void * data, unsigned int data_size, unsigned short point_size, bool antialiased, unsigned int hres, unsigned int vres )
	{
		int result = sth_add_font_from_memory( internal::_stash, (unsigned char*)data );
		if ( result == 0 )
		{
			LOGE( "Unable to load font from memory!\n" );
		}
		
		SimpleFontHandle * handle = internal::handle_by_id( result );
		handle->font_size = (unsigned short)point_size;
		if (kernel::instance()->parameters().device_flags & kernel::DeviceSupportsRetinaDisplay)
		{
			handle->font_size = handle->font_size * 2;
		}
				
		return font::Handle(result);
	} // load_font_from_memory

}; // namespace font
