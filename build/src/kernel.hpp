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
	// kernel error codes
	enum Error
	{
		NoError = 0,
		CoreFailed = -1,
		PostConfig = -2,
		NoInstance = -3,
		ConfigFailed = -4,
		StartupFailed = -5
	};

	// Kernel flags for device details
	enum
	{
		DeviceDesktop 					= (1 << 0), // 'device' is a desktop computer
		DeviceiPhone 					= (1 << 1), // set if this is an iPhone
		DeviceiPad 						= (1 << 2), // set if this is an iPad
		DeviceSupportsRetinaDisplay 	= (1 << 3), // set if this device supports retina
	};

	// status codes for config and startup return values
	enum
	{
		Failure = 0,
		Success = 1,
		NoWindow = -1
	};

	// parameters passed to callbacks
	struct Params
	{
		enum
		{
			Invalid = 0,
			GainFocusEvent,
			LostFocusEvent,
			ResizeEvent,
		};
		
		int argc;
		char ** argv;
		const char * error_message;
		const char * window_title;
		
		// kDevice constants above describe the current system
		unsigned char device_flags;
	
		// dimensions of the actual window in pixels
		unsigned short window_width;
		unsigned short window_height;
		
		// dimensions of the rendering area in pixels
		unsigned short render_width;
		unsigned short render_height;
		unsigned short prev_width;
		unsigned short prev_height;
		unsigned char event_type;
		bool is_active;
	};
	
	

	class IApplication;
	class IKernel
	{
	public:
		virtual ~IKernel() {}

		virtual bool is_active() const = 0;
		virtual void set_active( bool isactive ) = 0;
		virtual kernel::Params & parameters() = 0;
		
		// these tick functions wrap the application's tick call
		virtual void pre_tick() = 0;
		virtual void post_tick() = 0;
		
		// called after the IApplication's config() call returns successfully
		virtual kernel::Error post_application_config() = 0;
	};

	class IApplication
	{
	public:
		virtual ~IApplication() {}
		
		virtual int config( kernel::Params & params ) = 0; // return kSuccess, kFailure, or kNoWindow
		virtual int startup( kernel::Params & params ) = 0; // return kSuccess, kFailure, or kNoWindow
		virtual void tick( kernel::Params & params ) = 0; // called every frame
	};
	
	typedef IApplication * (*ApplicationCreator)();
	struct Registrar
	{
		Registrar( const char * name, ApplicationCreator fn );
	};
	
	//
	// kernel registration / search
	#define DECLARE_APPLICATION( className ) \
		public: static IApplication * create() { return new className; }\
		public: virtual const char * classname() { return #className; }

	#define IMPLEMENT_APPLICATION( className ) \
		kernel::Registrar kr_##className( #className, className::create )
	
	// main loop for a desktop app; this manages the main loop itself.
	// it's enough in a desktop application to simply hand off control to this function.
	Error main( int argc, char ** argv, IKernel * kernel_instance, const char * application_name );
	
	Error startup( int argc, char ** argv, const char * application_name );
	void shutdown();
	void tick();
	
	IKernel * instance();
}; // namespace kernel
