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


#include "fontstash.h"



namespace font
{
	const unsigned int FONT_MAX_VERTICES = 8192;
	const unsigned int FONT_MAX_INDICES = 6172;
	
	struct SimpleFontHandle
	{
		bool noaa;

	}; // SimpleFontHandle

	namespace internal
	{	
		renderer::VertexStream _vertexstream;
		
		
		struct sth_stash * _stash;
		
		assets::Shader * _shader;
	}; // namespace internal
	
	
	
	
	
	
	unsigned int f_generate_texture( int width, int height, void * pixels )
	{
#if 0
		glGenTextures(1, &texture->id);
		
		glBindTexture(GL_TEXTURE_2D, texture->id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, stash->tw,stash->th, 0, GL_ALPHA, GL_UNSIGNED_BYTE, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#endif
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
#if 0
		glDeleteTextures(1, &curtex->id);
#endif
	}
	
	void f_update_texture(unsigned int texture_id, int origin_x, int origin_y, int width, int height, void * pixels)
	{
#if 0
		glBindTexture(GL_TEXTURE_2D, texture_id);
		glPixelStorei(GL_UNPACK_ALIGNMENT,1);
		glTexSubImage2D(GL_TEXTURE_2D, 0, origin_x,origin_y, width, height, GL_ALPHA,GL_UNSIGNED_BYTE, pixels);
#endif
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
		
		driver->texture_update( params );
	}

	void f_draw_with_texture(unsigned int texture_id, void * data, int uv_offset, int color_offset, int stride, int vertex_count)
	{
#if 0
		glBindTexture(GL_TEXTURE_2D, texture->id);
		glEnable(GL_TEXTURE_2D);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glVertexPointer(2, GL_FLOAT, VERT_STRIDE, texture->verts);
		glTexCoordPointer(2, GL_FLOAT, VERT_STRIDE, texture->verts+2);
		glDrawArrays(GL_TRIANGLES, 0, texture->nverts);
		glDisable(GL_TEXTURE_2D);
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#endif
		
		renderer::TextureParameters params;
		params.texture_id = texture_id;
		
//		renderer::IRenderDriver * driver = renderer::driver();
		
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
			projection_matrix = glm::ortho(0.f, 800.f, 0.f, 600.f, -1.0f, 1.0f );
			
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
		sth_draw_text( internal::_stash, fontid, 16.0f, x, y, STH_RGBA(255,0,0,255), utf8, &width );


		sth_end_draw( internal::_stash );
		
		// restore state
		rs.rewind();
		rs.add_state( renderer::STATE_BLEND, 0 );
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
	
	font::Handle load_font_from_memory( const void * data, unsigned int data_size, unsigned int point_size, bool antialiased, unsigned int hres, unsigned int vres )
	{
		int result = sth_add_font_from_memory( internal::_stash, (unsigned char*)data );
		if ( result == 0 )
		{
			LOGE( "Unable to load font from memory!\n" );
			
		}
				
		return font::Handle(result);
	} // load_font_from_memory

}; // namespace font
