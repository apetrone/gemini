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
#include "game/menu.hpp"
#include "mathlib.h"
#include "assets.hpp"
#include "camera.hpp"
#include "renderstream.hpp"
#include "font.hpp"
#include "render_utilities.hpp"
#include "tiledloader.hpp"
#include "screencontrol.hpp"
#include "helpscreen.hpp"
#include "logoscreen.hpp"
#include "gamescreen.hpp"

#include "keyframechannel.hpp"
#include "debugdraw.hpp"
#include "util.hpp"
#include "particlesystem.hpp"
#include "engine.hpp"

#define TEST_2D 0
#define TEST_FONT 1

glm::mat4 objectMatrix;


void add_sprite_to_stream( renderer::VertexStream & vb, int x, int y, int width, int height, const Color & color, float * texcoords );

void world_to_screen( float & wx, float & wy, float & sx, float & sy )
{
	sx = wx;
	sy = wy;
} // world_to_screen

void world_to_screen( glm::vec2 & world_position, glm::vec2 & screen )
{
	screen = world_position;
} // world_to_screen











void render_particles( ParticleSystem & ps, renderer::IRenderDriver * driver, glm::mat4 & modelview_matrix, glm::mat4 & projection_matrix )
{
	renderer::VertexStream stream;
	
	stream.desc.add( renderer::VD_FLOAT3 );
	stream.desc.add( renderer::VD_UNSIGNED_BYTE4 );
	stream.desc.add( renderer::VD_FLOAT2 );
	
	stream.create(1024, 1024, renderer::DRAW_INDEXED_TRIANGLES);
	ParticleEmitter * emitter = 0;
	
	unsigned int indices[] = { 0, 1, 2, 2, 3, 0 };
	glm::mat3 billboard = glm::transpose( glm::mat3(modelview_matrix) );
	
	ParticleEmitterVector::iterator iter = ps.emitters.begin();
	ParticleEmitterVector::iterator end = ps.emitters.end();
	
	for( ; iter != end; ++iter )
	{
		emitter = (*iter);
		emitter->world_position.interpolate( kernel::instance()->parameters().step_alpha );
		for( unsigned int p = 0; p < emitter->max_particles; ++p )
		{
			Particle * particle = &emitter->particle_list[ p ];
			if ( particle->life_remaining > 0 )
			{
				//
				particle->position.interpolate( kernel::instance()->parameters().step_alpha );
				if ( !stream.has_room(4, 6) )
				{
					LOGE( "particle stream is full - flush!\n" );
				}

				SpriteVertexType * v = (SpriteVertexType*)stream.request(4);
				glm::vec3 offset( particle->size, particle->size, 0 );
				glm::vec3 temp;
				
				temp = (billboard * -offset) + particle->position.render;
				v[0].x = temp.x;
				v[0].y = temp.y;
				v[0].z = temp.z;
				
				temp = (billboard * glm::vec3(offset[0], -offset[1], -offset[2])) + particle->position.render;
				v[1].x = temp.x;
				v[1].y = temp.y;
				v[1].z = temp.z;

				temp = (billboard * offset) + particle->position.render;
				v[2].x = temp.x;
				v[2].y = temp.y;
				v[2].z = temp.z;
				
				temp = (billboard * glm::vec3(-offset[0], offset[1], -offset[2])) + particle->position.render;
				v[3].x = temp.x;
				v[3].y = temp.y;
				v[3].z = temp.z;
				
				v[0].color = v[1].color = v[2].color = v[3].color = particle->color;
				
				v[0].u = 0; v[0].v = 0;
				v[1].u = 1; v[1].v = 0;
				v[2].u = 1; v[2].v = 1;
				v[3].u = 0; v[3].v = 1;
				
//				debugdraw::point(particle->position.render, Color(255,255,255), particle->size, 0.0f);
				stream.append_indices( indices, 6 );
			}
		}
	}
	
	RenderStream rs;
	
	assets::ShaderString name("uv0");
	unsigned int test_attribs = assets::find_parameter_mask( name );
	
	name = "colors";
	test_attribs |= assets::find_parameter_mask(name);

	assets::Material * material = 0;
	assets::Shader * shader = 0;
	emitter = ps.emitters[0];
	
	if ( emitter )
	{
		material = assets::material_by_id(emitter->material_id);
		shader = assets::find_compatible_shader( material->requirements + test_attribs );
	}
	
	glm::mat4 object_matrix;
	
	stream.update();
	
	rs.add_blendfunc(renderer::BLEND_SRC_ALPHA, renderer::BLEND_ONE_MINUS_SRC_ALPHA);
	rs.add_state(renderer::STATE_BLEND, 1);
	rs.add_shader( shader );
	
	rs.add_state(renderer::STATE_DEPTH_TEST, 1);
	rs.add_state(renderer::STATE_DEPTH_WRITE, 0);
	
	rs.add_uniform_matrix4( shader->get_uniform_location("modelview_matrix"), &modelview_matrix );
	rs.add_uniform_matrix4( shader->get_uniform_location("projection_matrix"), &projection_matrix );
	rs.add_uniform_matrix4( shader->get_uniform_location("object_matrix"), &object_matrix );
	
	rs.add_material( material, shader );
	
	rs.add_draw_call( stream.vertexbuffer );
	
	
	rs.add_state(renderer::STATE_BLEND, 0);
	rs.add_state(renderer::STATE_DEPTH_WRITE, 1);
	rs.run_commands();
	
	stream.destroy();
} // render_particles


