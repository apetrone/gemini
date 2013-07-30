// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone

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
#include "render_utilities.hpp"
#include <slim/xlog.h>


#include "assets.hpp"
#include "renderer.hpp"
#include "renderstream.hpp"

namespace render_utilities
{
	void stream_geometry( RenderStream & rs, assets::Geometry * geo, renderer::GeneralParameters & gp )
	{
		assert( geo != 0 );
		assets::Material * material = assets::materials()->find_with_id( geo->material_id );
		assert( material != 0 );
		//		LOGV( "material: %i\n", material->Id() );
		assets::Shader * shader = assets::find_compatible_shader( geo->attributes + material->requirements + gp.global_params );
		assert( shader != 0 );
		
		if ( !shader )
		{
			LOGE( "Unable to find shader!\n" );
			return;
		}
		
		//		LOGV( "binding shader: %i\n", shader->id );
		rs.add_shader( shader );
		
		if ( gp.global_params > 0 )
		{
			//		rs.add_uniform3f( shader->get_uniform_location("lightPosition"), &light_position );
			//		rs.add_uniform3f( shader->get_uniform_location("cameraPosition"), gp.camera_position );
		}
		
		rs.add_uniform_matrix4( shader->get_uniform_location("modelview_matrix"), gp.modelview_matrix );
		rs.add_uniform_matrix4( shader->get_uniform_location("projection_matrix"), gp.projection_project );
		rs.add_uniform_matrix4( shader->get_uniform_location("object_matrix"), gp.object_matrix );
		
		rs.add_material( material, shader );
		
		rs.add_draw_call( geo->vertexbuffer );
	} // stream_geometry
	
	//
	// misc sprite tools
	namespace sprite
	{
		void calc_tile_uvs( float * uvs, unsigned int x, unsigned int y, unsigned int sprite_width, unsigned int sprite_height, unsigned int sheet_width, unsigned int sheet_height )
		{
			// This assumes an Orthographic projection set with the origin in the upper left
			// upper left
			uvs[0] = x / (float)sheet_width;
			uvs[1] = y / (float)sheet_height;
			
			// lower left
			uvs[2] = x / (float)sheet_width;
			uvs[3] = (y+sprite_height) / (float)sheet_height;
			
			// lower right
			uvs[4] = (x+sprite_width) / (float)sheet_width;
			uvs[5] = (y+sprite_height) / (float)sheet_height;
			
			// upper right
			uvs[6] = (x+sprite_width) / (float)sheet_width;
			uvs[7] = y / (float)sheet_height;
		} // calc_tile_uvs
	}; // sprite
}; // mamespace render_utilities