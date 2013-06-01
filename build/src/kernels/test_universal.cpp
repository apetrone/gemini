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
#include "typedefs.h"
#include "kernel.hpp"
#include <stdio.h>
#include "renderer.hpp"
#include "audio.hpp"
#include "input.hpp"
#include "log.h"
//#include <squirrel.h>
#include "filesystem.hpp"
#include "hashtable.hpp"

#include "game/menu.hpp"

#include "mathlib.h"
#include "assets.hpp"


#include "configloader.hpp"
#include "camera.hpp"
#include "renderstream.hpp"

#include "keyvalues.hpp"


#include "font.hpp"

#include "render_utilities.hpp"
#include "tiledloader.hpp"

#define TEST_2D 1

glm::mat4 objectMatrix;
glm::vec3 light_position = glm::vec3( 0, 2, 0 );

const float TEST_SIZE = 256;




using namespace kernel;


class TestUniversal : public kernel::IApplication,
	public IEventListener<KeyboardEvent>,
	public IEventListener<MouseEvent>,
	public IEventListener<SystemEvent>,
	public IEventListener<TouchEvent>
{
	struct SpriteVertexType
	{
		float x, y, z;
		Color color;
		float u, v;
	};

	audio::SoundHandle sound;
	audio::SoundSource source;
	renderer::VertexStream vb;
	assets::Texture * tex;
	assets::Mesh * mesh;
	assets::Material * mat, *mat2;
	font::Handle test_font;
	char * font_buffer;
	
	float alpha;
	int alpha_delta;
	Camera camera;

	assets::Shader default_shader;
	unsigned int test_attribs;
	
	RenderStream rs;
	assets::Geometry geo;
	
	
	TiledMap tiled_map;
	
	int tdx;
	int tdy;
public:
	DECLARE_APPLICATION( TestUniversal );

	TestUniversal()
	{
		font_buffer = 0;
	}
	
	virtual void event( kernel::TouchEvent & event )
	{
		if ( event.subtype == kernel::TouchBegin )
		{
//			fprintf( stdout, "Touch Event Began at %i, %i\n", event.x, event.y );
			tdx = event.x;
			tdy = event.y;
		}
		else if ( event.subtype == kernel::TouchMoved )
		{
//			fprintf( stdout, "Touch Event Moved at %i, %i\n", event.x, event.y );
			this->camera.move_view( (event.x - tdx), (event.y - tdy) );
			tdx = event.x;
			tdy = event.y;
		}
		else if ( event.subtype == kernel::TouchEnd )
		{
//			fprintf( stdout, "Touch Event Ended at %i, %i\n", event.x, event.y );
		}
	}
	
	virtual void event( KeyboardEvent & event )
	{
        if ( event.is_down )
        {
            fprintf( stdout, "key %i pressed\n", event.key );
			 
			if ( event.key == input::KEY_Q )
			{
//				audio::stop( source );
			}
        }
        else
        {
            fprintf( stdout, "key %i released\n", event.key );
        }
	}

	virtual void event( MouseEvent & event )
	{
        switch( event.subtype )
        {
            case kernel::MouseMoved:
			{
				if ( input::state()->mouse().is_down( input::MOUSE_LEFT ) )
				{
					int lastx, lasty;
					input::state()->mouse().last_mouse_position( lastx, lasty );
					
					camera.move_view( event.mx-lastx, event.my-lasty );
				}
                break;
			}
            case kernel::MouseButton:
                if ( event.is_down )
                {
                    fprintf( stdout, "mouse button %i is pressed\n", event.button );
                }
                else
                {
                    fprintf( stdout, "mouse button %i is released\n", event.button );
                }
                break;
                
            case kernel::MouseWheelMoved:
                if ( event.wheel_direction > 0 )
                {
                    fprintf( stdout, "mouse wheel toward screen\n" );
                }
                else
                {
                    fprintf( stdout, "mouse wheel away from screen\n" );
                }
                break;
            default:
                fprintf( stdout, "mouse event received!\n" );
                break;
        }

	}

	virtual void event( SystemEvent & event )
	{
		switch( event.subtype )
		{
			case kernel::WindowGainFocus:
				fprintf( stdout, "window gained focus\n" );
				break;
				
			case kernel::WindowLostFocus:
				fprintf( stdout, "window lost focus\n" );
				break;
				
			case kernel::WindowResized:
				fprintf( stdout, "resize event: %i x %i\n", event.window_width, event.window_height );
				break;
				
			default: break;
		}

	}
	
	virtual kernel::ApplicationResult config( kernel::Params & params )
	{
		params.window_width = 800;
		params.window_height = 600;
		params.window_title = "TestUniversal";
//		HSQUIRRELVM vm = sq_open(1024);
//		sq_close( vm );
		return kernel::Application_Success;
	}

	virtual kernel::ApplicationResult startup( kernel::Params & params )
	{
		LOGV( "Window dimensions: %i x %i, Render Viewport: %i x %i\n", params.window_width, params.window_height, params.render_width, params.render_height );
		LOGV( "IndexType is %i bytes.\n", sizeof(renderer::IndexType) );
//		sound = audio::create_sound( "sounds/powerup" );
//		source = audio::play( sound );
		
		int buffer_length = 0;
		font_buffer = fs::file_to_buffer( "fonts/nokiafc22.ttf", 0, &buffer_length );
		if ( font_buffer )
		{
			test_font = font::load_font_from_memory( font_buffer, buffer_length, 16, 0, 72, 72 );
		}
		else
		{
			LOGE( "Unable to load font\n" );
		}


#if 0
		setup_menu();

		foreach_child( _menu.current_menu(), print_option );
		_menu.navigate_to_child( 0 );
		foreach_child( _menu.current_menu(), print_option );
		
		
		_menu.navigate_back();
		foreach_child( _menu.current_menu(), print_option );
				
		_menu.clear_items();
#endif


		assets::ShaderString name("uv0");
		test_attribs = 0;
		test_attribs |= assets::find_parameter_mask( name );
		
		name = "colors";
		test_attribs |= assets::find_parameter_mask( name );
		

		util::json_load_with_callback( "maps/test.json", tiled_map_loader, &tiled_map, true );

//		KeyValues kv;
//		kv.set( "name", 3 );


#if 0
		tex = assets::load_texture( "textures/default" );
		if ( tex )
		{
			LOGV( "loaded texture successfully: %i!\n", tex->texture_id );
		}
		else
		{
			LOGW( "Could not load texture.\n" );
		}
#endif

//		mat = assets::load_material( "materials/rogue" );
//		mat2 = assets::load_material( "materials/gametiles" );

#if TEST_2D
//		camera.ortho( 0.0f, (float)params.render_width, (float)params.render_height, 0.0f, -0.5f, 255.0f );
#else
//		camera.perspective( 60, params.render_width, params.render_height, 0.1f, 512.0f );
#endif

		camera.ortho( 0.0f, (float)params.render_width, (float)params.render_height, 0.0f, -0.5f, 255.0f );

//		camera.set_absolute_position( glm::vec3( 0, 1, 5 ) );
//		camera.move_speed = 100;
		
		alpha = 0;
		alpha_delta = 1;
	
		vb.reset();
		vb.desc.add( renderer::VD_FLOAT3 );
		vb.desc.add( renderer::VD_UNSIGNED_BYTE4 );
		vb.desc.add( renderer::VD_FLOAT2 );
		
		vb.create(256, 1024, renderer::DRAW_INDEXED_TRIANGLES );

#if 1
		geo.vertex_count = 4;
		geo.index_count = 6;
		geo.vertices = CREATE_ARRAY( glm::vec3, 4 );
		geo.indices = CREATE_ARRAY( renderer::IndexType, 6 );
		geo.colors = CREATE_ARRAY( Color, 4 );
		geo.uvs = CREATE_ARRAY( renderer::UV, 4 );
		
		geo.draw_type = renderer::DRAW_INDEXED_TRIANGLES;
		geo.material_id = assets::load_material("materials/default")->Id();
		
		glm::vec3 * vertices = geo.vertices;
		Color * colors = geo.colors;
		renderer::UV * uvs = geo.uvs;
		vertices[0] = glm::vec3(0,0,0);
		colors[0].set( 255, 255, 255 );
		uvs[0].u = 0;
		uvs[0].v = 0;
	
		vertices[1] = glm::vec3(0, TEST_SIZE, 0);
		colors[1].set( 255, 255, 255 );
		uvs[1].u = 0;
		uvs[1].v = 1;
		
		vertices[2] = glm::vec3(TEST_SIZE, TEST_SIZE, 0);
		colors[2].set( 255, 255, 255 );
		uvs[2].u = 1;
		uvs[2].v = 1;

		vertices[3] = glm::vec3(TEST_SIZE, 0, 0);
		colors[3].set( 255, 255, 255 );
		uvs[3].u = 1;
		uvs[3].v = 0;
		
		renderer::IndexType indices[] = { 0, 1, 2, 2, 3, 0 };
		memcpy( geo.indices, indices, sizeof(renderer::IndexType) * geo.index_count );
		
		
		geo.render_setup();
#endif
//		assets::load_test_shader(&this->default_shader);

		mesh = 0;
#if 0
		// test mesh loading
		mesh = assets::load_mesh( "models/plasma3" );
		if ( mesh )
		{
			mesh->prepare_geometry();
		}
		else
		{
			LOGW( "unable to load mesh.\n" );
		}
#endif
		return kernel::Application_Success;
	}

	
	virtual void step( kernel::Params & params )
	{
		if ( input::state()->keyboard().is_down( input::KEY_W ) )
		{
			camera.move_forward( params.step_interval_seconds );
		}
		else if ( input::state()->keyboard().is_down( input::KEY_S ) )
		{
			camera.move_backward( params.step_interval_seconds );
		}
		
		if ( input::state()->keyboard().is_down( input::KEY_A ) )
		{
			camera.move_left( params.step_interval_seconds );
		}
		else if ( input::state()->keyboard().is_down( input::KEY_D ) )
		{
			camera.move_right( params.step_interval_seconds );
		}
	
		alpha += ((float)alpha_delta) * 0.025f;
		
		if ( alpha >= 1.0 || alpha <= 0 )
		{
			if ( alpha >= 1.0 )
			{
				alpha = 1.0f;
			}
			else
			{
				alpha = 0;
			}
			alpha_delta = -alpha_delta;
			
		}
		
		light_position.x = cosf( alpha ) * 4;
		light_position.z = sinf( alpha ) * 4;		
	}

	virtual void tick( kernel::Params & params )
	{
		rs.rewind();
		
		// setup global rendering state
		rs.add_clearcolor( 0.15, 0.10, 0.25, 1.0f );
		rs.add_clear( 0x00004000 | 0x00000100 );
		rs.add_viewport( 0, 0, (int)params.render_width, (int)params.render_height );

//		rs.add_state( renderer::STATE_DEPTH_TEST, 1 );
		rs.run_commands();
		rs.rewind();


		


		renderer::GeneralParameters gp;
		assets::ShaderString lightposition = "lightposition";
		gp.global_params = 0; //assets::find_parameter_mask( lightposition );
		gp.camera_position = &camera.pos;
		gp.modelview_matrix = &camera.matCam;
		gp.projection_project = &camera.matProj;
		gp.object_matrix = &objectMatrix;
		
//		stream_geometry( rs, &geo, gp );
		
		
#if TEST_2D
		//			rs.add_state( renderer::STATE_BLEND, 1 );
		//			rs.add_blendfunc( renderer::BLEND_SRC_ALPHA, renderer::BLEND_ONE_MINUS_SRC_ALPHA );
		
		// could potentially have a vertexbuffer per tileset
		// this would allow us to batch tiles that share the same tileset (texture)
		TileSet * lastset = 0;
		
		for( unsigned int layer_num = 0; layer_num < 1/*tiled_map.layer_count*/; ++layer_num )
		{
			for( int h = 0; h < tiled_map.height; ++h )
			{
				for( int w = 0; w < tiled_map.width; ++w )
				{
					unsigned char tile_gid = tiled_map.layers[ layer_num ].layer_data[ h * tiled_map.width + w ];
					if ( tile_gid > 0 )
					{
						Tile * tile = &tiled_map.tilelist.tiles[ tile_gid-1 ];
						if ( tile )
						{
							TileSet * set = &tiled_map.tilesets[ tile->tileset_id ];
							
							if ( (!vb.has_room(4, 6) || (set != lastset)) && lastset != 0 )
							{
								long offset = rs.stream.offset_pointer();
								vb.update();
								assets::Shader * shader = assets::find_compatible_shader( test_attribs + lastset->material->requirements );
								rs.add_shader( shader );
								
								rs.add_uniform_matrix4( shader->get_uniform_location("modelview_matrix"), &camera.matCam );
								rs.add_uniform_matrix4( shader->get_uniform_location("projection_matrix"), &camera.matProj );
								rs.add_uniform_matrix4( shader->get_uniform_location("object_matrix"), &objectMatrix );
								
								
								rs.add_material( lastset->material, shader );
								rs.add_draw_call( vb.vertexbuffer );
								
								rs.run_commands();
								vb.reset();
								rs.stream.seek( offset, true );
								
							}
							
							SpriteVertexType * v = (SpriteVertexType*)vb.request(4);
							if ( v )
							{
								lastset = set;
								//						SpriteVertexType * v = (SpriteVertexType*)vb[0];
								
								int x = w * tiled_map.tile_width;
								int y = h * tiled_map.tile_height;
								v[0].x = x;
								v[0].y = y;
								v[0].z = 0;
								v[0].color = Color(255,255,255);
								
								v[1].x = x;
								v[1].y = y+tiled_map.tile_height;
								v[1].z = 0;
								v[1].color = Color(255,255,255);
								
								v[2].x = x+tiled_map.tile_width;
								v[2].y = y+tiled_map.tile_height;
								v[2].z = 0;
								v[2].color = Color(255,255,255);
								
								v[3].x = x+tiled_map.tile_width;
								v[3].y = y;
								v[3].z = 0;
								v[3].color = Color(255,255,255);
								
								v[0].u = tile->quad_uvs[0];
								v[0].v = tile->quad_uvs[1];
								v[1].u = tile->quad_uvs[2];
								v[1].v = tile->quad_uvs[3];
								v[2].u = tile->quad_uvs[4];
								v[2].v = tile->quad_uvs[5];
								v[3].u = tile->quad_uvs[6];
								v[3].v = tile->quad_uvs[7];
								
								//								LOGV( "[%g %g, %g %g, %g %g, %g %g\n", v[0].x, v[0].y, v[1].x, v[1].y, v[2].x, v[2].y, v[3].x, v[3].y );
								
								renderer::IndexType indices[] = { 0, 1, 2, 2, 3, 0 };
								vb.append_indices( indices, 6 );
							}
						} // tile
					} // tile gid > 0
				} // for width
			} // for height
		} // for each layer

		if ( vb.last_index > 0 && lastset )
		{
			vb.update();
			assets::Shader * shader = assets::find_compatible_shader( test_attribs + lastset->material->requirements );
//			LOGV( "shader: %i, draw tileset: %i, count: %i\n", shader->id, lastset->id, vb.last_index );
			
			rs.add_shader( shader );
			rs.add_uniform_matrix4( shader->get_uniform_location("modelview_matrix"), &camera.matCam );
			rs.add_uniform_matrix4( shader->get_uniform_location("projection_matrix"), &camera.matProj );
			rs.add_uniform_matrix4( shader->get_uniform_location("object_matrix"), &objectMatrix );
			
			rs.add_material( lastset->material, shader );
			rs.add_draw_call( vb.vertexbuffer );
			
			rs.run_commands();
		}

		vb.reset();
		
#else
		// rotate the model
//		objectMatrix = glm::rotate( objectMatrix, 0.5f, glm::vec3( 0, 1, 0) );
		
		if ( mesh )
		{
			for( unsigned int geo_id = 0; geo_id < mesh->total_geometry; ++geo_id )
			{
				assets::Geometry * g = &mesh->geometry[ geo_id ];
				render_utilities::stream_geometry( rs, g, gp );
			}
		}

#endif
		rs.run_commands();

		font::draw_string( test_font, 50, 50, "Now is the time for all good men to come to the aid of the party", Color(255,0,0,255) );
//		font::draw_string( test_font, 50, 75, "Ја могу да једем стакло", Color(255, 255, 255, 255) );
//		font::draw_string( test_font, 50, 100, "私はガラスを食べられます。それは私を傷つけません。", Color(0, 128, 255, 255) );
	}

	virtual void shutdown( kernel::Params & params )
	{
		DEALLOC( font_buffer );
		
		if ( renderer::driver() )
		{
			renderer::driver()->shaderprogram_destroy( this->default_shader );
		}
		
		vb.destroy();
	}
};

IMPLEMENT_APPLICATION( TestUniversal );



// experimental
#if 0


void foreach_child( MenuItem * root, foreach_menu_callback callback )
{
	MenuItemVector::iterator it, end;
	it = root->children.begin();
	end = root->children.end();
	
	for( ; it != end; ++it )
	{
		MenuItem * option = (*it);
		callback( option );
	}
}

MenuNavigator _menu;

void setup_menu()
{
	MenuItem * root = _menu.root_menu();
	root->name = "Game";
	
	MenuItem * newgame = root->add_child( "New Game" );
	newgame->add_child( "Easy" );
	newgame->add_child( "Medium" );
	newgame->add_child( "Hard" );
	
	root->add_child( "Load Game" );
	root->add_child( "Options" );
	root->add_child( "Quit" );
}

void print_option( MenuItem * child )
{
	LOGV( "[ %s ]\n", child->name );
}


void print_options( MenuItem * root )
{
	LOGV( "[ %s ]\n", root->name );
	LOGV( "options:\n" );
	
	MenuItemVector::iterator it, end;
	it = root->children.begin();
	end = root->children.end();
	
	for( ; it != end; ++it )
	{
		MenuItem * option = (*it);
		LOGV( "-> %s\n", option->name );
	}
}
#endif