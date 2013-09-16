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
#include "kernel.hpp"
#include <stdio.h>
#include "renderer.hpp"
#include <slim/xlog.h>
#include "input.hpp"
#include "renderstream.hpp"
#include "debugdraw.hpp"


using namespace kernel;

#include <nom/nom.hpp>

namespace gui
{
	void * default_gui_malloc( size_t bytes )
	{
		return malloc(bytes);
	} // default_gui_malloc
	
	void default_gui_free( void * p )
	{
		free( p );
	} // default_gui_free



	
	void set_custom_allocator( gui_malloc malloc_fn, gui_free free_fn ) {}
	
	gui_malloc _gmalloc = default_gui_malloc;
	gui_free _gfree = default_gui_free;
	
	Compositor * create_compositor( uint16_t width, uint16_t height )
	{
		void * p = (Compositor*)_gmalloc(sizeof(Compositor));
		Compositor * c = new (p) Compositor( width, height );
		return c;
	} // create_compositor
	
	void destroy_compositor( Compositor * c )
	{
		c->~Compositor();
		_gfree( c );
	} // destroy_compositor
	

}; // gui

using namespace gui;


class TestNom : public kernel::IApplication,
	public IEventListener<KeyboardEvent>,
	public IEventListener<MouseEvent>,
	public IEventListener<SystemEvent>
{
public:
	DECLARE_APPLICATION( TestNom );

	
	
	gui::Compositor * compositor;
	
	virtual void event( KeyboardEvent & event )
	{
        if ( event.is_down )
        {
			if ( event.key == input::KEY_ESCAPE )
			{
				kernel::instance()->set_active( false );
			}
            fprintf( stdout, "key %i pressed\n", event.key );
        }
        else
        {

            fprintf( stdout, "key %i released\n", event.key );
        }
		
		if ( compositor )
		{
			compositor->key_event( event.key, event.is_down, event.unicode );
		}
		
	}

	virtual void event( MouseEvent & event )
	{
        switch( event.subtype )
        {
            case kernel::MouseMoved:
			{
				if ( compositor )
				{
					compositor->cursor_move_absolute( event.mx, event.my );
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
				
				if ( compositor )
				{
					uint32_t button = 0;
					
					// convert  input button to nom button
					
					compositor->cursor_button( button, event.is_down );
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
		params.window_title = "TestNom";
		
		return kernel::Application_Success;
	}

	virtual kernel::ApplicationResult startup( kernel::Params & params )
	{
		compositor = gui::create_compositor( params.render_width, params.render_height );
//		gui::Properties * props = new gui::Properties( compositor );

//		gui::Properties * props = gui::create( "Properties", compositor );

		gui::Button * b = new gui::Button( compositor );
		compositor->add_child(b);
		b->bounds.set( 0, 0, 200, 200 );
		
		
		
		gui::Button * b2 = new gui::Button( compositor );
		compositor->add_child(b2);
		b2->bounds.set( 50, 300, 200, 200 );
		
		debugdraw::startup(128);
		
		return kernel::Application_Success;
	}
	
	virtual void step( kernel::Params & params )
	{
		if ( compositor )
		{
			compositor->update( params.step_interval_seconds );
		}
	}

	virtual void tick( kernel::Params & params )
	{
		RenderStream rs;

		rs.add_clear( renderer::CLEAR_COLOR_BUFFER | renderer::CLEAR_DEPTH_BUFFER );
		rs.add_viewport( 0, 0, params.render_width, params.render_height );
		rs.add_clearcolor( 0.1, 0.1, 0.1, 1.0f );
		rs.run_commands();
		
		if ( compositor )
		{
//			compositor->render();
			for( gui::PanelVector::iterator it = compositor->children.begin(); it != compositor->children.end(); ++it )
			{
				gui::Panel * panel = (*it);
				
				gui::Size size = panel->bounds.size;
				glm::vec3 start = glm::vec3( panel->bounds.origin.x, panel->bounds.origin.y, 0.0f );
				glm::vec3 end = start + glm::vec3( size.width, size.height, 0.0f );
				debugdraw::line( start, end, Color( 255, 0, 255 ) );
				debugdraw::point( glm::vec3( panel->bounds.origin.x + size.width, panel->bounds.origin.y + size.height, 0.0f ), Color(255, 255, 255) );
			}

		}
		
		glm::mat4 modelview;
		glm::mat4 projection = glm::ortho( 0.0f, (float)params.render_width, (float)params.render_height, 0.0f, 0.0f, 128.0f );
		
		debugdraw::render( modelview, projection, params.render_width, params.render_height );
	}

	virtual void shutdown( kernel::Params & params )
	{
		gui::destroy_compositor( compositor );
		
		debugdraw::shutdown();
	}
};

IMPLEMENT_APPLICATION( TestNom );