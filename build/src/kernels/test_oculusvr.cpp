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
#include "OVR_CAPI_GL.h"
using namespace kernel;


namespace vr
{
	
	struct EyeDescription
	{
		
	};
	
	struct Device
	{
		ovrHmd hmd;
		ovrPosef head_pose[2];
		ovrSizei render_target_size;
		ovrGLTexture textures[2];
		renderer::RenderTarget* rt;
		renderer::Texture* render_texture;
		ovrEyeRenderDesc eye_render[2];
		
		Device()
		{
			rt = 0;
		}
		
		void query_display_resolution(int32_t& width, int32_t& height)
		{
			if (hmd)
			{
				ovrSizei resolution = hmd->Resolution;
				width = resolution.w;
				height = resolution.h;
			}
		}
		
		uint32_t get_eye_count()
		{
			return ovrEye_Count;
		}
		
		void begin_frame()
		{
			ovrHmd_BeginFrame(hmd, 0);
		}
		
		void end_frame()
		{
			ovrHmd_EndFrame(hmd, head_pose, (const ovrTexture*)textures);
		}
		
		EyeDescription get_eye_description(uint32_t eye_index)
		{
			ovrEyeType eye = hmd->EyeRenderOrder[eye_index];
			head_pose[eye_index] = ovrHmd_GetEyePose(hmd, eye);
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
		ovrSizei tex0 = ovrHmd_GetFovTextureSize(
												 device.hmd,
												 ovrEye_Left,
												 device.hmd->DefaultEyeFov[0],
												 1.0f
												 );
		
		ovrSizei tex1 = ovrHmd_GetFovTextureSize(
												 device.hmd,
												 ovrEye_Right,
												 device.hmd->DefaultEyeFov[1],
												 1.0f
												 );
		
		// determine combined width for both eyes
		// get max height of the two eyes (in case they differ)
		device.render_target_size.w = tex0.w + tex1.w;
		device.render_target_size.h = glm::max(tex0.h, tex1.h);
		
		LOGV("recommended render_target size: %i x %i\n",
			 device.render_target_size.w,
			 device.render_target_size.h
			 );
		
		device.render_target_size.w = fmin(device.render_target_size.w, width);
		device.render_target_size.h = fmin(device.render_target_size.h, height);
		
		// generate texture
		image::Image image;
		image.width = device.render_target_size.w;
		image.height = device.render_target_size.h;
		image.channels = 3;
		
		device.render_texture = renderer::driver()->texture_create(image);
		
		// create render target
		device.rt = renderer::driver()->render_target_create(
															 device.render_target_size.w,
															 device.render_target_size.h
															 );
		
		renderer::driver()->render_target_set_attachment(
														 device.rt,
														 renderer::RenderTarget::COLOR,
														 0,
														 device.render_texture
														 );
	
		// TODO: call ovrHmd_AttachToWindow under win32?
		
		width = device.render_target_size.w;
		height = device.render_target_size.h;
		
		
		ovrRecti viewport_left;
		viewport_left.Pos.x = 0;
		viewport_left.Pos.y = 0;
		viewport_left.Size.w = width/2;
		viewport_left.Size.h = height;
		
		ovrRecti viewport_right;
		viewport_right.Pos.x = width/2;
		viewport_right.Pos.y = 0;
		viewport_right.Size.w = width/2;
		viewport_right.Size.h = height;

		// now build textures
		device.textures[0].OGL.Header.API = ovrRenderAPI_OpenGL;
		device.textures[0].OGL.Header.TextureSize = device.render_target_size;
		device.textures[0].OGL.Header.RenderViewport = viewport_left;
		device.textures[0].OGL.TexId = device.rt->color_texture_id;

		device.textures[1].OGL.Header.API = ovrRenderAPI_OpenGL;
		device.textures[1].OGL.Header.TextureSize = device.render_target_size;
		device.textures[1].OGL.Header.RenderViewport = viewport_right;
		device.textures[1].OGL.TexId = device.rt->color_texture_id;
		
		if (device.hmd->HmdCaps & ovrHmdCap_ExtendDesktop)
		{
			LOGV("running in \"extended desktop\" mode\n");
		}
		else
		{
			LOGV("running in \"direct to hmd\" mode\n");
		}
		
		// try to enable low-persistence display and dynamic prediction
		ovrHmd_SetEnabledCaps(device.hmd, ovrHmdCap_LowPersistence | ovrHmdCap_DynamicPrediction);
		
		ovrRenderAPIConfig rc;
		memset(&rc, 0, sizeof(ovrRenderAPIConfig));
		rc.Header.API = ovrRenderAPI_OpenGL;
		rc.Header.Multisample = false;
		rc.Header.RTSize = device.render_target_size;

		unsigned int distortion_caps = ovrDistortionCap_Chromatic |
										ovrDistortionCap_Vignette |
										ovrDistortionCap_TimeWarp |
										ovrDistortionCap_Overdrive;
		
		ovrBool result = ovrHmd_ConfigureRendering(
												   device.hmd,
												   &rc,
												   distortion_caps,
												   device.hmd->DefaultEyeFov,
												   device.eye_render
												   );
								
		LOGV("vr: setup rendering -> %s\n", result ? "Success": "Failed");
	}
	
