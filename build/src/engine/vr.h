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
#pragma once

#include <core/typedefs.h>
#include <core/mathlib.h>

#include "renderer/renderer.h"

namespace gemini
{
	namespace vr
	{
		struct EyePose
		{
			// eye pose information
			glm::vec3 translation;
			glm::quat rotation;
			
			// this offset takes the interpupilary distance into account
			glm::vec3 offset;
			
			// current index of this eye pose
			// *used to determine which eye it is)
			uint8_t eye_index;
			
			bool is_left_eye() const { return eye_index == 0; }
			bool is_right_eye() const { return eye_index == 1; }
		};
		
		struct HeadMountedDevice
		{
			HeadMountedDevice() {}
			virtual ~HeadMountedDevice() {}
			

			// I don't really see this changing.
			virtual uint32_t total_eyes() const = 0;
			
			virtual void begin_frame(gemini::renderer::IRenderDriver* renderer) = 0;
			virtual void end_frame(gemini::renderer::IRenderDriver* renderer) = 0;

			virtual void query_display_resolution(int32_t& width, int32_t& height) = 0;
			virtual void get_eye_poses(EyePose poses[2], glm::mat4 projections[2]) = 0;
			
			virtual void dismiss_warning() = 0;
			virtual void reset_head_pose() = 0;
			
			virtual gemini::renderer::RenderTarget* render_target() = 0;
			
			
			virtual void test(glm::mat4& xform) = 0;
		};

		
		// generic interface
		
		void startup();
		void shutdown();
		int32_t total_devices();
			
		HeadMountedDevice* create_device(int32_t index = 0);
		void destroy_device(HeadMountedDevice* device);
		
		// device operations
		void setup_rendering(HeadMountedDevice* device, uint16_t width, uint16_t height);

		// TODO: use these device flags to identify features
	//	enum DeviceFlags
	//	{
	//		None,
	//		
	//		SupportsHeadTranslation,
	//		SupportsHeadRotation
	//	};

		// TODO: Abstraction layer for an AR/VR library integration
	//	class VRInterface
	//	{
	//	public:
	//		virtual ~VRInterface() {}
	//	};
	} // namespace vr
} // namespace gemini