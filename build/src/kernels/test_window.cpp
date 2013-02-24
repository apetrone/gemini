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
#include "log.h"

#include "audio.hpp"


using namespace kernel;


class TestWindow : public kernel::IApplication,
	public IEventListener<KeyboardEvent>,
	public IEventListener<MouseEvent>,
	public IEventListener<SystemEvent>
{
public:
	DECLARE_APPLICATION( TestWindow );

	
	audio::SoundHandle sound;
	
	virtual void event( KeyboardEvent & event )
	{
        if ( event.is_down )
        {
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
		params.window_title = "TestWindow";
		
		return kernel::Success;
	}

	virtual kernel::ApplicationResult startup( kernel::Params & params )
	{
		sound = audio::create_sound( "sounds/powerup" );
		audio::play( sound );
		
		
		
		return kernel::Success;
	}
	
	virtual void step( kernel::Params & params )
	{
	}

	virtual void tick( kernel::Params & params )
	{
		renderer::IRenderDriver * driver = renderer::driver();
		MemoryStream ms;
		char buffer[128] = {0};
		ms.init( buffer, 128 );
		
		// viewport
		ms.rewind();
		ms.write( 0 );
		ms.write( 0 );
		ms.write( params.window_width );
		ms.write( params.window_width );
		ms.rewind();
		driver->run_command( renderer::DC_VIEWPORT, ms );
		
		// set clear color
		ms.rewind();
		ms.write( 0.5f );
		ms.write( 0.0f );
		ms.write( 0.75f );
		ms.write( 1.0f );
		ms.rewind();
		driver->run_command( renderer::DC_CLEARCOLOR, ms );
		
		// color_buffer_bit
		ms.rewind();
		ms.write( 0x00004000 );
		ms.rewind();
		driver->run_command( renderer::DC_CLEAR, ms );
	}

	virtual void shutdown( kernel::Params & params )
	{
	}
};

IMPLEMENT_APPLICATION( TestWindow );