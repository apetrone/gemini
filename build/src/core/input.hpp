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
#pragma once

namespace input
{
	// this matches xwl key codes
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
		
		// windows key / command key
		KEY_LGUI,
		
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

		KEY_COUNT,
		
		MOUSE_LEFT = 104,
		MOUSE_RIGHT,
		MOUSE_MIDDLE,
		MOUSE_MOUSE4,
		MOUSE_MOUSE5,
		MOUSE_MOUSE6,
		MOUSE_MOUSE7,
		
		MOUSE_WHEEL,
		
		MOUSE_COUNT = (MOUSE_WHEEL-KEY_COUNT),
	}; // enum Button
	
	// key mods
	enum
	{
		KEYMOD_ALT = 1,
		KEYMOD_SHIFT = 2,
		KEYMOD_CONTROL = 4,
	};
	
	// This is a similar pattern to that found in Quake. I find it very logical to determining input state.
	enum
	{
		ISB_ISDOWN = 1,			// isDown this update
		ISB_WASDOWN = 2,		// wasDown last update
		ISB_JUST_RELEASED = 4,	// released this update
		ISB_IMPULSE = 8			// impulse flag; if set, isDown or released happened this update
	};
	
	#define MAX_INPUTSTATE_KEYS 320
	#define MAX_INPUTSTATE_BUTTONS 8
	#define MAX_INPUTSTATE_TOUCHES 10
	#define TOTAL_INPUTSTATE_KEYS (MAX_INPUTSTATE_KEYS+MAX_INPUTSTATE_BUTTONS)

	struct ButtonState
	{
		unsigned char state : 4;
		
		// handle a press or release event
		void press_release( bool is_down );
		
		// update this button state for this frame
		void update();
	}; // ButtonState
	
	
	class InputDevice
	{
	public:
		virtual ~InputDevice() {}
		
		virtual void reset() = 0;
		virtual void update( float delta_msec ) = 0;
	}; // InputDevice
	
	class KeyboardInput : public InputDevice
	{
		ButtonState keys[ KEY_COUNT ];
		
	public:
		virtual void reset();
		virtual void update( float delta_msec );
		
		void inject_key_event( int key, bool is_down, int unicode );
	
	
		// Accessors
		bool is_down( input::Button key );
		bool was_released( input::Button key );
	}; // KeyboardInput

	
	class MouseInput : public InputDevice
	{
		ButtonState buttons[ MOUSE_COUNT ];
		
		// multiple values equate to [0] previous, [1] current
		// thus: deltax = mousex[1] - mousex[0];
		int mousex[2];
		int mousey[2];
		
		int wheel_direction;
		
	public:
		virtual void reset();
		virtual void update( float delta_msec );
		
		void inject_mouse_move( int absolute_x, int absolute_y );
		void inject_mouse_button( int button_id, bool is_down );
		void inject_mouse_wheel( int direction );
		
		//
		// Accessors
		bool is_down( input::Button button_id );
		bool was_released( input::Button button_id );
		
		// retrieve the mouse position from last update
		void last_mouse_position( int & x, int & y );
		
		// retrieve the current mouse position
		void mouse_position( int & x, int & y );
	}; // MouseInput
	
	class TouchInput : public InputDevice
	{
		struct TouchState
		{
			// (state & 1) -> isDown
			// (state & 2) -> isDragging
			unsigned char state;
			float x;
			float y;
			
			// [0] is the previous, [1] is current
			int xpos[2];
			int ypos[2];
		}; // TouchState
		
		TouchState touches[ MAX_INPUTSTATE_TOUCHES ];
		
	public:
		virtual void reset();
		virtual void update( float delta_msec );
		
		void touch_began( int touchid, int x, int y );
		void touch_drag( int touchid, int x, int y );
		//	void touch_canceled( int touchid, int x, int y );
		void touch_end( int touchid, int x, int y );
		
		// retrieve touch from index
		void touch_at_index( TouchState & touch, int touch_id );
	}; // TouchInput
	
	// This is a facade interface which holds the current input state
	class InputState
	{
		KeyboardInput keyboard_input;
		MouseInput mouse_input;
		TouchInput touch_input;
	public:

		inline KeyboardInput & keyboard() { return keyboard_input; }
		inline MouseInput & mouse() { return mouse_input; }
		inline TouchInput & touch() { return touch_input; }
	}; // InputState
	
	InputState * state( void );
	void startup( void );
	void shutdown( void );
	void update( void );

	void buttonEvent( int button, int isDown );
	void handleEvent( ButtonState * b, int isDown );
	


}; // namespace input