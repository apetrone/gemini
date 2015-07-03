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
		KEY_INVALID,
		
		KEY_A,
		KEY_B,
		KEY_C,
		KEY_D,
		KEY_E,
		KEY_F,
		KEY_G,
		KEY_H,
		KEY_I,
		KEY_J,
		KEY_K,
		KEY_L,
		KEY_M,
		KEY_N,
		KEY_O,
		KEY_P,
		KEY_Q,
		KEY_R,
		KEY_S,
		KEY_T,
		KEY_U,
		KEY_V,
		KEY_W,
		KEY_Y,
		KEY_X,
		KEY_Z,

		// other keys
		KEY_MENU,
		KEY_SEMICOLON,
		KEY_SLASH,
		KEY_BACKSLASH,
		KEY_EQUALS,
		KEY_MINUS,
		KEY_LBRACKET,
		KEY_RBRACKET,
		KEY_COMMA,
		KEY_PERIOD,
		KEY_QUOTE,
		KEY_TILDE,
		KEY_ESCAPE,
		KEY_SPACE,
		KEY_RETURN,
		KEY_BACKSPACE,
		KEY_TAB,
		KEY_PAGEUP,
		KEY_PAGEDN,
		KEY_END,
		KEY_HOME,
		KEY_INSERT,
		KEY_DELETE,
		KEY_PAUSE,
		
		KEY_LSHIFT,
		KEY_RSHIFT,
		KEY_LCONTROL,
		KEY_RCONTROL,
		KEY_LALT,
		KEY_RALT,
		KEY_NUMLOCK,
		KEY_CAPSLOCK,
		
		// windows key / command key (left/right variants)
		KEY_LOSKEY,
		KEY_ROSKEY,

		KEY_FUNCTION,

		// numeric keys
		KEY_0,
		KEY_1,
		KEY_2,
		KEY_3,
		KEY_4,
		KEY_5,
		KEY_6,
		KEY_7,
		KEY_8,
		KEY_9,
		
		// function keys
		KEY_F1,
		KEY_F2,
		KEY_F3,
		KEY_F4,
		KEY_F5,
		KEY_F6,
		KEY_F7,
		KEY_F8,
		KEY_F9,
		KEY_F10,
		KEY_F11,
		KEY_F12,
		KEY_F13,
		KEY_F14,
		KEY_F15,
		KEY_F16,
		KEY_F17,
		KEY_F18,
		KEY_F19,
		KEY_F20,
		
		// directional keys
		KEY_LEFT,
		KEY_RIGHT,
		KEY_UP,
		KEY_DOWN,
		
		// numpad keys
		KEY_NUMPAD0,
		KEY_NUMPAD1,
		KEY_NUMPAD2,
		KEY_NUMPAD3,
		KEY_NUMPAD4,
		KEY_NUMPAD5,
		KEY_NUMPAD6,
		KEY_NUMPAD7,
		KEY_NUMPAD8,
		KEY_NUMPAD9,
		KEY_NUMPAD_PLUS,
		KEY_NUMPAD_MINUS,
		KEY_NUMPAD_PLUSMINUS,
		KEY_NUMPAD_MULTIPLY,
		KEY_NUMPAD_DIVIDE,
		KEY_NUMPAD_PERIOD,
		KEY_NUMPAD_ENTER,
		KEY_NUMPAD_EQUALS,
		
		KEY_COUNT
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
		GAMEPAD_BUTTON_X,
		GAMEPAD_BUTTON_Y,
		GAMEPAD_BUTTON_BACK,
		GAMEPAD_BUTTON_GUIDE,
		GAMEPAD_BUTTON_START,
		GAMEPAD_BUTTON_LEFTSTICK,
		GAMEPAD_BUTTON_RIGHTSTICK,
		GAMEPAD_BUTTON_LEFTSHOULDER,
		GAMEPAD_BUTTON_RIGHTSHOULDER,
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
	
	struct LIBRARY_EXPORT ButtonState
	{
		unsigned char state : 4;
		
		// handle a press or release event
		void press_release( bool is_down );
		
		// update this button state for this frame
		void update();
		
		// returns whether the button is down or not this frame
		bool is_down() const;
		
		// returns whether or not the button was just pressed
		bool was_pressed() const;
		
		// returns whether or not the button was just released
		bool was_released() const;
	}; // ButtonState

	
	struct LIBRARY_EXPORT AxisState
	{
		float normalized_value;
		int16_t value;
	};
	
	class LIBRARY_EXPORT InputDevice
	{
	public:
		virtual ~InputDevice();
		
		virtual void reset() = 0;
		virtual void update() = 0;
	}; // InputDevice
	
	class LIBRARY_EXPORT KeyboardInput : public InputDevice
	{
		ButtonState keys[ KEY_COUNT ];
		
	public:
		virtual void reset();
		virtual void update();
		
		void inject_key_event(int key, bool is_down);
	
		inline const ButtonState& get_key(input::Button key) { return keys[key]; }
	
		// Accessors
//		bool is_down( input::Button key );
//		bool was_released( input::Button key );
	}; // KeyboardInput

	
	class LIBRARY_EXPORT MouseInput : public InputDevice
	{
		// absolute mouse position in window coordinates
		int window_coords[2];
		
		// relative mouse delta
		int delta[2];
		
		int wheel_direction;
		
		ButtonState buttons[ MOUSE_COUNT ];

		
	public:
		virtual void reset();
		virtual void update();
		
		void inject_mouse_move(int absolute_x, int absolute_y);
		void inject_mouse_delta(int delta_x, int delta_y);
		void inject_mouse_button( MouseButton button_id, bool is_down );
		void inject_mouse_wheel( int direction );
		
		//
		// Accessors
		bool is_down( MouseButton button );
		bool was_released( MouseButton button );
		
		// retrieve the current mouse position
		void mouse_position( int & x, int & y );
		
		void mouse_delta(int &dx, int &dy);
	}; // MouseInput
	
	class LIBRARY_EXPORT TouchInput : public InputDevice
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
		virtual void reset();
		virtual void update();
		
		void touch_began( int touchid, int x, int y );
		void touch_drag( int touchid, int x, int y );
		//	void touch_canceled( int touchid, int x, int y );
		void touch_end( int touchid, int x, int y );
		
		// retrieve touch from index
		void touch_at_index( TouchState & touch, int touch_id );
	}; // TouchInput
	

	class LIBRARY_EXPORT JoystickInput : public InputDevice
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
		
		virtual void reset();
		virtual void update();
	}; // JoystickInput


	LIBRARY_EXPORT void startup(void);
	LIBRARY_EXPORT void shutdown(void);
	LIBRARY_EXPORT void update(void);

	LIBRARY_EXPORT const char* mouse_button_name(unsigned int button);
	LIBRARY_EXPORT const char* key_name(unsigned int key);
	LIBRARY_EXPORT const char* gamepad_name(unsigned int button);

} // namespace input
