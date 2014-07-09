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
#include "kernel.h"
#include <stdio.h>
#include <slim/xlog.h>
#include "font.h"
#include "filesystem.h"
#include "renderstream.h"
#include "assets.h"
#include "camera.h"
#include "vertexstream.h"
#include "mathlib.h"

#include "render_utilities.h"

#include "input.h"
#include "gemgl.h"

#define FONT_TEST 1
#define MODEL_TEST 0
#define MODEL_TEST2 0

#define SIMPLE_SHADER 0

#define DRAW_INDEXED 0 // enable this (1) to draw using indices; disable (0) to use draw_arrays

const int TEST_SIZE = 200;




struct TestVertex
{
	glm::vec3 pos;
#if !SIMPLE_SHADER
	renderer::UV uv;
	Color color;	
#endif

};


class TestMobile : public kernel::IApplication,
	kernel::IEventListener<kernel::TouchEvent>,
	kernel::IEventListener<kernel::KeyboardEvent>

{
public:
	DECLARE_APPLICATION( TestMobile );

	assets::Font * test_font;
	RenderStream rs;
	assets::Geometry geo;
	renderer::VertexStream vs;
	assets::Shader shader;
	glm::mat4 objectMatrix;
	renderer::GeneralParameters gp;
	

	virtual void event( kernel::TouchEvent & event )
	{
		if ( event.subtype == kernel::TouchBegin )
		{
			fprintf( stdout, "Touch Event Began at %i, %i\n", event.x, event.y );
		}
		else if ( event.subtype == kernel::TouchMoved )
		{
			fprintf( stdout, "Touch Event Moved at %i, %i\n", event.x, event.y );
		}
		else if ( event.subtype == kernel::TouchEnd )
		{
			fprintf( stdout, "Touch Event Ended at %i, %i\n", event.x, event.y );
		}
	}
	
	virtual void event( kernel::KeyboardEvent & event )
	{
		if (event.is_down)
		{
			if (event.key == input::KEY_ESCAPE)
			{
				kernel::instance()->set_active(false);
			}
		}
	}
	
	virtual kernel::ApplicationResult config( kernel::Params & params )
	{
		params.window_width = 800;
		params.window_height = 600;
		params.window_title = "TestMobile";
		return kernel::Application_Success;
	}
	
	void setup_vertex_data( TestVertex * vertices )
	{
		const int TEST3_OFFSET = 200;
		vertices[0].pos = glm::vec3(TEST3_OFFSET,0, 0);

#if !SIMPLE_SHADER
		vertices[0].color.set( 255, 255, 255 );
		vertices[0].uv.u = 0;
		vertices[0].uv.v = 0;
#endif
		
		vertices[1].pos = glm::vec3(TEST3_OFFSET, TEST_SIZE, 0);

#if !SIMPLE_SHADER
		vertices[1].color.set( 0, 0, 255 );
		vertices[1].uv.u = 0;
		vertices[1].uv.v = 1;
#endif
		
		vertices[2].pos = glm::vec3(TEST3_OFFSET+TEST_SIZE, TEST_SIZE, 0);

#if !SIMPLE_SHADER
		vertices[2].color.set( 0, 255, 0 );
		vertices[2].uv.u = 1;
		vertices[2].uv.v = 1;
#endif
		
//		vertices[3].pos = glm::vec3(TEST3_OFFSET+TEST_SIZE, 0, 0);
//
//#if !SIMPLE_SHADER
//		vertices[3].color.set( 255, 0, 0 );
//		vertices[3].uv.u = 1;
//		vertices[3].uv.v = 0;
//#endif
	}

	virtual kernel::ApplicationResult startup( kernel::Params & params )
	{
#if FONT_TEST		
		LOGV( "loading fonts/nokiafc22.ttf...\n" );
		test_font = assets::fonts()->load_from_path( "fonts/default16" );
		LOGV( "test_font = %i\n", test_font );
#endif



#if MODEL_TEST
		geo.vertex_count = 4;
		geo.vertices = CREATE_ARRAY( glm::vec3, 4 );
		geo.colors = CREATE_ARRAY( Color, 4 );
		geo.uvs = CREATE_ARRAY( renderer::UV, 4 );
		
#if DRAW_INDEXED
		geo.index_count = 6;
		geo.indices = CREATE_ARRAY( renderer::IndexType, 6 );
		geo.draw_type = renderer::DRAW_INDEXED_TRIANGLES;
#else
		geo.index_count = 0;
		geo.indices = 0;
		geo.draw_type = renderer::DRAW_TRIANGLES;
#endif

		const char * material_name = "materials/default";
		geo.material_id = assets::materials()->load_from_path(material_name)->Id();
		LOGV( "material_name = '%s', geo.material_id = %i\n", material_name, geo.material_id );
		
		glm::vec3 * vertices = geo.vertices;
		Color * colors = geo.colors;
		renderer::UV * uvs = geo.uvs;
		vertices[0] = glm::vec3(0,0,0);
		colors[0].set( 255, 255, 255 );
		uvs[0].u = 0;
		uvs[0].v = 0;
		
		vertices[1] = glm::vec3(0, TEST_SIZE, 0);
		colors[1].set( 0, 0, 255, 64 );
		uvs[1].u = 0;
		uvs[1].v = 1;
		
		vertices[2] = glm::vec3(TEST_SIZE, TEST_SIZE, 0);
		colors[2].set( 0, 255, 0, 127 );
		uvs[2].u = 1;
		uvs[2].v = 1;
		
//		vertices[3] = glm::vec3(TEST_SIZE, 0, 0);
//		colors[3].set( 255, 0, 0 );
//		uvs[3].u = 1;
//		uvs[3].v = 0;
		
		
#if DRAW_INDEXED
		renderer::IndexType indices[] = { 0, 1, 2, 2, 3, 0 };
		memcpy( geo.indices, indices, sizeof(renderer::IndexType) * geo.index_count );
#endif



		geo.render_setup();
#endif


#if MODEL_TEST2
		
		vs.desc.add(renderer::VD_FLOAT3);
#if !SIMPLE_SHADER
		vs.desc.add(renderer::VD_FLOAT2);
		vs.desc.add(renderer::VD_UNSIGNED_BYTE4);
#endif
		
		vs.create( 32, 0, renderer::DRAW_TRIANGLES, renderer::BUFFER_STREAM );
#endif
	
		shader.set_frag_data_location( "out_color" );
#if !SIMPLE_SHADER
		shader.alloc_uniforms( 3 );
#else
		shader.alloc_uniforms( 2 );
#endif

		shader.uniforms[0].set_key( "projection_matrix" );
		shader.uniforms[1].set_key( "modelview_matrix" );
#if !SIMPLE_SHADER
		shader.uniforms[2].set_key( "diffusemap" );
#endif
		
#if !SIMPLE_SHADER
		shader.alloc_attributes( 3 );
#else
		shader.alloc_attributes( 1 );
#endif

		shader.attributes[0].set_key( "in_position" ); shader.attributes[0].second = 0;
#if !SIMPLE_SHADER
		shader.attributes[1].set_key( "in_uv" ); shader.attributes[1].second = 1;
		shader.attributes[2].set_key( "in_color" ); shader.attributes[2].second = 2;
#endif
		
		
		
#if !SIMPLE_SHADER
		assets::load_shader( "shaders/fontshader", &shader );
#else
		assets::load_shader( "shaders/simple", &shader );
#endif
			
		return kernel::Application_Success;
	}
	
	virtual void step( kernel::Params & params )
	{
	}
	
	
	void model_test( Camera & camera, kernel::Params & params )
	{
#if MODEL_TEST	
		
		gp.global_params = 0;
		gp.camera_position = &camera.pos;
		gp.modelview_matrix = &camera.matCam;
		gp.projection_project = &camera.matProj;
		gp.object_matrix = &objectMatrix;
		
		render_utilities::stream_geometry(rs, &geo, gp);
#endif
	}
	
	void model_test2( Camera & camera, kernel::Params & params )
	{
#if MODEL_TEST2
		TestVertex * vertices = (TestVertex*)vs.request(3);
		setup_vertex_data( vertices );
		
		
		vs.update();
				
		// activate shader
		rs.add_shader( &shader );
		
		// setup uniforms
		rs.add_uniform_matrix4( shader.get_uniform_location("modelview_matrix"), &camera.matCam );
		rs.add_uniform_matrix4( shader.get_uniform_location("projection_matrix"), &camera.matProj );
				
#if !SIMPLE_SHADER
		assets::Texture * tex = assets::load_texture("textures/default");
		rs.add_sampler2d( shader.get_uniform_location("diffusemap"), 0, tex->texture_id );
#endif
		
		// add draw call for vertexbuffer
		rs.add_draw_call( vs.vertexbuffer );
#endif

		vs.reset();
	} // model_test2
		
	void font_test( Camera & camera, kernel::Params & params )
	{
#if FONT_TEST
		font::draw_string( test_font, 50, 300, "Now is the time for all good men to come to the aid of the party", Color(64,255,192,255) );
#endif
	}

	virtual void tick( kernel::Params & params )
	{
//		LOGV( "--------begin frame--------\n" );
		rs.rewind();
		
		// setup global rendering state
		rs.add_clearcolor( 0.0, 0.0, 0.0, 1.0f );
		rs.add_clear( renderer::CLEAR_COLOR_BUFFER | renderer::CLEAR_DEPTH_BUFFER );
		rs.add_viewport( 0, 0, (int)params.render_width, (int)params.render_height );
		
		
		rs.add_state( renderer::STATE_DEPTH_TEST, 0 );
		
		Camera camera;
//		camera.set_absolute_position( glm::vec3( 0, 1, 5 ) );
		//camera.perspective( 60, params.render_width, params.render_height, 0.1f, 512.0f );
		camera.ortho(0.0f, (float)params.render_width, (float)params.render_height, 0.0f, -1.0f, 1.0f);
//		camera.matProj = glm::ortho( 0.0f, (float)params.render_width, (float)params.render_height, 0.0f, -1.0f, 1.0f );

#if 0
		const glm::mat4 & projection_matrix = camera.matProj;
		LOGV( "proj: %g %g %g %g\n%g %g %g %g\n%g %g %g %g\n%g %g %g %g\n",
			 projection_matrix[0][0], projection_matrix[0][1], projection_matrix[0][2], projection_matrix[0][3],
			 projection_matrix[1][0], projection_matrix[1][1], projection_matrix[1][2], projection_matrix[1][3],
			 projection_matrix[2][0], projection_matrix[2][1], projection_matrix[2][2], projection_matrix[2][3],
			 projection_matrix[3][0], projection_matrix[3][1], projection_matrix[3][2], projection_matrix[3][3]
			 );
#endif

		model_test( camera, params );
		model_test2( camera, params );
		
		// run all commands
		rs.run_commands();

		
		font_test( camera, params );

		vs.reset();
		
//		LOGV( "--------end frame--------\n" );
	}
	
	virtual void shutdown( kernel::Params & params )
	{
#if MODEL_TEST2
		assets::destroy_shader(&shader);
		vs.destroy();
#endif
	}
};

IMPLEMENT_APPLICATION( TestMobile );