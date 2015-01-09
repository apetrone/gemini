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