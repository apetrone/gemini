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

#include <string.h>
#include <limits.h>

namespace kernel
{
	float GameControllerEvent::normalized_value() const
	{
		return (axis_value/(float)SHRT_MAX);
	}

	namespace detail
	{
		struct EventHooks
		{
			void* events[ kernel::EventTypeCount ];
			EventHooks();
		};

		EventHooks::EventHooks()
		{
			memset(events, 0, sizeof(void*) * kernel::EventTypeCount );
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




	void assign_listener_for_eventtype( kernel::EventType event_type, void * listener )
	{
		detail::event_hooks.events[ event_type ] = listener;
	} // assign_listener_for_eventtype

	void * find_listener_for_eventtype( kernel::EventType event_type )
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

		// we should default to swapping buffers ourself
		swap_buffers = 1;

		current_physics_tick = 0;
		current_frame = 0;
	}

	// prevent this from being emitted in every translation unit
	IKernel::~IKernel()
	{
	}


	Error startup()
	{
		// The kernel instance must be set before calling kernel::startup!
		assert(detail::kernel_instance != nullptr);

		// perform any startup duties here before we init the core
		Error result = detail::kernel_instance->startup();

		return result;
	} // startup

	void shutdown()
	{
		detail::kernel_instance->shutdown();
	}

	void tick()
	{
		detail::kernel_instance->tick();
	}

} // namespace kernel
