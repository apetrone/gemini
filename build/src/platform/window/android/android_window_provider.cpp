// -------------------------------------------------------------
// Copyright (C) 2015- Adam Petrone
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
#include "android_backend.h"
#include "android_window_provider.h"
#include <android/native_window.h> // requires ndk r5 or higher
// #include <android/native_window_jni.h>
// 
// PLATFORM_SUPPORT_EGL and PLATFORM_SUPPORT_OPENGLES must be defined
#if !(defined(PLATFORM_EGL_SUPPORT) || !defined(PLATFORM_GLES2_SUPPORT))
	#error Android requires EGL and OpenGL ES support!
#endif

namespace platform
{
	namespace window
	{
		struct AndroidWindow : public NativeWindow
		{
			AndroidWindow(const WindowDimensions& window_dimensions,
				ANativeWindow* android_window) :
				NativeWindow(window_dimensions),
				native_window(android_window)
			{				
			}

			ANativeWindow* native_window;

			virtual void* get_native_handle() const override
			{
				return native_window;
			}

			virtual void update_visual(int visual_id) override
			{
				// EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
				// guaranteed to be accepted by ANativeWindow_setBuffersGeometry().				
				// As soon as we picked a EGLConfig, we can safely reconfigure the
				// ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID.
				ANativeWindow_setBuffersGeometry(native_window, 0, 0, visual_id);
			}
		}; // struct DispManXWindow


		AndroidWindowProvider::AndroidWindowProvider()
		{
		}

		AndroidWindowProvider::~AndroidWindowProvider()
		{
		}


		Result AndroidWindowProvider::startup()
		{
			return Result(Result::Success);
		}

		void AndroidWindowProvider::shutdown()
		{
		}

		NativeWindow* AndroidWindowProvider::create(const Parameters& parameters)
		{
			AndroidWindow* window = MEMORY_NEW(AndroidWindow, platform::get_platform_allocator())(parameters.window, get_android_window());
			return window;
		}

		void AndroidWindowProvider::destroy(NativeWindow* window)
		{
			AndroidWindow* pointer = static_cast<AndroidWindow*>(window);
			MEMORY_DELETE(pointer, platform::get_platform_allocator());
		}

		Frame AndroidWindowProvider::get_frame(NativeWindow* window) const
		{
			Frame frame;
			frame.width = window->dimensions.width;
			frame.height = window->dimensions.height;
			return frame;
		}

		Frame AndroidWindowProvider::get_render_frame(NativeWindow* window) const
		{
			Frame frame;
			frame.width = window->dimensions.width;
			frame.height = window->dimensions.height;
			return frame;
		}

		size_t AndroidWindowProvider::get_screen_count() const
		{
			return 1;
		}

		Frame AndroidWindowProvider::get_screen_frame(size_t screen_index) const
		{
			Frame frame;
			assert(screen_index == 0);
			frame.width = 0;
			frame.height = 0;
			return frame;
		}		
	} // namespace window
} // namespace platform
