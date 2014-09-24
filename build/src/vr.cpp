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

#include <gemini/typedefs.h>
#include <gemini/mathlib.h>

#include <slim/xlog.h>
#include <slim/xstr.h>

#include "vr.h"

#define USE_OCULUS_RIFT 1

#if USE_OCULUS_RIFT
	#include "OVR.h"
	#include "OVR_CAPI_GL.h"
#endif

namespace vr
{
#if USE_OCULUS_RIFT
	struct OculusRift : public HeadMountedDevice
	{
		ovrHmd hmd;
		ovrPosef head_pose[2];
		ovrSizei render_target_size;
		ovrGLTexture textures[2];
		renderer::RenderTarget* rt;
		renderer::Texture* render_texture;
		ovrEyeRenderDesc eye_render[2];
				
		virtual uint32_t total_eyes() const
		{
			return ovrEye_Count;
		}
		
		virtual void begin_frame(renderer::IRenderDriver* renderer)
		{
			renderer->render_target_activate(this->rt);
			ovrHmd_BeginFrame(hmd, 0);
		}
		
		virtual void end_frame(renderer::IRenderDriver* renderer)
		{
			renderer->render_target_deactivate(this->rt);			
			ovrHmd_EndFrame(hmd, head_pose, (const ovrTexture*)textures);
		}
		
		virtual void query_display_resolution(int32_t& width, int32_t& height)
		{
			if (hmd)
			{
				ovrSizei resolution = hmd->Resolution;
				width = resolution.w;
				height = resolution.h;
			}
		}
		
		virtual EyePose eye_pose_at(uint32_t eye_index)
		{
			EyePose pose;
			
			ovrEyeType eye = hmd->EyeRenderOrder[eye_index];
			pose.eye_index = (eye == ovrEye_Left) ? 0 : 1;
			
			head_pose[eye] = ovrHmd_GetEyePose(hmd, eye);
			
			
			const ovrVector3f& translation = head_pose[eye].Position;
			pose.translation = glm::vec3(translation.x, translation.y, translation.z);
			
			const ovrQuatf& rotation = head_pose[eye].Orientation;
			pose.rotation = glm::quat(rotation.w, rotation.x, rotation.y, rotation.z);
			
			const ovrVector3f& adjust = eye_render[eye].ViewAdjust;
			pose.offset = glm::vec3(adjust.x, adjust.y, adjust.z);
			
			return pose;
		}
		
		virtual void dismiss_warning()
		{
			ovrHSWDisplayState warning;
			ovrHmd_GetHSWDisplayState(this->hmd, &warning);
			if (warning.Displayed)
			{
				LOGV("dismissing health and safety warning message\n");
				ovrHmd_DismissHSWDisplay(this->hmd);
			}
		}
		
		virtual void reset_head_pose()
		{
			LOGV("re-centering head pose\n");
			ovrHmd_RecenterPose(this->hmd);
		}
		
		virtual renderer::RenderTarget* render_target()
		{
			return this->rt;
		}
	};
#endif // USE_OCULUS_RIFT
	
	void startup()
	{
		ovr_Initialize();
	}
	
	void shutdown()
	{
		ovr_Shutdown();
	}
	
	int32_t total_devices()
	{
		return ovrHmd_Detect();
	}
	
	HeadMountedDevice* create_device()
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
		
		
		// try to setup tracking and other sensors
		ovrBool result = ovrHmd_ConfigureTracking(hmd, 0xFFFFFFFF, 0);
		if (!result)
		{
			LOGW("Failed to configure tracking!\n");
		}
		
		OculusRift* rift = CREATE(OculusRift);
		rift->hmd = hmd;
		
