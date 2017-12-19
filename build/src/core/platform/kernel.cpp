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

#include "kernel.h"

#include <core/array.h>

#include <string.h>
#include <limits.h>


namespace kernel
{
	IKernel::IKernel()
		: active(1)
	{
	}

	bool IKernel::is_active() const
	{
		return active > 0;
	}

	void IKernel::set_active(bool isactive)
	{
		active = isactive;
	}

	float GameControllerEvent::normalized_value() const
	{
		return (axis_value/(float)SHRT_MAX);
	}

	namespace detail
	{
		struct EventHooks
		{
			IKernel* events[ kernel::EventTypeCount ];
			EventHooks();
		};

		EventHooks::EventHooks()
		{
			memset(events, 0, sizeof(IKernel*) * kernel::EventTypeCount);
		}

		EventHooks event_hooks;
		IKernel* kernel_instance = 0;
		Parameters parameters;
	} // namespace detail

	IKernel* instance()
	{
		return detail::kernel_instance;
	} // instance

	void set_instance(IKernel* instance)
	{
		detail::kernel_instance = instance;
	} // set_instance

	Parameters& parameters()
	{
		return detail::parameters;
	} // parameters




	void assign_listener_for_eventtype( kernel::EventType event_type, IKernel* listener )
	{
		detail::event_hooks.events[ event_type ] = listener;
	} // assign_listener_for_eventtype

	IKernel* find_listener_for_eventtype( kernel::EventType event_type )
	{
		return detail::event_hooks.events[ event_type ];
	} // find_listener_for_eventtype


	Parameters::Parameters()
	{
		error_message = 0;
		device_flags = 0;
		step_alpha = 0;
		step_interval_seconds = 0;
		use_vsync = true;

		simulation_time_scale = 1.0f;
		simulation_delta_seconds = 0.0f;

		// we should default to swapping buffers ourself
		swap_buffers = 1;

		current_physics_tick = 0;
		current_frame = 0;
	}

	// prevent this from being emitted in every translation unit
	IKernel::~IKernel()
	{
	}


	namespace detail
	{
		gemini::Allocator allocator;

		Array<gemini::InputMessage>* messages;
	} // namespace detail

	Error startup()
	{
		// The kernel instance must be set before calling kernel::startup!
		assert(detail::kernel_instance != nullptr);

		// perform any startup duties here before we init the core
		Error result = detail::kernel_instance->startup();

		detail::allocator = gemini::memory_allocator_default(gemini::MEMORY_ZONE_PLATFORM);
		detail::messages = MEMORY2_NEW(detail::allocator, Array<gemini::InputMessage>)(detail::allocator);

		return result;
	} // startup

	void shutdown()
	{
		detail::kernel_instance->shutdown();

		MEMORY2_DELETE(detail::allocator, detail::messages);
		detail::messages = nullptr;
	}

	//void tick()
	//{
	//	detail::kernel_instance->tick();
	//}
} // namespace kernel

namespace gemini
{
	KernelEvent::KernelEvent()
	{
		memset(this, 0, sizeof(KernelEvent));
	}

	void kernel_event_queue(KernelEvent &event)
	{
		InputMessage input_message;
		kernel_event_translate(input_message, event);
		kernel::detail::messages->push_back(input_message);
	} // kernel_event_queue

	void kernel_event_reset()
	{
		kernel::detail::messages->resize(0);
	} // kernel_event_reset

	void kernel_event_translate(InputMessage &output, KernelEvent &event)
	{
		uint64_t timestamp = kernel::parameters().current_physics_tick;
		switch (event.type)
		{
			case kernel::Keyboard:
			{
				output.timestamp = timestamp;
				output.type = InputMessage::Keyboard;
				output.button = event.key;
				output.params[0] = event.is_down;
				output.params[1] = event.modifiers;
				break;
			}
		}
	} // kernel_event_translate

	Array<InputMessage> &kernel_events()
	{
		return *kernel::detail::messages;
	} // kernel_events
} // namespace gemini