	void setup_sensors(Device& device)
	{
		ovrBool result = ovrHmd_ConfigureTracking(device.hmd, 0xFFFFFFFF, 0);
		if (!result)
		{
			LOGW("Failed to configure tracking!\n");
		}
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
		if (device.render_texture)
		{
			renderer::driver()->texture_destroy(device.render_texture);
		}
	
		if (device.rt)
		{
			renderer::driver()->render_target_destroy(device.rt);
		}
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
			else if (event.key == input::KEY_SPACE)
			{
				ovrHSWDisplayState warning;
				ovrHmd_GetHSWDisplayState(device.hmd, &warning);
				if (warning.Displayed)
				{
					LOGV("dismissing health and safety warning message\n");
					ovrHmd_DismissHSWDisplay(device.hmd);
				}
			}
			else if (event.key == input::KEY_TAB)
			{
				LOGV("re-centering head pose\n");
				ovrHmd_RecenterPose(device.hmd);
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
		
		vr::setup_sensors(device);
		
		return kernel::Application_Success;
	}
	
	virtual void step( kernel::Params & params )
	{
	}

	virtual void tick( kernel::Params & params )
	{
		renderer::IRenderDriver * driver = renderer::driver();
		util::MemoryStream ms;
		char buffer[256] = {0};
		ms.init(buffer, 256);

		device.begin_frame();
		
		driver->render_target_activate(device.rt);



		for (uint32_t eye_index = 0; eye_index < device.get_eye_count(); ++eye_index)
		{
			ovrEyeType eye_type = device.hmd->EyeRenderOrder[eye_index];
			vr::EyeDescription eye = device.get_eye_description(eye_index);
//			glm::quat orientation = device.get_eye_orientation(eye_index);

			
			int x;
			int y = 0;
			int width, height;

			width = device.rt->width/2;
			height = device.rt->height;
			
			if (eye_type == ovrEye_Left)
			{
				x = 0;
				y = 0;
			}
			else if (eye_type == ovrEye_Right)
			{
				x = width;
				y = 0;
			}
			
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

		driver->render_target_deactivate(device.rt);
		
		// viewport
		ms.rewind();
		ms.write( 0 );
		ms.write( 0 );
		ms.write( params.window_width );
		ms.write( params.window_height );
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

		device.end_frame();
	}

	virtual void shutdown( kernel::Params & params )
	{
		vr::destroy_device(device);
		vr::shutdown();
	}
};

IMPLEMENT_APPLICATION( TestOculusVR );