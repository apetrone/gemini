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
#include "kernel.hpp"
#include <stdio.h>
#include "log.h"
#include "font.hpp"
#include "filesystem.hpp"
#include "renderstream.hpp"
#include "assets.hpp"
#include "camera.hpp"
#include "vertexstream.hpp"
#include "mathlib.h"


#define FONT_TEST 0
#define MODEL_TEST 0
#define MODEL_TEST2 1

#define DRAW_INDEXED 0 // enable this (1) to draw using indices; disable (0) to use draw_arrays

const int TEST_SIZE = 200;


struct GeneralParameters
{
	glm::mat4 * modelview_matrix;
	glm::mat4 * projection_project;
	glm::mat4 * object_matrix;
	
	unsigned int global_params;
	glm::vec3 * camera_position;
	
	
	
};

static void stream_geometry( RenderStream & rs, assets::Geometry * geo, GeneralParameters & gp )
{
	assert( geo != 0 );
	assets::Material * material = assets::material_by_id( geo->material_id );
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


struct TestVertex
{
	glm::vec2 pos;
	renderer::UV uv;
	Color color;
};

class TestMobile : public kernel::IApplication,
	kernel::IEventListener<kernel::TouchEvent>

{
public:
	DECLARE_APPLICATION( TestMobile );

	font::Handle test_font;
	RenderStream rs;
	char * font_buffer;
	assets::Geometry geo;
	renderer::VertexStream vs;
	assets::Shader shader;
	glm::mat4 objectMatrix;
	GeneralParameters gp;
	
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
	
	virtual kernel::ApplicationResult config( kernel::Params & params )
	{
		params.window_width = 800;
		params.window_height = 600;
		params.window_title = "TestMobile";
		return kernel::Success;
	}

	virtual kernel::ApplicationResult startup( kernel::Params & params )
	{
#if FONT_TEST
		font::startup();
		
		int buffer_length = 0;
		font_buffer = fs::file_to_buffer( "fonts/nokiafc22.ttf", 0, &buffer_length );
		if ( font_buffer )
		{
			test_font = font::load_font_from_memory( font_buffer, buffer_length, 16, 0, 72, 72 );
			LOGV( "font loaded: %i bytes\n", buffer_length );
		}
		else
		{
			LOGE( "Unable to load font\n" );
		}
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
		geo.material_id = assets::load_material(material_name)->Id();
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
		
		vertices[3] = glm::vec3(TEST_SIZE, 0, 0);
		colors[3].set( 255, 0, 0 );
		uvs[3].u = 1;
		uvs[3].v = 0;
		
		
#if DRAW_INDEXED
		renderer::IndexType indices[] = { 0, 1, 2, 2, 3, 0 };
		memcpy( geo.indices, indices, sizeof(renderer::IndexType) * geo.index_count );
#endif
		geo.render_setup();
#endif


#if MODEL_TEST2
		
		vs.desc.add(renderer::VD_FLOAT2);
		vs.desc.add(renderer::VD_FLOAT2);
		vs.desc.add(renderer::VD_UNSIGNED_BYTE4);
		vs.create( 32, 0, renderer::DRAW_TRIANGLES, renderer::BUFFER_STREAM );


		shader.set_frag_data_location( "out_color" );
		shader.alloc_uniforms( 3 );
		shader.uniforms[0].set_key( "projection_matrix" );
		shader.uniforms[1].set_key( "modelview_matrix" );
		shader.uniforms[2].set_key( "diffusemap" );
		
		shader.alloc_attributes( 3 );
		shader.attributes[0].set_key( "in_position" ); shader.attributes[0].second = 0;
		shader.attributes[1].set_key( "in_uv" ); shader.attributes[1].second = 1;
		shader.attributes[2].set_key( "in_color" ); shader.attributes[2].second = 2;
		
		assets::load_shader( "shaders/fontshader", &shader );


#endif
	
		return kernel::Success;
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
		
		stream_geometry(rs, &geo, gp);
#endif
	}
	
	void model_test2( Camera & camera, kernel::Params & params )
	{
#if MODEL_TEST2
		TestVertex * vertices = (TestVertex*)vs.request(4);
		vertices[0].pos = glm::vec2(0,0);
		vertices[0].color.set( 255, 255, 255 );
		vertices[0].uv.u = 0;
		vertices[0].uv.v = 0;
		
		vertices[1].pos = glm::vec2(0, TEST_SIZE);
		vertices[1].color.set( 0, 0, 255 );
		vertices[1].uv.u = 0;
		vertices[1].uv.v = 1;
		
		vertices[2].pos = glm::vec2(TEST_SIZE, TEST_SIZE);
		vertices[2].color.set( 0, 255, 0 );
		vertices[2].uv.u = 1;
		vertices[2].uv.v = 1;
		
		vertices[3].pos = glm::vec2(TEST_SIZE, 0);
		vertices[3].color.set( 255, 0, 0 );
		vertices[3].uv.u = 1;
		vertices[3].uv.v = 0;
		
		
		vs.update();
				
		// activate shader
		rs.add_shader( &shader );
		
		// setup uniforms
		rs.add_uniform_matrix4( shader.get_uniform_location("modelview_matrix"), &camera.matCam );
		rs.add_uniform_matrix4( shader.get_uniform_location("projection_matrix"), &camera.matProj );
		
		assets::Texture * tex = assets::load_texture("textures/default");
		rs.add_sampler2d( shader.get_uniform_location("diffusemap"), 0, tex->texture_id );
		
		// add draw call for vertexbuffer
		rs.add_draw_call( vs.vertexbuffer );
#endif
	}
	
	void font_test( Camera & camera, kernel::Params & params )
	{
#if FONT_TEST
		font::draw_string( test_font, 50, 50, "Now is the time for all good men to come to the aid of the party", Color(64,255,192,255) );
#endif
	}

	virtual void tick( kernel::Params & params )
	{
//		LOGV( "--------begin frame--------\n" );
		rs.rewind();
		
		// setup global rendering state
		rs.add_clearcolor( 0.15, 0.10, 0.25, 1.0f );
		rs.add_clear( renderer::CLEAR_COLOR_BUFFER | renderer::CLEAR_DEPTH_BUFFER );
		rs.add_viewport( 0, 0, (int)params.render_width, (int)params.render_height );
			
		
		Camera camera;
//		camera.set_absolute_position( glm::vec3( 0, 1, 5 ) );
		//camera.perspective( 60, params.render_width, params.render_height, 0.1f, 512.0f );
		camera.ortho(0, (int)params.render_width, (int)params.render_height, 0, -1.0f, 1024.0f);


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
#if FONT_TEST
		DEALLOC(font_buffer);
		font::shutdown();
#endif


#if MODEL_TEST2
		assets::destroy_shader(&shader);
		vs.destroy();
#endif
	}
};

IMPLEMENT_APPLICATION( TestMobile );