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

namespace input
{
	const unsigned int MAX_INPUTSTATE_TOUCHES = 10;

	const uint8_t MAX_JOYSTICK_BUTTONS = 16;
	const uint8_t MAX_JOYSTICK_AXES = 6;
	const uint8_t MAX_JOYSTICKS = 8;

	const int16_t AxisValueMinimum = SHRT_MIN;
	const int16_t AxisValueMaximum = SHRT_MAX;

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

	// This is modeled after SDL2's enums, which are in turn, modeled after the
	// Xbox 360 config.
	enum GamepadButton
	{
		GAMEPAD_BUTTON_INVALID,

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
		GAMEPAD_BUTTON_DPAD_UP,
		GAMEPAD_BUTTON_DPAD_DOWN,
		GAMEPAD_BUTTON_DPAD_LEFT,
		GAMEPAD_BUTTON_DPAD_RIGHT,

		GAMEPAD_BUTTON_COUNT
	}; // enum GamepadButton

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
	};


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

		// handle a press or release event
		LIBRARY_EXPORT void press_release( bool is_down );

		// update this button state for this frame
		LIBRARY_EXPORT void update();

		// returns whether the button is down or not this frame
		LIBRARY_EXPORT bool is_down() const;

		// returns whether or not the button was just pressed
		LIBRARY_EXPORT bool was_pressed() const;

		// returns whether or not the button was just released
		LIBRARY_EXPORT bool was_released() const;
	}; // ButtonState


	struct AxisState
	{
		float normalized_value;
		int16_t value;
	};

	class InputDevice
	{
	public:
		LIBRARY_EXPORT virtual ~InputDevice();

		LIBRARY_EXPORT virtual void reset() = 0;
		LIBRARY_EXPORT virtual void update() = 0;
	}; // InputDevice

	class KeyboardInput : public InputDevice
	{
		ButtonState keys[ BUTTON_COUNT ];

	public:
		LIBRARY_EXPORT virtual void reset();
		LIBRARY_EXPORT virtual void update();

		LIBRARY_EXPORT void inject_key_event(int key, bool is_down);

		inline const ButtonState& get_key(input::Button key) { return keys[key]; }

		// Accessors
//		bool is_down( input::Button key );
//		bool was_released( input::Button key );
	}; // KeyboardInput


	class MouseInput : public InputDevice
	{
		// absolute mouse position in window coordinates
		int window_coords[2];
		int cursor_delta[2];

		int wheel_direction;

		ButtonState buttons[ MOUSE_COUNT ];


	public:
		LIBRARY_EXPORT virtual void reset();
		LIBRARY_EXPORT virtual void update();

		LIBRARY_EXPORT void inject_mouse_move(int absolute_x, int absolute_y);
		LIBRARY_EXPORT void inject_mouse_delta(int dx, int dy);
		LIBRARY_EXPORT void inject_mouse_button( MouseButton button_id, bool is_down );
		LIBRARY_EXPORT void inject_mouse_wheel( int direction );

		//
		// Accessors
		LIBRARY_EXPORT bool is_down(MouseButton button);
		LIBRARY_EXPORT bool was_released(MouseButton button);

		// retrieve the current mouse position in screen coordinates
		LIBRARY_EXPORT void mouse_position(int& x, int& y);

		LIBRARY_EXPORT void mouse_delta(int& dx, int& dy);
		LIBRARY_EXPORT void reset_delta()
		{
			cursor_delta[0] = cursor_delta[1] = 0;
		}
	}; // MouseInput

	class TouchInput : public InputDevice
	{
		struct TouchState
		{
			// (state & 1) -> isDown
			// (state & 2) -> isDragging
			unsigned int state;
			float x;
			float y;

			// [0] is the previous, [1] is current
			int xpos[2];
			int ypos[2];
		}; // TouchState

		TouchState touches[ MAX_INPUTSTATE_TOUCHES ];

	public:
		LIBRARY_EXPORT virtual void reset();
		LIBRARY_EXPORT virtual void update();

		LIBRARY_EXPORT void touch_began( int touchid, int x, int y );
		LIBRARY_EXPORT void touch_drag( int touchid, int x, int y );
		//	void touch_canceled( int touchid, int x, int y );
		LIBRARY_EXPORT void touch_end( int touchid, int x, int y );

		// retrieve touch from index
		LIBRARY_EXPORT void touch_at_index( TouchState & touch, int touch_id );
	}; // TouchInput


	class JoystickInput : public InputDevice
	{
	public:
		enum Flags
		{
			Disconnected 	= 0, // joystick disconnected / not used
			Connected 		= 1, // joystick is connected and enabled
			HapticSupport 	= 2 // joystick has haptics support
		};

		uint32_t flags;
		ButtonState buttons[MAX_JOYSTICK_BUTTONS];
		AxisState axes[MAX_JOYSTICK_AXES];

		LIBRARY_EXPORT virtual void reset();
		LIBRARY_EXPORT virtual void update();
	}; // JoystickInput


	LIBRARY_EXPORT void startup(void);
	LIBRARY_EXPORT void shutdown(void);
	LIBRARY_EXPORT void update(void);

	LIBRARY_EXPORT const char* mouse_button_name(unsigned int button);
	LIBRARY_EXPORT const char* key_name(unsigned int key);
	LIBRARY_EXPORT const char* gamepad_name(unsigned int button);
} // namespace input
