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
		
		KeyboardButton,
		MouseButton,
		MouseMoved,
		MouseWheelMoved,
		
		TouchBegin,
		TouchMoved,
		TouchEnd,
	};
	
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