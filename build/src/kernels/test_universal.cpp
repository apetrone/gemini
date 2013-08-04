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
#include <slim/xlog.h>
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
#include "menuscreen.hpp"
#include "win_loss_screen.hpp"

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


util::ConfigLoadStatus load_sprite_from_file( const Json::Value & root, void * data );

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
//		HelpScreen * help = CREATE(HelpScreen);
		GameScreen * game = CREATE(GameScreen);
		MenuScreen * menu = CREATE(MenuScreen);
		WinLossScreen * win = CREATE(WinLossScreen);
		win->setup_win_screen();
		
		WinLossScreen * loss = CREATE(WinLossScreen);
		loss->setup_loss_screen();

		// make the controller aware of these screens
		engine::engine()->screen_controller()->add_screen( logo );
//		engine::engine()->screen_controller()->add_screen( help );
		engine::engine()->screen_controller()->add_screen( game );
		engine::engine()->screen_controller()->add_screen( menu );
		engine::engine()->screen_controller()->add_screen( win );
		engine::engine()->screen_controller()->add_screen( loss );

		// setup the stack
		engine::engine()->screen_controller()->push_screen( "GameScreen", this );
//		engine::engine()->screen_controller()->push_screen( "MenuScreen", this );
//		engine::engine()->screen_controller()->push_screen( "LogoScreen", this );
//		engine::engine()->screen_controller()->push_screen( "WinScreen", this );
//		engine::engine()->screen_controller()->push_screen( "LossScreen", this );
		

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
