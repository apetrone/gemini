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

#include "vr.h"

using namespace kernel;



class TestOculusVR : public kernel::IApplication,
	public IEventListener<KeyboardEvent>,
	public IEventListener<MouseEvent>,
	public IEventListener<SystemEvent>
{
public:
	DECLARE_APPLICATION( TestOculusVR );

	vr::HeadMountedDevice* device;
	
	virtual void event( KeyboardEvent & event )
	{
        if ( event.is_down )
        {
			if (event.key == input::KEY_ESCAPE)
			{
				kernel::instance()->set_active(false);
			}
			else if (event.key == input::KEY_SPACE)
			{
				device->dismiss_warning();
			}
			else if (event.key == input::KEY_TAB)
			{
				device->reset_head_pose();
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
		
		LOGV("total VR devices: %i\n", vr::total_devices());
		device = vr::create_device();
	
		int32_t w, h;
		device->query_display_resolution(w, h);
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

	virtual void tick(kernel::Params& params)
	{
		renderer::IRenderDriver * driver = renderer::driver();
		util::MemoryStream ms;
		char buffer[256] = {0};
		ms.init(buffer, 256);

		device->begin_frame(driver);
		
		renderer::RenderTarget* rt = device->render_target();
		
		for (uint32_t eye_index = 0; eye_index < 2; ++eye_index)
		{
			vr::EyePose eye_pose = device->eye_pose_at(eye_index);

			int x;
			int y = 0;
			int width, height;

			width = rt->width/2;
			height = rt->height;
			
			if (eye_pose.is_left_eye())
			{
				x = 0;
				y = 0;
			}
			else if (eye_pose.is_right_eye())
			{
				x = width;
				y = 0;
			}
			
//			driver->viewport(x, y, width, height);
//			driver->clearcolor(0.0f, 0.5f, 0.5f, 1.0f);
//			driver->clear(renderer::COLOR_BUFFER);
			
			// viewport
			ms.rewind();
			ms.write( x );
			ms.write( y );
			ms.write( width );
			ms.write( height );
			ms.rewind();
			driver->run_command( renderer::DC_VIEWPORT, ms );
			
			// set clear color
			ms.rewind();
			ms.write( 0.0f );
			ms.write( 0.5f );
			ms.write( 0.5f );
			ms.write( 1.0f );
			ms.rewind();
			driver->run_command( renderer::DC_CLEARCOLOR, ms );

			// color_buffer_bit
			ms.rewind();
			ms.write( 0x00004000 );
			ms.rewind();
			driver->run_command( renderer::DC_CLEAR, ms );
		}

		device->end_frame(driver);
	}

	virtual void shutdown( kernel::Params & params )
	{
		vr::destroy_device(device);
		vr::shutdown();
	}
};

IMPLEMENT_APPLICATION( TestOculusVR );