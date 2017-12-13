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
#pragma once

#include <core/typedefs.h>

#include <stdint.h>
#include <limits.h>

// Convert these to enum classes and then we
// won't need this to cleanup after Windows.
#if defined(MOD_CONTROL)
#undef MOD_CONTROL
#endif

#if defined(MOD_SHIFT)
#undef MOD_SHIFT
#endif

#if defined(MOD_ALT)
#undef MOD_ALT
#endif

// Renaming from 'KEY_' to 'BUTTON_' to deal with
// linux/input.h macros.

const size_t GEMINI_JOYSTICK_MAX_JOYSTICKS = 4;
const size_t GEMINI_TOUCHES_MAX_COUNT = 10;


// Joystick states
const size_t GEMINI_JOYSTICK_DISCONNECTED = 0; // joystick disconnected / not used
const size_t GEMINI_JOYSTICK_CONNECTED = 1; // joystick is connected and enabled
const size_t GEMINI_JOYSTICK_SUPPORTS_HAPTICS = 2; // joystick has haptic support

namespace gemini
{
	enum Button
	{
		BUTTON_INVALID,

		BUTTON_A,
		BUTTON_B,
		BUTTON_C,
		BUTTON_D,
		BUTTON_E,
		BUTTON_F,
		BUTTON_G,
		BUTTON_H,
		BUTTON_I,
		BUTTON_J,
		BUTTON_K,
		BUTTON_L,
		BUTTON_M,
		BUTTON_N,
		BUTTON_O,
		BUTTON_P,
		BUTTON_Q,
		BUTTON_R,
		BUTTON_S,
		BUTTON_T,
		BUTTON_U,
		BUTTON_V,
		BUTTON_W,
		BUTTON_Y,
		BUTTON_X,
		BUTTON_Z,

		// other keys
		BUTTON_MENU,
		BUTTON_SEMICOLON,
		BUTTON_SLASH,
		BUTTON_BACKSLASH,
		BUTTON_EQUALS,
		BUTTON_MINUS,
		BUTTON_PLUS,
		BUTTON_LBRACKET,
		BUTTON_RBRACKET,
		BUTTON_COMMA,
		BUTTON_PERIOD,
		BUTTON_QUOTE,
		BUTTON_TILDE,
		BUTTON_ESCAPE,
		BUTTON_SPACE,
		BUTTON_RETURN,
		BUTTON_BACKSPACE,
		BUTTON_TAB,
		BUTTON_PAGEUP,
		BUTTON_PAGEDN,
		BUTTON_END,
		BUTTON_HOME,
		BUTTON_INSERT,
		BUTTON_DELETE,
		BUTTON_PAUSE,

		BUTTON_LSHIFT,
		BUTTON_RSHIFT,
		BUTTON_LCONTROL,
		BUTTON_RCONTROL,
		BUTTON_LALT,
		BUTTON_RALT,
		BUTTON_NUMLOCK,
		BUTTON_CAPSLOCK,
		BUTTON_SCROLLLOCK,

		// windows key / command key (left/right variants)
		BUTTON_LOSKEY,
		BUTTON_ROSKEY,

		BUTTON_FUNCTION,

		// numeric keys
		BUTTON_0,
		BUTTON_1,
		BUTTON_2,
		BUTTON_3,
		BUTTON_4,
		BUTTON_5,
		BUTTON_6,
		BUTTON_7,
		BUTTON_8,
		BUTTON_9,

		// function keys
		BUTTON_F1,
		BUTTON_F2,
		BUTTON_F3,
		BUTTON_F4,
		BUTTON_F5,
		BUTTON_F6,
		BUTTON_F7,
		BUTTON_F8,
		BUTTON_F9,
		BUTTON_F10,
		BUTTON_F11,
		BUTTON_F12,
		BUTTON_F13,
		BUTTON_F14,
		BUTTON_F15,
		BUTTON_F16,
		BUTTON_F17,
		BUTTON_F18,
		BUTTON_F19,
		BUTTON_F20,
		BUTTON_F21,
		BUTTON_F22,
		BUTTON_F23,
		BUTTON_F24,

		// directional keys
		BUTTON_LEFT,
		BUTTON_RIGHT,
		BUTTON_UP,
		BUTTON_DOWN,

