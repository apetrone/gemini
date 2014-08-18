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
		
		
		void query_display_resolution(int32_t& width, int32_t& height)
		{
			if (hmd)
			{
				ovrSizei resolution = hmd->Resolution;
				width = resolution.w;
				height = resolution.h;
			}
		}
	};

	void startup()
	{
		ovr_Initialize();
	}
	
	int32_t detect_devices()
	{
		return ovrHmd_Detect();
	}
	
	void setup_rendering(Device& device, uint16_t width, uint16_t height)
	{
		ovrSizei size;
		size.w = width;
		size.h = height;
		ovrRenderAPIConfig rc;
		rc.Header.API = ovrRenderAPI_OpenGL;
		rc.Header.Multisample = false;
		rc.Header.RTSize = size;
		
		float vfov = tan(90/2);
		float hfov = tan(100/2);
		
		unsigned int distortion_caps = 0;
		ovrFovPort eye_fov[2];
		eye_fov[0].UpTan = vfov;
		eye_fov[0].DownTan = vfov;
		eye_fov[0].LeftTan = hfov;
		eye_fov[0].RightTan = hfov;
		eye_fov[1].UpTan = vfov;
		eye_fov[1].DownTan = vfov;
		eye_fov[1].LeftTan = hfov;
		eye_fov[1].RightTan = hfov;
		
		ovrEyeRenderDesc eye_render[2];
		
		ovrBool result = ovrHmd_ConfigureRendering(
			device.hmd,
			&rc,
			distortion_caps,
			eye_fov,
			eye_render
		);
		LOGV("vr: setup rendering %s\n", result ? "Success": "Failed");
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

	vr::Device device;
	
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
		// try to setup VR device and resolution here.
		vr::startup();
		
		LOGV("total VR devices: %i\n", vr::detect_devices());
		device = vr::create_device();
	
		int32_t w, h;
		device.query_display_resolution(w, h);
		LOGV("creating window with resolution: %i x %i\n", w, h);
		
		params.window_width = (uint16_t)w;
		params.window_height = (uint16_t)h;
		params.window_title = "TestOculusVR";
		
		return kernel::Application_Success;
	}

	virtual kernel::ApplicationResult startup( kernel::Params & params )
	{
		vr::setup_rendering(device, params.render_width, params.render_height);
			
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