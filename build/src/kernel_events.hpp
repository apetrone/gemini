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

namespace kernel
{
	//
	// Event Types
	// These are all the event types recognized and translated by the kernel.
	// The Application must be able to handle a subset of these.
	//
	enum EventType
	{
		System,
		
		// common to desktop applications
		Keyboard,
		Mouse,
		
		// common to mobile
		Accelerometer,
		Gyroscope,
		Touch,
		
		JoystickAxis,
		JoystickButton,
		JoystickMotion,
		
		EventTypeCount
	}; // EventType
	
	// EventSubTypes do not need a separate event structure.
	// These provide the specific event for which to extract
	// information from the structure.
	enum EventSubType
	{
		WindowLostFocus,
		WindowGainFocus,
		WindowResized,
		
		KeyboardButton,
		MouseButton,
		MouseMoved,
		MouseWheelMoved,
		
		TouchBegin,
		TouchMoved,
		TouchEnd,
	}; // EventSubType
	
	//
	// These are all events used by the kernel. Any new events that a kernel should
	// support must be added here.
	//
	
	template <EventType type>
	struct Event
	{
		static const EventType event_type = type;
		EventSubType subtype;
	}; // Event
	
	struct SystemEvent : public Event<System>
	{
		short window_width;
		short window_height;
		short render_width;
		short render_height;
	}; // SystemEvent
	
	struct KeyboardEvent : public Event<Keyboard>
	{
		bool is_down;
		int unicode;
		int key;
	}; // KeyboardEvent

	struct MouseEvent : public Event<Mouse>
	{
		bool is_down;
		short button;
		int mx;
		int my;
		
		// -1 is movement towards the user; 1 is movement away from the user
		short wheel_direction;
	}; // MouseEvent

	
	struct TouchEvent : public Event<Touch>
	{
		int id;
		int x;
		int y;
	}; // TouchEvent
	
	//
	// event support classes
	//
	
	template <class Type>
	class IEventListener
	{
	public:
		virtual ~IEventListener() {}
		
		// called to handle an event of Type
		virtual void event( Type & event ) = 0;
	}; // IKernelEventListener
	
}; // namespace kernel