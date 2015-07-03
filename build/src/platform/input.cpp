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

	const char* mouse_button_name(unsigned int value)
	{
		static const char* names[] = {
			"MOUSE_INVALID",
			"MOUSE_LEFT",
			"MOUSE_RIGHT",
			"MOUSE_MIDDLE",
			"MOUSE_MOUSE4",
			"MOUSE_MOUSE5",
			"MOUSE_MOUSE6",
			"MOUSE_MOUSE7"
		};
		
		if (value < MOUSE_COUNT)
		{
			return names[value];
		}
		
		return "MOUSE_INVALID";
	} // mouse_button_name
	
	const char* key_name(unsigned int key)
	{
		static const char* names[] = {
			"KEY_INVALID",
			"KEY_A",
			"KEY_B",
			"KEY_C",
			"KEY_D",
			"KEY_E",
			"KEY_F",
			"KEY_G",
			"KEY_H",
			"KEY_I",
			"KEY_J",
			"KEY_K",
			"KEY_L",
			"KEY_M",
			"KEY_N",
			"KEY_O",
			"KEY_P",
			"KEY_Q",
			"KEY_R",
			"KEY_S",
			"KEY_T",
			"KEY_U",
			"KEY_V",
			"KEY_W",
			"KEY_Y",
			"KEY_X",
			"KEY_Z",
			"KEY_MENU",
			"KEY_SEMICOLON",
			"KEY_SLASH",
			"KEY_BACKSLASH",
			"KEY_EQUALS",
			"KEY_MINUS",
			"KEY_LBRACKET",
			"KEY_RBRACKET",
			"KEY_COMMA",
			"KEY_PERIOD",
			"KEY_QUOTE",
			"KEY_TILDE",
			"KEY_ESCAPE",
			"KEY_SPACE",
			"KEY_RETURN",
			"KEY_BACKSPACE",
			"KEY_TAB",
			"KEY_PAGEUP",
			"KEY_PAGEDN",
			"KEY_END",
			"KEY_HOME",
			"KEY_INSERT",
			"KEY_DELETE",
			"KEY_PAUSE",
			"KEY_LSHIFT",
			"KEY_RSHIFT",
			"KEY_LCONTROL",
			"KEY_RCONTROL",
			"KEY_LALT",
			"KEY_RALT",
			"KEY_NUMLOCK",
			"KEY_CAPSLOCK",
			"KEY_LOSKEY",
			"KEY_ROSKEY",
			"KEY_FUNCTION",
			"KEY_0",
			"KEY_1",
			"KEY_2",
			"KEY_3",
			"KEY_4",
			"KEY_5",
			"KEY_6",
			"KEY_7",
			"KEY_8",
			"KEY_9",
			"KEY_F1",
			"KEY_F2",
			"KEY_F3",
			"KEY_F4",
			"KEY_F5",
			"KEY_F6",
			"KEY_F7",
			"KEY_F8",
			"KEY_F9",
			"KEY_F10",
			"KEY_F11",
			"KEY_F12",
			"KEY_F13",
			"KEY_F14",
			"KEY_F15",
			"KEY_F16",
			"KEY_F17",
			"KEY_F18",
			"KEY_F19",
			"KEY_F20",
			"KEY_LEFT",
			"KEY_RIGHT",
			"KEY_UP",
			"KEY_DOWN",
			"KEY_NUMPAD0",
			"KEY_NUMPAD1",
			"KEY_NUMPAD2",
			"KEY_NUMPAD3",
			"KEY_NUMPAD4",
			"KEY_NUMPAD5",
			"KEY_NUMPAD6",
			"KEY_NUMPAD7",
			"KEY_NUMPAD8",
			"KEY_NUMPAD9",
			"KEY_NUMPAD_PLUS",
			"KEY_NUMPAD_MINUS",
			"KEY_NUMPAD_PLUSMINUS",
			"KEY_NUMPAD_MULTIPLY",
			"KEY_NUMPAD_DIVIDE",
			"KEY_NUMPAD_PERIOD",
			"KEY_NUMPAD_ENTER",
			"KEY_NUMPAD_EQUALS"
		};
		
		if (key < KEY_COUNT)
		{
			return names[key];
		}
		
		return "KEY_INVALID";
	} // key_name
	
	
	const char* gamepad_name(unsigned int value)
	{
		static const char* names[] = {
			"GAMEPAD_BUTTON_INVALID",
			"GAMEPAD_BUTTON_A",
			"GAMEPAD_BUTTON_B",
			"GAMEPAD_BUTTON_X",
			"GAMEPAD_BUTTON_Y",
			"GAMEPAD_BUTTON_BACK",
			"GAMEPAD_BUTTON_GUIDE",
			"GAMEPAD_BUTTON_START",
			"GAMEPAD_BUTTON_LEFTSTICK",
			"GAMEPAD_BUTTON_RIGHTSTICK",
			"GAMEPAD_BUTTON_LEFTSHOULDER",
			"GAMEPAD_BUTTON_RIGHTSHOULDER",
			"GAMEPAD_BUTTON_DPAD_UP",
			"GAMEPAD_BUTTON_DPAD_DOWN",
			"GAMEPAD_BUTTON_DPAD_LEFT",
			"GAMEPAD_BUTTON_DPAD_RIGHT",
		};
		
		if (value < GAMEPAD_BUTTON_COUNT)
		{
			return names[value];
		}
		
		return "GAMEPAD_BUTTON_INVALID";
	} // gamepad_name
	
	// ---------------------------------------------------------------------
	// devices
	// ---------------------------------------------------------------------
	

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
} // namespace input
