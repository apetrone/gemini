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

#include <platform/platform.h>

namespace gemini
{
	void ButtonState::update_state( bool is_down )
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
	} // update_state

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
		return state & (Button_Held | Button_IsDown);
	}

	bool ButtonState::was_pressed() const
	{
		return (state == (Button_Impulse|Button_IsDown));
	}

	bool ButtonState::was_released() const
	{
		return (state == (Button_Impulse|Button_Released));
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
			"BUTTON_SCROLLLOCK",

			// windows key / command key (left/right variants)
			"BUTTON_LOSKEY",
			"BUTTON_ROSKEY",

			"BUTTON_FUNCTION",

			// numeric keys
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

			// function keys
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
			"BUTTON_F21",
			"BUTTON_F22",
			"BUTTON_F23",
			"BUTTON_F24",

			// directional keys
			"BUTTON_LEFT",
			"BUTTON_RIGHT",
			"BUTTON_UP",
			"BUTTON_DOWN",

			// numpad keys
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



	const char* joystick_button_name(unsigned int value)
	{
		static const char* names[] = {
			"GAMEPAD_BUTTON_INVALID",
			"GAMEPAD_BUTTON_A",
			"GAMEPAD_BUTTON_B",
			"GAMEPAD_BUTTON_C",
			"GAMEPAD_BUTTON_X",
			"GAMEPAD_BUTTON_Y",
			"GAMEPAD_BUTTON_Z",
			"GAMEPAD_BUTTON_BACK",
			"GAMEPAD_BUTTON_GUIDE",
			"GAMEPAD_BUTTON_SELECT",
			"GAMEPAD_BUTTON_START",
			"GAMEPAD_BUTTON_LEFTSTICK",
			"GAMEPAD_BUTTON_RIGHTSTICK",
			"GAMEPAD_BUTTON_LEFTSHOULDER",
			"GAMEPAD_BUTTON_RIGHTSHOULDER",
			"GAMEPAD_BUTTON_L2",
			"GAMEPAD_BUTTON_R2",
			"GAMEPAD_BUTTON_DPAD_UP",
			"GAMEPAD_BUTTON_DPAD_DOWN",
			"GAMEPAD_BUTTON_DPAD_LEFT",
			"GAMEPAD_BUTTON_DPAD_RIGHT"
		};

		if (value < GAMEPAD_BUTTON_COUNT)
		{
			return names[value];
		}

		return "GAMEPAD_BUTTON_INVALID";
	} // joystick_button_name
} // namespace gemini
