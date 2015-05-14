// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone
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

#include <core/typedefs.h>
#include <core/mathlib.h>
#include <core/logging.h>

#include "vr.h"

#if GEMINI_WITH_OCULUSVR
	#include "OVR.h"
	#include "OVR_CAPI_GL.h"
#endif

#if defined(PLATFORM_WINDOWS)
	#if defined(min)
		#undef min
	#endif

	#if defined(max)
		#undef max
	#endif
#endif

namespace gemini
{
	namespace vr
	{
	#if GEMINI_WITH_OCULUSVR
		struct OculusRift : public HeadMountedDevice
		{
			ovrHmd hmd;
			ovrPosef head_pose[2];
			ovrSizei render_target_size;
			ovrGLTexture textures[2];
			renderer::RenderTarget* rt;
			renderer::Texture* render_texture;
			ovrEyeRenderDesc eye_render[2];
			ovrFrameTiming frame_timing;
			ovrTrackingState tracking_state;
			ovrPosef predicted_head_pose;
			
			virtual uint32_t total_eyes() const
			{
				return ovrEye_Count;
			}
			
			virtual void begin_frame(renderer::IRenderDriver* renderer)
			{
				renderer->render_target_activate(this->rt);
				frame_timing = ovrHmd_BeginFrame(hmd, 0);
				
				// check state
				tracking_state = ovrHmd_GetTrackingState(hmd, frame_timing.ScanoutMidpointSeconds);
				
				// TODO: key off statusflags to determine if tracking was lost
				
				// copy predicted head pose
				predicted_head_pose = tracking_state.HeadPose.ThePose;
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
			
			void copy_eye_pose(ovrPosef& pose, EyePose& out, ovrVector3f& hmdToEyeViewOffset)
			{
				const ovrVector3f& translation = pose.Position;
				out.translation = glm::make_vec3(&pose.Position.x);
				
				const ovrQuatf& rotation = pose.Orientation;
				out.rotation = glm::make_quat(&pose.Orientation.x);

				const ovrVector3f& adjust = hmdToEyeViewOffset;
				out.offset = glm::make_vec3(&adjust.x);
			}
			
			virtual void get_eye_poses(EyePose poses[2], glm::mat4 projections[2])
			{
				ovrVector3f hmdToEyeViewOffset[2] = { eye_render[ovrEye_Left].HmdToEyeViewOffset, eye_render[ovrEye_Right].HmdToEyeViewOffset };
				
				// unused for now
				int frame_index = 0;
				ovrHmd_GetEyePoses(hmd, frame_index, hmdToEyeViewOffset, head_pose, nullptr);
				
				//LOGV("TODO: we must scale head movement with IPD. See OWD sample\n");

				// copy eye poses
				copy_eye_pose(head_pose[0], poses[0], hmdToEyeViewOffset[0]);
				poses[0].eye_index = 0;
				
				copy_eye_pose(head_pose[0], poses[1], hmdToEyeViewOffset[1]);
				poses[1].eye_index = 1;
				
				
				// calculate projections
				float near_plane = 0.01f;
				float far_plane = 8192.0f;
				
				
				ovrMatrix4f eye_projections[2];
				eye_projections[ ovrEye_Left ] = ovrMatrix4f_Projection(hmd->DefaultEyeFov[ovrEye_Left], near_plane, far_plane, true);
				projections[ovrEye_Left] = glm::transpose(glm::make_mat4(&eye_projections[ovrEye_Left].M[0][0]));
				
				eye_projections[ ovrEye_Right ] = ovrMatrix4f_Projection(hmd->DefaultEyeFov[ovrEye_Right], near_plane, far_plane, true);
				projections[ovrEye_Right] = glm::transpose(glm::make_mat4(&eye_projections[ovrEye_Right].M[0][0]));
			}
			
			virtual void dismiss_warning()
			{
				ovrHSWDisplayState warning;
				ovrHmd_GetHSWDisplayState(this->hmd, &warning);
				if (warning.Displayed)
				{
					LOGV("dismissing health and safety warning message (mandatory 5s timeout)\n");
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
			
			virtual void test(glm::mat4& xform)
			{
				EyePose poses[2];
				glm::mat4 proj[2];
				get_eye_poses(poses, proj);
				
				xform = glm::toMat4(poses[0].rotation);
				xform = glm::translate(xform, poses[0].translation);
				xform = glm::inverse(xform);
			}
		};
	#endif // GEMINI_WITH_OCULUSVR
		
		void startup()
		{
	#if GEMINI_WITH_OCULUSVR
			ovr_Initialize();
	#endif
		}
		
		void shutdown()
		{
	#if GEMINI_WITH_OCULUSVR
			ovr_Shutdown();
	#endif
		}
		
		int32_t total_devices()
		{
	#if GEMINI_WITH_OCULUSVR
			return ovrHmd_Detect();
	#else
			return 0;
	#endif
		}
		
		HeadMountedDevice* create_device(int32_t index)
		{
	#if GEMINI_WITH_OCULUSVR
			LOGV("creating an instance of a VR device...\n");
			ovrHmd hmd = ovrHmd_Create(index);
			
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
			
			LOGV("Firmware version: %i.%i\n", hmd->FirmwareMajor, hmd->FirmwareMinor);
			LOGV("VR device: %s\n", hmd->ProductName);
			LOGV("VR device manufacturer: %s\n", hmd->Manufacturer);
			LOGV("Your Eye Height: %3.2f\n", ovrHmd_GetFloat(hmd, OVR_KEY_EYE_HEIGHT, 0));
			LOGV("Your IPD: %3.1fmm\n", ovrHmd_GetFloat(hmd, OVR_KEY_IPD, 0) * 1000.0f);
			
			// try to setup tracking and other sensors
			unsigned int sensor_flags = ovrTrackingCap_Orientation | ovrTrackingCap_MagYawCorrection | ovrTrackingCap_Position;
			ovrBool result = ovrHmd_ConfigureTracking(hmd, sensor_flags, 0);
			if (!result)
			{
				LOGW("Failed to configure tracking!\n");
			}
			
			OculusRift* rift = MEMORY_NEW(OculusRift, platform::memory::global_allocator());
			rift->hmd = hmd;
			
			return rift;
	#else
			return nullptr;
	#endif
		}
		
		void destroy_device(HeadMountedDevice* device)
		{
	#if GEMINI_WITH_OCULUSVR
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
			
			MEMORY_DELETE(rift, platform::memory::global_allocator());
	#else
	#endif
		}
		
		void setup_rendering(HeadMountedDevice* device, uint16_t width, uint16_t height)
		{
	#if GEMINI_WITH_OCULUSVR
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
			unsigned int enable_flags = 0;
			enable_flags |= ovrHmdCap_LowPersistence;
			enable_flags |= ovrHmdCap_DynamicPrediction;
			
			ovrHmd_SetEnabledCaps(rift->hmd, enable_flags);
			
			ovrRenderAPIConfig rc;
			memset(&rc, 0, sizeof(ovrRenderAPIConfig));
			rc.Header.API = ovrRenderAPI_OpenGL;
			rc.Header.Multisample = false;
			rc.Header.RTSize = rift->render_target_size;
			
			unsigned int distortion_caps = 0;
	//		distortion_caps |= ovrDistortionCap_Chromatic;
	//		distortion_caps |= ovrDistortionCap_Vignette;
			distortion_caps |= ovrDistortionCap_TimeWarp;
	//		distortion_caps |= ovrDistortionCap_Overdrive;
			distortion_caps |= ovrDistortionCap_SRGB;
			
			ovrBool result = ovrHmd_ConfigureRendering(
													   rift->hmd,
													   &rc,
													   distortion_caps,
													   rift->hmd->DefaultEyeFov,
													   rift->eye_render
													   );
			
			// TODO: for monoscoping rendering; remove rift->eye_render[].HmdToEyeViewOffset
			
			LOGV("vr: setup rendering -> %s\n", result ? "Success": "Failed");
	#endif
		}
	} // namespace vr
} // namespace gemini