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
		Event_CursorMove,
		Event_CursorDrag,
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
		//		Panel* sender;
		
		Point cursor;	// absolute (window) coordinates
		Point delta;	// delta coordinates
		Point local;	// local panel coordinates
		
		// keyboard modifiers
		uint32_t modifiers;
		

		
		CursorButton::Type cursor_button;
		Compositor* compositor;
		
	public:
		EventArgs(Compositor* compositor = 0, EventType event_type = Invalid) :
			handled(false),
			type(event_type),
			focus(0),
			hot(0),
			capture(0),
			modifiers(0)
		{
			this->compositor = compositor;
		}
		
		virtual ~EventArgs() {}
		
		
		EventArgs(const EventArgs& other)
		{
			*this = other;
		}
		
		const EventArgs& operator=(const EventArgs& other)
		{
			this->type = other.type;
			this->focus = other.focus;
			this->hot = other.hot;
			
			this->cursor = other.cursor;
			this->delta = other.delta;
			this->local = other.local;
			
			this->handled = other.handled;
			
			this->cursor_button = other.cursor_button;
			this->compositor = other.compositor;
			
			return *this;
		}
	};
	
	enum
	{
		EV_INVALID = 0,
		
		// low level events
		EV_MouseDown,
		EV_MouseUp,
		EV_MouseMove,
		
		EV_KeyDown,
		EV_KeyUp,
		
		EV_GainFocus,
		EV_LostFocus,
		
		EV_MouseEnter, // gain hot
		EV_MouseExit, // lost hot
		
		// higher level events, subscribable
		EV_CLICK,
		EV_DOUBLECLICK,
		EV_DRAG,
		
		EV_TEST,
		
		EV_LastEvent
	};

	
	struct PanelEvent
	{
		//bool handled; // set to true if an event was handled successfully
		Panel * target;
		unsigned int type;
		Point click; // where the first click occurred
		Point current; // current position
		Point mousePosition;
		int mouseButton;
		int unicode;
		int key;
		PanelEvent();

		InputState * inputState;
	}; // PanelEvent
	
	struct DragEventArgs : public PanelEvent
	{
		Point startDrag;
		Point deltaDrag;
	};
	
	struct DerivedPanelEvent : public PanelEvent
	{
		void * userdata;
		
		DerivedPanelEvent();
	}; // DerivedPanelEvent
	
	template <class ReturnType, class ParameterType>
	class Callback
	{
	public:
		virtual ReturnType execute( ParameterType parameter ) = 0;
	};
	
	
	
	
	
	
	class Listener
	{
	public:
		virtual ~Listener() {}
		
		virtual void focus_changed(Panel* old_focus, Panel* new_focus) = 0;
		virtual void hot_changed(Panel* old_hot, Panel* new_hot) = 0;
		
		virtual void handle_event(const EventArgs& event) = 0;
	};
	
} // namespace gui
