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

#include "kernel_events.h"
#include <core/typedefs.h>

namespace kernel
{
	// kernel error codes
	enum Error
	{
		NoError 			= 0,
		CoreFailed 			= -1,
		PostConfigFailed 	= -2,
		NoInstance 			= -3,
		ConfigFailed 		= -4,
		StartupFailed 		= -5,
		ApplicationFailure 	= -6,
		RendererFailed 		= -7,
	}; // Error

	// Kernel flags for device details
	enum
	{
		DeviceDesktop 					= (1 << 0), // 'device' is a desktop computer
		DeviceiPhone 					= (1 << 1), // set if this is an iPhone
		DeviceiPad 						= (1 << 2), // set if this is an iPad
		DeviceSupportsRetinaDisplay 	= (1 << 3), // set if this device supports retina
		DeviceAndroid					= (1 << 4), // Android-based device
	};
	
	typedef unsigned char KernelDeviceFlags;

	// parameters passed to callbacks
	struct LIBRARY_EXPORT Parameters
	{
		const char* error_message;
		
		// device constants above describe the current system
		KernelDeviceFlags device_flags;
		
		// time state
		double step_interval_seconds;
		float step_alpha;
		float framedelta_filtered_msec;
		float framedelta_filtered_seconds;
		float framedelta_raw_msec;
		
		
		// the current tick (physics step)
		uint64_t current_tick;
		
		uint64_t current_frame;
		
		// this is needed to allow normal rendering and Oculus Rift rendering.
		// the Rift SDK will swap buffers itself, which causes flickering
		// if we also do it. This defaults to true and shouldn't be
		// modified unless your kernel has custom needs for buffer swap.
		bool swap_buffers;
				
		// vertical sync
		bool use_vsync;


		//
		// DESKTOP-specific params
		//
		int argc;
		char ** argv;
		
		// has a valid window
		bool has_window;
		
		Parameters();
	}; // Params
	
	LIBRARY_EXPORT Parameters& parameters();

	class LIBRARY_EXPORT IKernel
	{
	public:
		virtual ~IKernel() {}

		virtual bool is_active() const = 0;
		virtual void set_active(bool isactive) = 0;

		virtual void resolution_changed(int width, int height) = 0;

		// called first thing during setup; useful for initializing libraries
		virtual Error startup() = 0;

		virtual void tick() = 0;
		
		// called right before control returns to the main entry point
		virtual void shutdown() = 0;

	};

	// call this on application startup
	LIBRARY_EXPORT Error startup();
	
	// call this when the application will be terminated
	LIBRARY_EXPORT void shutdown();

	// call this when the resolution of the window or device has changed
	LIBRARY_EXPORT void resolution_changed(int width, int height);
	
	// called once per frame, preferably in a loop
	LIBRARY_EXPORT void tick();
	
	LIBRARY_EXPORT int run_application();

	LIBRARY_EXPORT IKernel* instance();
	LIBRARY_EXPORT void set_instance(IKernel* instance);
	
	// this is used by the kernel to dispatch events to the IApplication's event listeners
	template <class Type>
	void event_dispatch( Type & event )
	{
		EventType event_type = Type::event_type;
		IEventListener<Type> * event_listener = (IEventListener<Type>*)find_listener_for_eventtype(event_type);
		if ( event_listener )
		{
			event_listener->event( event );
		}
	} // event_dispatch
} // namespace kernel