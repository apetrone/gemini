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

using namespace kernel;


namespace gui
{
	struct Compositor
	{
		// step_seconds The fixed timestep in which the last step happened
		void step( float step_seconds ) {}
		
		// delta_seconds The time since the last tick
		void tick( float delta_seconds ) {}
		
		// render this compositor to a texture
		void render() {}
	};

	


	Compositor * create_compositor() { return 0; }
	void destroy_compositor( Compositor * c ) {}
	
	typedef void * (*gui_malloc)( size_t bytes );
	typedef void (*gui_free)( void * ptr );
	
	void set_custom_allocator( gui_malloc malloc_fn, gui_free free_fn ) {}
};


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
	}

	virtual void event( MouseEvent & event )
	{
        switch( event.subtype )
        {
            case kernel::MouseMoved:
                break;
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
		params.window_title = "TestNom";
		
		return kernel::Application_Success;
	}

	virtual kernel::ApplicationResult startup( kernel::Params & params )
	{
		compositor = gui::create_compositor();
		
		return kernel::Application_Success;
	}
	
	virtual void step( kernel::Params & params )
	{
		if ( compositor )
		{
			compositor->step( params.step_interval_seconds );
		}
	}

	virtual void tick( kernel::Params & params )
	{
		if ( compositor )
		{
			compositor->tick( params.framedelta_filtered_msec );
		}
	}

	virtual void shutdown( kernel::Params & params )
	{
		gui::destroy_compositor( compositor );
	}
};

IMPLEMENT_APPLICATION( TestNom );