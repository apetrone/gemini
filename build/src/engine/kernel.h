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
#include <platform/mem.h>
#include "kernel_events.h"

namespace gemini
{
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
		struct Parameters
		{
			const char* error_message;
			const char* window_title;
			
			// kDevice constants above describe the current system
			KernelDeviceFlags device_flags;
			
			double step_interval_seconds;
			float step_alpha;
			float framedelta_filtered_msec;
			float framedelta_raw_msec;
			
			// dimensions of the actual window in pixels
			unsigned short window_width;
			unsigned short window_height;
			
			// in windowed modes, this is the target display the window
			// will be transferred to
			unsigned short target_display;
			
			// dimensions of the rendering area in pixels
			unsigned short render_width;
			unsigned short render_height;
			unsigned short prev_width;
			unsigned short prev_height;
			unsigned char event_type;
			
			// need to take this into account when calculating screen coordinates
			unsigned short titlebar_height;
			
			// the current tick (physics step)
			uint64_t current_tick;
			
			uint64_t current_frame;
			
			// this is needed to allow normal rendering and Oculus Rift rendering.
			// the Rift SDK will swap buffers itself, which causes flickering
			// if we also do it.
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
			
			// set to true to create a fullscreen window
			bool use_fullscreen;


			Parameters();
		}; // Params
		
		Parameters& parameters();

		class IApplication;
		class IKernel
		{
		public:
			virtual ~IKernel() {}

			virtual bool is_active() const = 0;
			virtual void set_active( bool isactive ) = 0;
//			virtual kernel::Parameters& parameters() = 0;

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
			
			
			// until these are moved to the platform module:
			// TODO: This feels like it doesn't fit here, move it.
			virtual void capture_mouse( bool capture ) = 0;
			virtual void warp_mouse(int x, int y) = 0;
			virtual void get_mouse_position(int& x, int& y) = 0;
			virtual void show_mouse(bool show) = 0;
		};

		class IApplication
		{
		public:
			virtual ~IApplication() {}
			
			virtual ApplicationResult config(kernel::Parameters& params) = 0;
			virtual ApplicationResult startup(kernel::Parameters& params) = 0;
			virtual void step(kernel::Parameters& params) = 0;
			virtual void tick(kernel::Parameters& params) = 0; // called every frame
			virtual void shutdown(kernel::Parameters& params) = 0;
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
		void parse_commandline(int argc, char** argv);

		IKernel* instance();

		
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
} // namespace gemini