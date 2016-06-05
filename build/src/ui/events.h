// -------------------------------------------------------------
// Copyright (C) 2015- Adam Petrone
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


namespace gui
{
	class Panel;
	class Compositor;

	namespace CursorButton
	{
		enum Type
		{
			None,
			Left,
			Right,
			Middle,
			Mouse4,
			Mouse5,
		};
	}

	namespace KeyboardModifier
	{
		enum Type
		{
			None,

			Left_Control	= 1,
			Right_Control	= 2,
			Control			= 3,

			Left_Shift		= 4,
			Right_Shift		= 8,
			Shift			= 12,

			Left_Alt		= 16,
			Right_Alt		= 32,
			Alt				= 48
		};
	}

	enum EventType
	{
		Invalid = 0,
		Event_CursorEnter,
		Event_CursorExit,
		Event_FocusGain,
		Event_FocusLost,
		Event_CursorMove,
		Event_CursorDrag,				// panel is being dragged
		Event_CursorDragEnter,			// a drag enters this panel
		Event_CursorDragExit,			// a drag exits this panel
		Event_CursorDrop,				// dropped onto this panel
		Event_CursorScroll,
		Event_CursorButtonPressed,
		Event_CursorButtonReleased,

		Event_KeyButtonPressed,
		Event_KeyButtonReleased,

		Event_Click
	};

	class EventArgs
	{
	public:
		bool handled; // true if this event has been handled
		EventType type;
		Panel* focus; // this has focus
		Panel* hot; // mouse is on top of this
		Panel* capture; // captured panel

		// The panel from which this event originated.
		Panel* sender;

		// The panel to which this event is intended.
		Panel* target;

		Point cursor;	// compositor coordinates
		Point delta;	// delta coordinates
		Point local;	// local panel coordinates

		// keyboard modifiers
		uint32_t modifiers;

		CursorButton::Type cursor_button;

		// > 0 towards screen
		// < 0 towards user
		int32_t wheel;

		Compositor* compositor;

	public:
		EventArgs(Compositor* _compositor = nullptr, EventType event_type = Invalid)
			: handled(false)
			, type(event_type)
			, focus(nullptr)
			, hot(nullptr)
			, capture(nullptr)
			, sender(nullptr)
			, target(nullptr)
			, modifiers(0)
			, cursor_button(CursorButton::None)
			, wheel(0)
			, compositor(_compositor)
		{
		}

		EventArgs(const EventArgs& other)
		{
			*this = other;
		}

		const EventArgs& operator=(const EventArgs& other)
		{
			handled = other.handled;
			type = other.type;
			focus = other.focus;
			hot = other.hot;
			capture = other.capture;
			sender = other.sender;
			target = other.target;

			cursor = other.cursor;
			delta = other.delta;
			local = other.local;

			cursor_button = other.cursor_button;
			compositor = other.compositor;

			return *this;
		}
	}; // EventArgs

	class Listener
	{
	public:
		virtual ~Listener() {}

		virtual void focus_changed(Panel* old_focus, Panel* new_focus) = 0;
		virtual void hot_changed(Panel* old_hot, Panel* new_hot) = 0;

		virtual void handle_event(const EventArgs& event) = 0;
	};

} // namespace gui