util::ConfigLoadStatus load_sprite_from_file( const Json::Value & root, void * data );

void frame_to_pixels( unsigned short frame, assets::Texture * texture, unsigned int sprite_width, unsigned int sprite_height, unsigned int & x, unsigned int & y )
{
	unsigned short cols = (texture->width / sprite_width);
	unsigned short rows = (texture->height / sprite_height);
	
	x = (frame % cols) * sprite_width;
	y = (frame / rows) * sprite_height;
} // frame_to_pixels







struct MovementCommand
{
	float up, down, left, right;
}; // MovementCommand



using namespace kernel;

class TestUniversal : public kernel::IApplication,
	public IEventListener<KeyboardEvent>,
	public IEventListener<MouseEvent>,
	public IEventListener<SystemEvent>,
	public IEventListener<TouchEvent>
{
	RenderStream rs;

	int tdx;
	int tdy;

	
public:
	DECLARE_APPLICATION( TestUniversal );

	TestUniversal()
	{
		engine::startup();
	}
	
	
	~TestUniversal()
	{
		engine::shutdown();
	}
	
	virtual void event( kernel::TouchEvent & event )
	{
		IScreen * screen = engine::engine()->screen_controller()->active_screen();
		if ( screen )
		{
			screen->on_event( event, this );
		}
#if 0
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
#endif
	}
	
	virtual void event( KeyboardEvent & event )
	{
		IScreen * screen = engine::engine()->screen_controller()->active_screen();
		if ( screen )
		{
			screen->on_event( event, this );
		}
		
#if 0
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
#endif
	}

	virtual void event( MouseEvent & event )
	{
		IScreen * screen = engine::engine()->screen_controller()->active_screen();
		if ( screen )
		{
			screen->on_event( event, this );
		}
#if 0
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
#endif

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
		params.window_width = 720;
		params.window_height = 480;
		params.window_title = "TestUniversal";

		return kernel::Application_Success;
	}

	virtual kernel::ApplicationResult startup( kernel::Params & params )
	{
		LOGV( "Window dimensions: %i x %i, Render Viewport: %i x %i\n", params.window_width, params.window_height, params.render_width, params.render_height );
		LOGV( "IndexType is %i bytes.\n", sizeof(renderer::IndexType) );
		
		LogoScreen * logo = CREATE(LogoScreen);
		HelpScreen * help = CREATE(HelpScreen);
		GameScreen * game = CREATE(GameScreen);

		// make the controller aware of these screens
		engine::engine()->screen_controller()->add_screen( logo );
		engine::engine()->screen_controller()->add_screen( help );
		engine::engine()->screen_controller()->add_screen( game );

		// setup the stack
		engine::engine()->screen_controller()->push_screen( "GameScreen", this );
//		engine::engine()->screen_controller()->push_screen( "LogoScreen", this );

		debugdraw::startup( 1024 );

		return kernel::Application_Success;
	}

	
	virtual void step( kernel::Params & params )
	{
		if (engine::engine()->screen_controller()->active_screen())
		{
			engine::engine()->screen_controller()->active_screen()->on_step( this );
		}
	}

	virtual void tick( kernel::Params & params )
	{
		if ( engine::engine()->screen_controller()->active_screen() )
		{
			engine::engine()->screen_controller()->active_screen()->on_update( this );
		}
	
		debugdraw::update( params.framedelta_filtered );
		rs.rewind();
		
		// setup global rendering state
		rs.add_clearcolor( 0.15, 0.10, 0.25, 1.0f );
		rs.add_clear( 0x00004000 | 0x00000100 );
		rs.add_viewport( 0, 0, (int)params.render_width, (int)params.render_height );

		rs.add_state( renderer::STATE_DEPTH_TEST, 0 );
		rs.run_commands();
		rs.rewind();
		
		if ( engine::engine()->screen_controller()->active_screen() )
		{
			engine::engine()->screen_controller()->active_screen()->on_draw( this );
		}

//		font::draw_string( test_font, 50, 50, xstr_format("deltatime: %gms", params.framedelta_filtered), Color(255,128,0,255) );
//		font::draw_string( test_font, 50, 75, "Ја могу да једем стакло", Color(255, 255, 255, 255) );
//		font::draw_string( test_font, 50, 100, "私はガラスを食べられます。それは私を傷つけません。", Color(0, 128, 255, 255) );
	}

	virtual void shutdown( kernel::Params & params )
	{
		debugdraw::shutdown();
	}
};

IMPLEMENT_APPLICATION( TestUniversal );
