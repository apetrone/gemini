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
			state = Button_Impulse | Button_Released;
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

	void startup(void)
	{
//		_input_state.keyboard().reset();
//		_input_state.mouse().reset();
	}

	void shutdown(void)
	{

	}

	void update(void)
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
			"BUTTON_INVALID",
			"BUTTON_A",
			"BUTTON_B",
			"BUTTON_C",
			"BUTTON_D",
			"BUTTON_E",
			"BUTTON_F",
			"BUTTON_G",
			"BUTTON_H",
			"BUTTON_I",
			"BUTTON_J",
			"BUTTON_K",
			"BUTTON_L",
			"BUTTON_M",
			"BUTTON_N",
			"BUTTON_O",
			"BUTTON_P",
			"BUTTON_Q",
			"BUTTON_R",
			"BUTTON_S",
			"BUTTON_T",
			"BUTTON_U",
			"BUTTON_V",
			"BUTTON_W",
			"BUTTON_Y",
			"BUTTON_X",
			"BUTTON_Z",
			"BUTTON_MENU",
			"BUTTON_SEMICOLON",
			"BUTTON_SLASH",
			"BUTTON_BACKSLASH",
			"BUTTON_EQUALS",
			"BUTTON_MINUS",
			"BUTTON_LBRACKET",
			"BUTTON_RBRACKET",
			"BUTTON_COMMA",
			"BUTTON_PERIOD",
			"BUTTON_QUOTE",
			"BUTTON_TILDE",
			"BUTTON_ESCAPE",
			"BUTTON_SPACE",
			"BUTTON_RETURN",
			"BUTTON_BACKSPACE",
			"BUTTON_TAB",
			"BUTTON_PAGEUP",
			"BUTTON_PAGEDN",
			"BUTTON_END",
			"BUTTON_HOME",
			"BUTTON_INSERT",
			"BUTTON_DELETE",
			"BUTTON_PAUSE",
			"BUTTON_LSHIFT",
			"BUTTON_RSHIFT",
			"BUTTON_LCONTROL",
			"BUTTON_RCONTROL",
			"BUTTON_LALT",
			"BUTTON_RALT",
			"BUTTON_NUMLOCK",
			"BUTTON_CAPSLOCK",
			"BUTTON_LOSKEY",
			"BUTTON_ROSKEY",
			"BUTTON_FUNCTION",
			"BUTTON_0",
			"BUTTON_1",
			"BUTTON_2",
			"BUTTON_3",
			"BUTTON_4",
			"BUTTON_5",
			"BUTTON_6",
			"BUTTON_7",
			"BUTTON_8",
			"BUTTON_9",
			"BUTTON_F1",
			"BUTTON_F2",
			"BUTTON_F3",
			"BUTTON_F4",
			"BUTTON_F5",
			"BUTTON_F6",
			"BUTTON_F7",
			"BUTTON_F8",
			"BUTTON_F9",
			"BUTTON_F10",
			"BUTTON_F11",
			"BUTTON_F12",
			"BUTTON_F13",
			"BUTTON_F14",
			"BUTTON_F15",
			"BUTTON_F16",
			"BUTTON_F17",
			"BUTTON_F18",
			"BUTTON_F19",
			"BUTTON_F20",
			"BUTTON_LEFT",
			"BUTTON_RIGHT",
			"BUTTON_UP",
			"BUTTON_DOWN",
			"BUTTON_NUMPAD0",
			"BUTTON_NUMPAD1",
			"BUTTON_NUMPAD2",
			"BUTTON_NUMPAD3",
			"BUTTON_NUMPAD4",
			"BUTTON_NUMPAD5",
			"BUTTON_NUMPAD6",
			"BUTTON_NUMPAD7",
			"BUTTON_NUMPAD8",
			"BUTTON_NUMPAD9",
			"BUTTON_NUMPAD_PLUS",
			"BUTTON_NUMPAD_MINUS",
			"BUTTON_NUMPAD_PLUSMINUS",
			"BUTTON_NUMPAD_MULTIPLY",
			"BUTTON_NUMPAD_DIVIDE",
			"BUTTON_NUMPAD_PERIOD",
			"BUTTON_NUMPAD_ENTER",
			"BUTTON_NUMPAD_EQUALS"
		};

		if (key < BUTTON_COUNT)
		{
			return names[key];
		}

		return "BUTTON_INVALID";
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
		memset(&keys, 0, sizeof(ButtonState) * BUTTON_COUNT);
	} // reset

	void KeyboardInput::update()
	{
		for( unsigned int i = 0; i < BUTTON_COUNT; ++i )
		{
			this->keys[i].update();
		}
	} // update

	void KeyboardInput::inject_key_event(int key, bool is_down)
	{
		input::Button button = (input::Button)key;
		assert( button <= BUTTON_COUNT );
		ButtonState* b = &this->keys[ button ];
		b->press_release(is_down);
	} // inject_key_event

//	bool KeyboardInput::is_down(input::Button key)
//	{
//		ButtonState* b;
//		assert( key < BUTTON_COUNT );
//		b = &keys[ key ];
//		return b->is_down();
//	} // is_down
//
//	bool KeyboardInput::was_released(input::Button key )
//	{
//		ButtonState* b;
//		assert( key < BUTTON_COUNT );
//		b = &keys[ key ];
//		return b->was_released();
//	} // was_released

	//
	// MouseInput

	void MouseInput::reset()
	{
		memset(&buttons, 0, sizeof(ButtonState) * MOUSE_COUNT);
		window_coords[0] = window_coords[1] = 0;
	} // reset

	void MouseInput::update()
	{
		for( unsigned int i = 0; i < MOUSE_COUNT; ++i )
		{
			buttons[ MOUSE_LEFT+i ].update();
		}
	} // update


	void MouseInput::inject_mouse_move(int absolute_x, int absolute_y)
	{
		window_coords[0] = absolute_x;
		window_coords[1] = absolute_y;

//		fprintf(stdout, "inject move: %i %i\n", absolute_x, absolute_y);
	} // inject_mouse_move

	void MouseInput::inject_mouse_delta(int dx, int dy)
	{
		cursor_delta[0] += dx;
		cursor_delta[1] += dy;
	}

	void MouseInput::inject_mouse_button(MouseButton button, bool is_down)
	{
		assert( button < MOUSE_COUNT && button >= 0 );
		buttons[ button ].press_release( is_down );
	} // inject_mouse_button

	void MouseInput::inject_mouse_wheel(int direction)
	{
		wheel_direction = direction;
	} // inject_mouse_wheel

	bool MouseInput::is_down(MouseButton button)
	{
		assert( button < MOUSE_COUNT && button >= 0 );
		return buttons[ button ].is_down();
	} // is_down

	bool MouseInput::was_released(MouseButton button)
	{
		assert( button < MOUSE_COUNT && button >= 0 );
		// WAS down and NOT down now
		return buttons[button].was_released();
	} // was_released

	void MouseInput::mouse_position(int& x, int& y)
	{
		x = window_coords[0];
		y = window_coords[1];
	} // mouse_position

	void MouseInput::mouse_delta(int& dx, int& dy)
	{
		dx = cursor_delta[0];
		dy = cursor_delta[1];
	} // mouse_delta

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
