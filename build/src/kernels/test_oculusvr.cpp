// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone

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
#include <stdio.h>

#include <gemini/config.h>
#include <slim/xlog.h>

#include "kernel.h"
#include "renderer/renderer.h"
#include "input.h"

#include "OVR.h"

using namespace kernel;


namespace vr
{
	struct Device
	{
		ovrHmd hmd;
	};

	void startup()
	{
		ovr_Initialize();
	}
	
	int32_t detect_devices()
	{
		return ovrHmd_Detect();
	}
	
	Device create_device()
	{
		LOGV("creating an instance of a VR device...\n");
		ovrHmd hmd = ovrHmd_Create(0);
		
		if (hmd)
		{
			LOGV("VR device found!\n");
			
		}
		else
		{
			LOGV("no VR device found. creating a debug version...\n");
			hmd = ovrHmd_CreateDebug(ovrHmd_DK2);
		}
		
		// reach this and get a prize. couldn't even create a debug version...
		assert(hmd != 0);

		LOGV("VR device: %s\n", hmd->ProductName);
		LOGV("VR device manufacturer: %s\n", hmd->Manufacturer);

		
		Device device;
		device.hmd = hmd;
		return device;
	}
	
	void destroy_device(const Device& device)
	{
		ovrHmd_Destroy(device.hmd);
	}
	
	void shutdown()
	{
		ovr_Shutdown();
	}
	
	
}





class TestOculusVR : public kernel::IApplication,
	public IEventListener<KeyboardEvent>,
	public IEventListener<MouseEvent>,
	public IEventListener<SystemEvent>
{
public:
	DECLARE_APPLICATION( TestOculusVR );

	
	virtual void event( KeyboardEvent & event )
	{
        if ( event.is_down )
        {
			if (event.key == input::KEY_ESCAPE)
			{
				kernel::instance()->set_active(false);
			}
        }
	}

	virtual void event( MouseEvent & event )
	{
	}

	virtual void event( SystemEvent & event )
	{
	}
	
	virtual kernel::ApplicationResult config( kernel::Params & params )
	{
		params.window_width = 1280;
		params.window_height = 720;
		params.window_title = "TestOuclusVR";
		
		return kernel::Application_Success;
	}

	virtual kernel::ApplicationResult startup( kernel::Params & params )
	{
		vr::startup();
	
		LOGV("total devices: %i\n", vr::detect_devices());
		
		vr::Device dev;
		
		dev = vr::create_device();
		
		vr::destroy_device(dev);
	
		return kernel::Application_Success;
	}
	
	virtual void step( kernel::Params & params )
	{
	}

	virtual void tick( kernel::Params & params )
	{
		renderer::IRenderDriver * driver = renderer::driver();
		MemoryStream ms;
		char buffer[128] = {0};
		ms.init( buffer, 128 );
		
		// viewport
		ms.rewind();
		ms.write( 0 );
		ms.write( 0 );
		ms.write( params.window_width );
		ms.write( params.window_width );
		ms.rewind();
		driver->run_command( renderer::DC_VIEWPORT, ms );
		
		// set clear color
		ms.rewind();
		ms.write( 0.5f );
		ms.write( 0.0f );
		ms.write( 0.75f );
		ms.write( 1.0f );
		ms.rewind();
		driver->run_command( renderer::DC_CLEARCOLOR, ms );
		
		// color_buffer_bit
		ms.rewind();
		ms.write( 0x00004000 );
		ms.rewind();
		driver->run_command( renderer::DC_CLEAR, ms );
	}

	virtual void shutdown( kernel::Params & params )
	{
		vr::shutdown();
	}
};

IMPLEMENT_APPLICATION( TestOculusVR );