		// numpad keys
		BUTTON_NUMPAD0,
		BUTTON_NUMPAD1,
		BUTTON_NUMPAD2,
		BUTTON_NUMPAD3,
		BUTTON_NUMPAD4,
		BUTTON_NUMPAD5,
		BUTTON_NUMPAD6,
		BUTTON_NUMPAD7,
		BUTTON_NUMPAD8,
		BUTTON_NUMPAD9,
		BUTTON_NUMPAD_PLUS,
		BUTTON_NUMPAD_MINUS,
		BUTTON_NUMPAD_PLUSMINUS,
		BUTTON_NUMPAD_MULTIPLY,
		BUTTON_NUMPAD_DIVIDE,
		BUTTON_NUMPAD_PERIOD,
		BUTTON_NUMPAD_ENTER,
		BUTTON_NUMPAD_EQUALS,

		BUTTON_COUNT
	}; // enum Button

	enum MouseButton
	{
		MOUSE_INVALID,

		MOUSE_LEFT,
		MOUSE_RIGHT,
		MOUSE_MIDDLE,
		MOUSE_MOUSE4,
		MOUSE_MOUSE5,
		MOUSE_MOUSE6,
		MOUSE_MOUSE7,

		MOUSE_COUNT
	}; // enum MouseButton

	// key mods
	enum Modifiers
	{
		MOD_NONE,

		MOD_LEFT_CONTROL	= 1,
		MOD_RIGHT_CONTROL	= 2,
		MOD_CONTROL			= 3,

		MOD_LEFT_SHIFT		= 4,
		MOD_RIGHT_SHIFT		= 8,
		MOD_SHIFT			= 12,

		MOD_LEFT_ALT		= 16,
		MOD_RIGHT_ALT		= 32,
		MOD_ALT				= 48
	}; // enum Modifiers


	// This is a similar pattern to that found in Quake. I find it very logical to determining input state.
	enum ButtonStateFlags
	{
		Button_IsDown 	= 1,
		Button_Held 	= 2,
		Button_Released = 4,
		Button_Impulse 	= 8
	};

	struct ButtonState
	{
		uint8_t state;
		uint16_t axis_value;
		uint64_t timestamp;

		// handle a press or release event
		void update_state(uint16_t value, uint64_t current_tick);

		// Returns the number of ticks this button is down for, or zero.
		uint64_t held_ticks(uint64_t current_tick) const;

		// update this button state for this frame
		void update();

		// returns whether the button is down or not this frame
		bool is_down() const;

		// returns whether or not the button was just pressed
		bool was_pressed() const;

		// returns whether or not the button was just released
		bool was_released() const;

		// returns a normalized float value
		float value() const;
	}; // ButtonState

	// This is modeled after SDL2's enums, which are in turn, modeled after the
	// Xbox 360 config.
	enum GamepadButton
	{
		GAMEPAD_BUTTON_INVALID,

		// left stick axes (split into cardinal directions)
		GAMEPAD_STICK0_AXIS_RIGHT,
		GAMEPAD_STICK0_AXIS_LEFT,
		GAMEPAD_STICK0_AXIS_UP,
		GAMEPAD_STICK0_AXIS_DOWN,

		// right stick axes (split into cardinal directions)
		GAMEPAD_STICK1_AXIS_RIGHT,
		GAMEPAD_STICK1_AXIS_LEFT,
		GAMEPAD_STICK1_AXIS_UP,
		GAMEPAD_STICK1_AXIS_DOWN,

		GAMEPAD_JOYSTICK_BUTTON_OFFSET,

		GAMEPAD_BUTTON_A,
		GAMEPAD_BUTTON_B,
		GAMEPAD_BUTTON_C,
		GAMEPAD_BUTTON_X,
		GAMEPAD_BUTTON_Y,
		GAMEPAD_BUTTON_Z,
		GAMEPAD_BUTTON_BACK,
		GAMEPAD_BUTTON_GUIDE,
		GAMEPAD_BUTTON_SELECT,
		GAMEPAD_BUTTON_START,
		GAMEPAD_BUTTON_LEFTSTICK,
		GAMEPAD_BUTTON_RIGHTSTICK,
		GAMEPAD_BUTTON_LEFTSHOULDER,
		GAMEPAD_BUTTON_RIGHTSHOULDER,
		GAMEPAD_BUTTON_L2,
		GAMEPAD_BUTTON_R2,
		GAMEPAD_BUTTON_L3,
		GAMEPAD_BUTTON_R3,
		GAMEPAD_BUTTON_DPAD_UP,
		GAMEPAD_BUTTON_DPAD_DOWN,
		GAMEPAD_BUTTON_DPAD_LEFT,
		GAMEPAD_BUTTON_DPAD_RIGHT,

		GAMEPAD_BUTTON_COUNT
	}; // enum GamepadButton

	const char* mouse_button_name(unsigned int button);
	const char* key_name(unsigned int key);
	const char* joystick_button_name(uint32_t button);

} // namespace gemini
