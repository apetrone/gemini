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

#include "mem.h"
#include "kernel_events.h"

namespace kernel
{
	// kernel error codes
	enum Error
	{
		NoError = 0,
		CoreFailed = -1,
		PostConfigFailed = -2,
		NoInstance = -3,
		ConfigFailed = -4,
		StartupFailed = -5,
		ApplicationFailure = -6,
		RendererFailed = -7,
	}; // Error

	// Kernel flags for device details
	enum
	{
		DeviceDesktop 					= (1 << 0), // 'device' is a desktop computer
		DeviceiPhone 					= (1 << 1), // set if this is an iPhone
		DeviceiPad 						= (1 << 2), // set if this is an iPad
		DeviceSupportsRetinaDisplay 	= (1 << 3), // set if this device supports retina,
		DeviceAndroid					= (1 << 4), // Android-based device
	};

	// status codes for config and startup return values
	enum ApplicationResult
	{
		Application_Failure = 0,
		Application_Success = 1,
		Application_NoWindow = -1
	};
	
	typedef unsigned char KernelDeviceFlags;

	// parameters passed to callbacks
	struct Params
	{
		const char * error_message;
		const char * window_title;
		
		// kDevice constants above describe the current system
		KernelDeviceFlags device_flags;
		
		float step_interval_seconds;
		float step_alpha;
		float framedelta_filtered_msec;
		float framedelta_raw_msec;
		
		// dimensions of the actual window in pixels
		unsigned short window_width;
		unsigned short window_height;
		
		// dimensions of the rendering area in pixels
		unsigned short render_width;
		unsigned short render_height;
		unsigned short prev_width;
		unsigned short prev_height;
		unsigned char event_type;
		
		// need to take this into account when calculating screen coordinates
		unsigned short titlebar_height;
	}; // Params
	
	

	class IApplication;
	class IKernel
	{
	public:
		virtual ~IKernel() {}

		virtual bool is_active() const = 0;
		virtual void set_active( bool isactive ) = 0;
		virtual kernel::Params & parameters() = 0;

		// called first thing during setup; useful for initializing libraries
		virtual void startup() = 0;
		
		// this is called during startup to register systems specific to this kernel
		// it can be used to load or specify platform specific services
		virtual void register_services() = 0;
		
		// these tick functions wrap the application's tick call
		virtual void pre_tick() = 0;
		virtual void post_tick() = 0;
		
		// called after the IApplication's config() call returns successfully
		virtual void post_application_config( ApplicationResult result ) = 0;
		virtual void post_application_startup( ApplicationResult result ) = 0;
		
		// called right before control returns to the main entry point
		virtual void shutdown() = 0;
		
		// TODO: This feels like it doesn't fit here, move it.
		virtual void capture_mouse( bool capture ) = 0;
	};

	class IApplication
	{
	public:
		virtual ~IApplication() {}
		
		virtual ApplicationResult config( kernel::Params & params ) = 0;
		virtual ApplicationResult startup( kernel::Params & params ) = 0;
		virtual void step( kernel::Params & params ) = 0;
		virtual void tick( kernel::Params & params ) = 0; // called every frame
		virtual void shutdown( kernel::Params & params ) = 0;
	};
	
	typedef IApplication * (*ApplicationCreator)();
	struct Registrar
	{
		Registrar( const char * name, ApplicationCreator fn );
	};
	
	//
	// kernel registration / search
	#define DECLARE_APPLICATION( className ) \
		public: static IApplication * create() { return CREATE(className); }\
		public: virtual const char * classname() { return #className; }
	
	#define IMPLEMENT_APPLICATION( className ) \
		kernel::Registrar kr_##className( #className, className::create )
	
	Error startup( IKernel * kernel_instance );
	void shutdown();
	void update();
	void tick();
	

	IKernel * instance();

	
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
}; // namespace kernel