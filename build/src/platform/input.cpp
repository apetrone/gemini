// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone
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

#include "input.h"

#include <string.h>
#include <assert.h>

namespace input
{
	void ButtonState::press_release( bool is_down )
	{
		if ( is_down )
		{
			// this button was down last update, too
			if ( state & Button_IsDown )
			{
				state |= Button_Held;
			}
			else
			{
				state |= Button_IsDown;
				state |= Button_Impulse;
			}
		}
		else
		{
			// remove 'isDown' flag
			state &= ~Button_IsDown;
			
			// set 'released' and 'impulse' flag
			state |= Button_Impulse | Button_Released;
		}
	} // press_release
	
	void ButtonState::update()
	{
		// impulse flag
		if ( state & Button_Impulse )
		{
			if ( state & Button_Released ) // button released this update
			{
				state &= ~Button_IsDown;
			}
		}
		else
		{
			if ( state & Button_IsDown ) // button held
			{
				state |= Button_Held;
			}
			else if ( state & Button_Released ) // button released last update
			{
				state &= ~Button_Released;
				state &= ~Button_Held;
				state &= ~Button_IsDown;
			}
		}
		
		// clear impulse flag
		state &= ~Button_Impulse;
	} // update
	
	bool ButtonState::is_down() const
	{
		return (state & Button_Held) == Button_Held;
	}
	
	bool ButtonState::was_pressed() const
	{
		return (state == (Button_Impulse|Button_IsDown));
	}
	
	bool ButtonState::was_released() const
	{
		return (state == (Button_Impulse|Button_Released));
	}
	
	// prevent vtable from being emitted in every
	// translation unit
	InputDevice::~InputDevice()
	{
	}
	
	void startup( void )
	{
//		_input_state.keyboard().reset();
//		_input_state.mouse().reset();
	}
	
	void shutdown( void )
	{
		
	}
	
	void update( void )
	{
//		_input_state.keyboard().update( 0 );
//		_input_state.mouse().update( 0 );
		
#if 0
		for (uint8_t j = 0; j < MAX_JOYSTICKS; ++j)
		{
			JoystickInput& joystick = _input_state.joystick(j);
			if (joystick.flags & JoystickInput::Connected)
			{
				joystick.update(0);
			}
		}
#endif
	}

	//
	// KeyboardInput
	
	void KeyboardInput::reset()
	{
		memset(&keys, 0, sizeof(ButtonState) * KEY_COUNT);
	} // reset
	
	void KeyboardInput::update()
	{
		for( unsigned int i = 0; i < KEY_COUNT; ++i )
		{
			this->keys[i].update();
		}
	} // update
	
	void KeyboardInput::inject_key_event(int key, bool is_down)
	{
		input::Button button = (input::Button)key;
		assert( button <= KEY_COUNT );
		ButtonState* b = &this->keys[ button ];
		b->press_release(is_down);
	} // inject_key_event
	
//	bool KeyboardInput::is_down(input::Button key)
//	{
//		ButtonState* b;
//		assert( key < KEY_COUNT );
//		b = &keys[ key ];
//		return b->is_down();
//	} // is_down
//	
//	bool KeyboardInput::was_released(input::Button key )
//	{
//		ButtonState* b;
//		assert( key < KEY_COUNT );		
//		b = &keys[ key ];
//		return b->was_released();
//	} // was_released
	
	//
	// MouseInput
	
	void MouseInput::reset()
	{
		memset(&buttons, 0, sizeof(ButtonState) * MOUSE_COUNT);
		window_coords[0] = window_coords[1] = 0;
		delta[0] = delta[1] = 0;
	} // reset
	
	void MouseInput::update()
	{
		for( unsigned int i = 0; i < MOUSE_COUNT; ++i )
		{
			buttons[ MOUSE_LEFT+i ].update();
		}

		delta[0] = delta[1] = 0;
	} // update
	
	
	void MouseInput::inject_mouse_move(int absolute_x, int absolute_y)
	{
//		delta[0] = absolute_x - window_coords[0];
		window_coords[0] = absolute_x;
		
//		delta[1] = absolute_y - window_coords[1];
		window_coords[1] = absolute_y;
	} // inject_mouse_move
	
	void MouseInput::inject_mouse_delta(int delta_x, int delta_y)
	{
		delta[0] = delta_x;
//		window_coords[0] += delta_x;
		
		delta[1] = delta_y;
//		window_coords[1] += delta_y;
	} // inject_mouse_delta
	
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
		return buttons[ button ].is_down();
	} // is_down
	
	bool MouseInput::was_released( input::MouseButton button )
	{
		assert( button < MOUSE_COUNT && button >= 0 );
		// WAS down and NOT down now
		return buttons[button].was_released();
	} // was_released
	
	void MouseInput::mouse_position( int & x, int & y )
	{
		x = window_coords[1];
		y = window_coords[1];
	} // mouse_position
	
	void MouseInput::mouse_delta(int &dx, int &dy)
	{
		dx = delta[0];
		dy = delta[1];
	}

	//
	// TouchInput
	void TouchInput::reset()
	{
		
	} // reset
	
	void TouchInput::update()
	{
		
	} // update
	
	
	//
	// JoystickInput
	void JoystickInput::reset()
	{
		memset(&buttons, 0, sizeof(ButtonState) * MAX_JOYSTICK_BUTTONS);
		memset(&axes, 0, sizeof(AxisState) * MAX_JOYSTICK_AXES);
		flags = 0;
	}
	
	void JoystickInput::update()
	{
		for(uint8_t i = 0; i < MAX_JOYSTICK_BUTTONS; ++i)
		{
			buttons[ i ].update();
		}
	}
	
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
			case MOUSE_COUNT: break;
		}
		
		return "MOUSE_INVALID";
	} // mouse_button_name
	
	
} // namespace input
