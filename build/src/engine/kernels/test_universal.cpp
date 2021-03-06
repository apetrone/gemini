// -------------------------------------------------------------
// Copyright (C) 2012- Adam Petrone
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//		* Redistributions of source code must retain the above copyright notice,
//		this list of conditions and the following disclaimer.

//		* Redistributions in binary form must reproduce the above copyright notice,
//		this list of conditions and the following disclaimer in the documentation
//		and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//		 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------
#include <gemini/typedefs.h>
#include "kernel.h"
#include <stdio.h>
#include "renderer.h"
#include "audio.h"
#include "input.h"
#include <slim/xlog.h>
//#include <squirrel.h>
#include "filesystem.h"
#include "game/menu.h"
#include "mathlib.h"
#include "assets.h"
#include "camera.h"
#include "renderstream.h"
#include "font.h"
#include "render_utilities.h"
#include "tiledloader.h"
#include "screencontrol.h"
#include "helpscreen.h"
#include "logoscreen.h"
#include "gamescreen.h"
#include "menuscreen.h"
#include "win_loss_screen.h"

#include "keyframechannel.h"
#include "debugdraw.h"
#include "util.h"
#include "particlesystem.h"
#include "engine.h"

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

		// tests
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

		debugdraw::update( params.framedelta_filtered_msec );
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

//		font::draw_string( test_font, 50, 50, xstr_format("deltatime: %gms", params.framedelta_filtered_msec), Color(255,128,0,255) );
//		font::draw_string( test_font, 50, 75, "Ја могу да једем стакло", Color(255, 255, 255, 255) );
//		font::draw_string( test_font, 50, 100, "私はガラスを食べられます。それは私を傷つけません。", Color(0, 128, 255, 255) );
	}

	virtual void shutdown( kernel::Params & params )
	{
		debugdraw::shutdown();
	}
};

IMPLEMENT_APPLICATION( TestUniversal );
