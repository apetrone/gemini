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
		AndroidWindow::AndroidWindow(ANativeWindow* android_window) :
			native_window(android_window),
			width(0),
			height(0)
		{
		}

		void* AndroidWindow::get_native_handle()
		{
			return native_window;
		}

		void AndroidWindow::set_native_handle(ANativeWindow* window)
		{
			native_window = window;
		}

		void AndroidWindow::update_visual(int visual_id)
		{
			// We need to cache this here because this is updated when
			// the context is updated. Which, in Android, is before we have
			// a valid Android window created.
			visual = visual_id;
		}

		void AndroidWindow::update_size(int width, int height)
		{
			this->width = width;
			this->height = height;
		}


		AndroidWindowProvider::AndroidWindowProvider() :
			main_window(nullptr)
		{
		}


		AndroidWindowProvider::~AndroidWindowProvider()
		{
		}

		Result AndroidWindowProvider::startup()
		{
			return Result::success();
		}

		void AndroidWindowProvider::shutdown()
		{
		}

		NativeWindow* AndroidWindowProvider::create(const Parameters& parameters, void* graphics_data)
		{
			return &main_window;
		}

		void AndroidWindowProvider::destroy(NativeWindow* window)
		{
		}

		Frame AndroidWindowProvider::get_frame(NativeWindow* window) const
		{
			Frame frame;
			frame.x = 0;
			frame.y = 0;
			frame.width = get_android_window()->width;
			frame.height = get_android_window()->height;
			return frame;
		}

		Frame AndroidWindowProvider::get_render_frame(NativeWindow* window) const
		{
			Frame frame;
			frame.x = 0;
			frame.y = 0;
			frame.width = get_android_window()->width;
			frame.height = get_android_window()->height;
			return frame;
		}

		size_t AndroidWindowProvider::get_screen_count() const
		{
			return 1;
		}

		Frame AndroidWindowProvider::get_screen_frame(size_t screen_index) const
		{
			// No support for multiple screens
			assert(screen_index == 0);
			Frame frame;
			frame.x = 0;
			frame.y = 0;
			frame.width = get_android_window()->width;
			frame.height = get_android_window()->height;
			return frame;
		}

		ANativeWindow* AndroidWindowProvider::get_native_window()
		{
			return static_cast<ANativeWindow*>(main_window.get_native_handle());
		}

		void AndroidWindowProvider::set_native_window(ANativeWindow* window)
		{
			main_window.set_native_handle(window);
		}

		AndroidWindow* AndroidWindowProvider::get_android_window()
		{
			return &main_window;
		}

		const AndroidWindow* AndroidWindowProvider::get_android_window() const
		{
			return &main_window;
		}
	} // namespace window
} // namespace platform