		return rift;
	}
	
	void destroy_device(HeadMountedDevice* device)
	{
		OculusRift* rift = static_cast<OculusRift*>(device);
		if (rift->render_texture)
		{
			renderer::driver()->texture_destroy(rift->render_texture);
		}
		
		if (rift->rt)
		{
			renderer::driver()->render_target_destroy(rift->rt);
		}
		ovrHmd_Destroy(rift->hmd);
		
		DESTROY(OculusRift, rift);
	}
	
	void setup_rendering(HeadMountedDevice* device, uint16_t width, uint16_t height)
	{
		vr::OculusRift* rift = static_cast<OculusRift*>(device);
		
		ovrSizei tex0 = ovrHmd_GetFovTextureSize(
												 rift->hmd,
												 ovrEye_Left,
												 rift->hmd->DefaultEyeFov[0],
												 1.0f
												 );
		
		ovrSizei tex1 = ovrHmd_GetFovTextureSize(
												 rift->hmd,
												 ovrEye_Right,
												 rift->hmd->DefaultEyeFov[1],
												 1.0f
												 );
		
		// determine combined width for both eyes
		// get max height of the two eyes (in case they differ)
		rift->render_target_size.w = tex0.w + tex1.w;
		rift->render_target_size.h = glm::max(tex0.h, tex1.h);
		
		LOGV("recommended render_target size: %i x %i\n",
			 rift->render_target_size.w,
			 rift->render_target_size.h
			 );
		
		rift->render_target_size.w = fmin(rift->render_target_size.w, width);
		rift->render_target_size.h = fmin(rift->render_target_size.h, height);
		
		// generate texture
		image::Image image;
		image.width = rift->render_target_size.w;
		image.height = rift->render_target_size.h;
		image.channels = 3;
		
		rift->render_texture = renderer::driver()->texture_create(image);
		
		// create render target
		rift->rt = renderer::driver()->render_target_create(
															 rift->render_target_size.w,
															 rift->render_target_size.h
															 );
		
		renderer::driver()->render_target_set_attachment(
														 rift->rt,
														 renderer::RenderTarget::COLOR,
														 0,
														 rift->render_texture
														 );
								
		renderer::driver()->render_target_set_attachment(
														rift->rt,
														 renderer::RenderTarget::DEPTHSTENCIL,
														 0,
														 0
														 );
		
		// TODO: call ovrHmd_AttachToWindow under win32?
		
		width = rift->render_target_size.w;
		height = rift->render_target_size.h;
		
		
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
		rift->textures[0].OGL.Header.API = ovrRenderAPI_OpenGL;
		rift->textures[0].OGL.Header.TextureSize = rift->render_target_size;
		rift->textures[0].OGL.Header.RenderViewport = viewport_left;
		rift->textures[0].OGL.TexId = rift->rt->color_texture_id;
		
		rift->textures[1].OGL.Header.API = ovrRenderAPI_OpenGL;
		rift->textures[1].OGL.Header.TextureSize = rift->render_target_size;
		rift->textures[1].OGL.Header.RenderViewport = viewport_right;
		rift->textures[1].OGL.TexId = rift->rt->color_texture_id;
		
		if (rift->hmd->HmdCaps & ovrHmdCap_ExtendDesktop)
		{
			LOGV("running in \"extended desktop\" mode\n");
		}
		else
		{
			LOGV("running in \"direct to hmd\" mode\n");
		}
		
		// try to enable low-persistence display and dynamic prediction
		ovrHmd_SetEnabledCaps(rift->hmd, ovrHmdCap_LowPersistence | ovrHmdCap_DynamicPrediction);
		
		ovrRenderAPIConfig rc;
		memset(&rc, 0, sizeof(ovrRenderAPIConfig));
		rc.Header.API = ovrRenderAPI_OpenGL;
		rc.Header.Multisample = false;
		rc.Header.RTSize = rift->render_target_size;
		
		unsigned int distortion_caps = 0;
//		distortion_caps |= ovrDistortionCap_Chromatic;
//		distortion_caps |= ovrDistortionCap_Vignette;
		distortion_caps |= ovrDistortionCap_TimeWarp;
		distortion_caps |= ovrDistortionCap_Overdrive;
		
		ovrBool result = ovrHmd_ConfigureRendering(
												   rift->hmd,
												   &rc,
												   distortion_caps,
												   rift->hmd->DefaultEyeFov,
												   rift->eye_render
												   );
		
		LOGV("vr: setup rendering -> %s\n", result ? "Success": "Failed");
	}
}
