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
#include "typedefs.h"
#include "input.hpp"
#include <slim/xlog.h>

namespace input
{
	unsigned int * _input_mapping_table = 0;
	
	InputState _input_state;
	InputState * state( void )
	{
		return &_input_state;
	}
	
	void ButtonState::press_release( bool is_down )
	{
		if ( is_down )
		{
			// this button was down last update, too
			if ( state & 1 )
			{
				state |= 2;
			}
			else
			{
				state |= 1;
				state |= 8;
			}
		}
		else
		{
			// remove 'isDown' flag
			state &= ~1;
			
			// set 'released' and 'impulse' flag
			state |= 12;
		}
	} // press_release
	
	void ButtonState::update()
	{
		// impulse flag
		if ( state & 8 )
		{
			if ( state & 1 ) // button down this update
			{
//				LOGV( "button %i is down\n", i );
			}
			else if ( state & 4 ) // button released this update
			{
//				LOGV( "button %i is released\n", i );
				state &= ~1;
			}
		}
		else
		{
			if ( state & 1 ) // button held
			{
				state |= 2;
			}
			else if ( state & 4 ) // button released last update
			{
				state &= ~4;
				state &= ~2;
				state &= ~1;
			}
		}
		
		// clear impulse flag
		state &= ~8;
	} // update
	
	void startup( void )
	{
		_input_state.keyboard().reset();
		_input_state.mouse().reset();
	}
	
	void shutdown( void )
	{
		
	}
	
	void update( void )
	{
		_input_state.keyboard().update( 0 );
		_input_state.mouse().update( 0 );
	}

	//
	// KeyboardInput
	
	void KeyboardInput::reset()
	{
		memset(&keys, 0, sizeof(ButtonState) * KEY_COUNT);
	} // reset
	
	void KeyboardInput::update( float delta_msec )
	{
		for( unsigned int i = 0; i < KEY_COUNT; ++i )
		{
			this->keys[i].update();
		}
	} // update
	
	void KeyboardInput::inject_key_event( int key, bool is_down, int unicode )
	{
		input::Button button = (input::Button)key;
		assert( button <= KEY_COUNT );
		
		ButtonState * b = &this->keys[ button ];
		if ( is_down )
		{
			// this button was down last update, too		
			if ( b->state & 1 )
			{
				b->state |= 2;
			}
			else
			{
				b->state |= 1;
				b->state |= 8;
			}
		}
		else
		{
			// remove 'isDown' flag
			b->state &= ~1;
			
			// set 'released' and 'impulse' flag
			b->state |= 12;
		}
		
	} // inject_key_event
	
	bool KeyboardInput::is_down(input::Button key)
	{
		ButtonState * b;
		assert( key < KEY_COUNT );
		b = &keys[ key ];
		return (b->state & 1);
	} // is_down
	
	bool KeyboardInput::was_released(input::Button key )
	{
		ButtonState * b;
		assert( key < KEY_COUNT );		
		b = &keys[ key ];
		// WAS down and NOT down now
		return (b->state & 2) && !(b->state & 1);
	} // was_released
	
	//
	// MouseInput
	
	void MouseInput::reset()
	{
		memset(&buttons, 0, sizeof(ButtonState) * MOUSE_COUNT);
	} // reset
	
	void MouseInput::update( float delta_msec )
	{
		for( unsigned int i = 0; i < MOUSE_COUNT; ++i )
		{
			buttons[ MOUSE_LEFT+i ].update();
		}
	} // update
	
	
	void MouseInput::inject_mouse_move( int absolute_x, int absolute_y )
	{
		mousex[0] = mousex[1];
		mousex[1] = absolute_x;
		
		mousey[0] = mousey[1];
		mousey[1] = absolute_y;
	} // inject_mouse_move
	
	void MouseInput::inject_mouse_button( MouseButton button, bool is_down )
	{
		assert( button < MOUSE_COUNT && button >= 0 );
		buttons[ button ].press_release( is_down );
	} // inject_mouse_button
	
	void MouseInput::inject_mouse_wheel( int direction )
	{
		wheel_direction = direction;
	} // inject_mouse_wheel
	
	bool MouseInput::is_down( input::MouseButton button )
	{
		assert( button < MOUSE_COUNT && button >= 0 );
		return buttons[ button ].state & 1;
	} // is_down
	
	bool MouseInput::was_released( input::MouseButton button )
	{
		assert( button < MOUSE_COUNT && button >= 0 );
		// WAS down and NOT down now
		return (buttons[ button ].state & 2) && !(buttons[ button ].state & 1 );
	} // was_released
	
	void MouseInput::last_mouse_position( int & x, int & y )
	{
		x = mousex[0];
		y = mousey[0];
	} // last_mouse_position
	
	void MouseInput::mouse_position( int & x, int & y )
	{
		x = mousex[1];
		y = mousey[1];
	} // mouse_position

	//
	// TouchInput
	void TouchInput::reset()
	{
		
	} // reset
	
	void TouchInput::update( float delta_msec )
	{
		
	} // update
	
	
	
	const char * mouse_button_name( MouseButton button )
	{
		switch(button)
		{
			case MOUSE_LEFT: return "MOUSE_LEFT";
			case MOUSE_RIGHT: return "MOUSE_RIGHT";
			case MOUSE_MIDDLE: return "MOUSE_MIDDLE";
			case MOUSE_MOUSE4: return "MOUSE_MOUSE4";
			case MOUSE_MOUSE5: return "MOUSE_MOUSE5";
			case MOUSE_MOUSE6: return "MOUSE_MOUSE6";
			case MOUSE_MOUSE7: return "MOUSE_MOUSE7";
			default: return "MOUSE_INVALID";
		}
	} // mouse_button_name
	
	
}; // namespace input