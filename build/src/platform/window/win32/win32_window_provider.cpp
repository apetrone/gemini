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
#include "win32_window_provider.h"
#include "win32_window.h"

#include "platform_internal.h"
#include <core/typedefs.h>

namespace platform
{
	namespace window
	{
		namespace win32
		{
			const wchar_t WINDOW_CLASS_NAME[] = L"platform_window_class";
			const size_t WINDOW_MAX_LENGTH = 128;

			LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp)
			{
				return DefWindowProc(hwnd, message, wp, lp);
			} // WindowProc

			void create_context(win32::Window*)
			{

			} // create_context

			void destroy_context(win32::Window*)
			{

			} // destroy_context
		} // namespace win32


		Result startup(RenderingBackend)
		{
			return Result::success();
		}

		void shutdown()
		{
		}

		void dispatch_events()
		{

		}

		NativeWindow* create(const Parameters& window_parameters)
		{
			win32::Window* window = MEMORY_NEW(win32::Window, get_platform_allocator())(window_parameters);

			// a new window must have a valid title
			assert(window_parameters.window_title);

			// a new window must have a valid height
			assert(window_parameters.frame.height > 0);

			// test to see if we already have a valid window class registered
			WNDCLASSEX window_class_info;
			if (GetClassInfoEx(0, win32::WINDOW_CLASS_NAME, &window_class_info) == 0)
			{
				// class doesn't exist; let's create one
				window_class_info.cbSize = sizeof(WNDCLASSEX);
				window_class_info.cbClsExtra = 0;
				window_class_info.cbWndExtra = 0;

				window_class_info.hbrBackground = 0;
				window_class_info.hCursor = LoadCursor(NULL, IDC_ARROW);
				window_class_info.hIcon = LoadIcon(NULL, IDI_WINLOGO);
				window_class_info.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
				window_class_info.hInstance = static_cast<HINSTANCE>(GetModuleHandle(NULL));

				window_class_info.lpfnWndProc = win32::WindowProc;
				window_class_info.lpszClassName = win32::WINDOW_CLASS_NAME;
				window_class_info.lpszMenuName = NULL;

				window_class_info.style = CS_DBLCLKS | CS_HREDRAW | CS_OWNDC | CS_VREDRAW;

				RegisterClassEx(&window_class_info);
			}


			DWORD window_style = WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
			DWORD window_width = 0;
			DWORD window_height = 0;

			if (!window_parameters.enable_fullscreen)
			{
				RECT new_rect = {
					0,
					0,
					static_cast<LONG>(window_parameters.frame.width),
					static_cast<LONG>(window_parameters.frame.height)
				};

				window_style |= WS_MINIMIZEBOX | WS_CAPTION | WS_BORDER | WS_SYSMENU;
				if (window_parameters.enable_resize)
				{
					window_style |= WS_OVERLAPPEDWINDOW;
				}

				AdjustWindowRect(&new_rect, window_style, 0);
				window_width = static_cast<DWORD>(new_rect.right - new_rect.left);
				window_height = static_cast<DWORD>(new_rect.bottom - new_rect.top);
			}
			else
			{
				window_style |= WS_POPUP;
			}

			const char* utf8_title = window_parameters.window_title;
			wchar_t window_title[win32::WINDOW_MAX_LENGTH];
			memset(window_title, 0, win32::WINDOW_MAX_LENGTH);
			MultiByteToWideChar(CP_UTF8, 0, utf8_title, -1, window_title, win32::WINDOW_MAX_LENGTH);

			RECT client_rect;
			client_rect.left = 0;
			client_rect.right = static_cast<LONG>(window_width);
			client_rect.top = 0;
			client_rect.bottom = static_cast<LONG>(window_height);

			// ATI driver bug fix from irrlicht
			MoveWindow(static_cast<HWND>(window->get_native_handle()),
				client_rect.left,
				client_rect.top,
				client_rect.right,
				client_rect.bottom,
				TRUE);

			win32::create_context(window);

			return window;
		}

		void destroy(NativeWindow* window)
		{
			win32::Window* native_window = static_cast<win32::Window*>(window);

			win32::destroy_context(native_window);

			MEMORY_DELETE(native_window, get_platform_allocator());
		}

		void activate_context(NativeWindow*)
		{
		}

		// deactivate this window for rendering
		void deactivate_context(NativeWindow*)
		{
		}

		// swap buffers on this window
		void swap_buffers(NativeWindow*)
		{
		}

		// return the window size in screen coordinates
		Frame get_frame(NativeWindow*)
		{
			Frame frame;
			return frame;
		}

		// return the renderable window surface in pixels
		Frame get_render_frame(NativeWindow*)
		{
			Frame frame;
			return frame;
		}

		// total number of screens detected on this system
		size_t screen_count()
		{
			return 0;
		}

		/// @brief get the specified screen's rect (origin, width, and height) in pixels
		Frame screen_frame(size_t)
		{
			Frame frame;
			return frame;
		}

		// bring window to focus
		void focus(NativeWindow*)
		{}

		// show or hide the mouse cursor
		void show_cursor(bool)
		{}

		// set the cursor position in screen coordinates
		void set_cursor(float, float) {}

		// get the cursor position in screen coordinates
		void get_cursor(float&, float&) {}

		// if enabled, the OS will generate delta mouse movement events
		void set_relative_mouse_mode(bool) {}

		void set_mouse_tracking(bool) {}
	} // namespace window
} // namespace platform
