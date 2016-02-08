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
#include "graphics/win32/win32_graphics_provider.h"

#include "platform_internal.h"
#include <core/typedefs.h>
#include <core/logging.h>

#include "kernel.h"

namespace platform
{
	namespace window
	{
		Win32WindowProvider* _window_provider;
		Win32GraphicsProvider* _graphics_provider;

		namespace win32
		{
			struct screen_rect_info
			{
				size_t target_screen_index;
				size_t current_screen_index;
				Frame frame;
			};

			BOOL CALLBACK get_screen_frame(HMONITOR,
				HDC,
				LPRECT monitor_rect,
				LPARAM data)
			{
				screen_rect_info* info = reinterpret_cast<screen_rect_info*>(data);
				if (info->current_screen_index == info->target_screen_index)
				{
					info->frame.x = static_cast<float>(monitor_rect->left);
					info->frame.y = static_cast<float>(monitor_rect->top);
					info->frame.width = static_cast<float>(monitor_rect->right - monitor_rect->left);
					info->frame.height = static_cast<float>(monitor_rect->bottom - monitor_rect->top);

					// Stop enumerating monitors
					return FALSE;
				}

				info->current_screen_index++;

				// Keep enumerating monitors until we find the requested screen
				return TRUE;
			}

			BOOL CALLBACK count_all_screens(HMONITOR,
				HDC,
				LPRECT,
				LPARAM data)
			{
				size_t* total_screens = reinterpret_cast<size_t*>(data);
				(*total_screens)++;
				// continue enumerating monitors
				return TRUE;
			}

			LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
			{
				// fetch our window pointer from hwnd
				win32::Window* window = reinterpret_cast<win32::Window*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
				if (window)
				{
					switch (message)
					{
						// handle mouse events
						case WM_MOUSEMOVE:
						{
							kernel::MouseEvent mevent;
							mevent.mx = LOWORD(lparam);
							mevent.my = HIWORD(lparam);
							mevent.wheel_direction = 0;
							mevent.window = window;
							mevent.subtype = kernel::MouseMoved;
							kernel::event_dispatch(mevent);
							break;
						}

						case WM_MOUSEWHEEL:
						{
							// -1 is towards the user
							// 1 is away from the user
							kernel::MouseEvent mevent;
							mevent.mx = LOWORD(lparam);
							mevent.my = HIWORD(lparam);
							mevent.wheel_direction = (static_cast<int16_t>(HIWORD(wparam)) > 0) ? 1 : -1;
							mevent.window = window;
							mevent.subtype = kernel::MouseWheelMoved;
							kernel::event_dispatch(mevent);
							break;
						}

						// handle keyboard events

						// handle system events
						case WM_SETFOCUS:
						case WM_KILLFOCUS:
						{
							kernel::SystemEvent sysevent;
							sysevent.subtype = (message == WM_KILLFOCUS) ? kernel::WindowLostFocus : kernel::WindowGainFocus;
							sysevent.window = window;
							kernel::event_dispatch(sysevent);
							break;
						}

						case WM_DESTROY:
						case WM_CLOSE:
						{
							if (window->get_destroy_behavior() == DestroyWindowBehavior::None)
							{
								kernel::SystemEvent sysevent;
								sysevent.subtype = kernel::WindowClosed;
								sysevent.window = window;
								kernel::event_dispatch(sysevent);
							}
							break;
						}

						case WM_SIZE:
						{
							kernel::SystemEvent sysevent;
							sysevent.subtype = kernel::WindowResized;
							sysevent.render_width = sysevent.window_width = static_cast<int16_t>(LOWORD(lparam));
							sysevent.render_height = sysevent.window_height = static_cast<int16_t>(HIWORD(lparam));
							sysevent.window = window;
							kernel::event_dispatch(sysevent);
							break;
						}
					} // switch
				} // window

				return DefWindowProc(hwnd, message, wparam, lparam);
			} // WindowProc
		} // namespace win32

		Result Win32WindowProvider::startup()
		{
			return Result::success();
		}

		void Win32WindowProvider::shutdown()
		{
		}

		NativeWindow* Win32WindowProvider::create(const Parameters& parameters, void* /*native_visual*/)
		{
			const wchar_t WINDOW_CLASS_NAME[] = L"platform_window_class";
			const size_t WINDOW_MAX_LENGTH = 128;

			win32::Window* window = MEMORY_NEW(win32::Window, get_platform_allocator())(parameters);
			window->set_destroy_behavior(DestroyWindowBehavior::None);

			// a new window must have a valid title
			assert(parameters.window_title);

			// a new window must have a valid height
			assert(parameters.frame.height > 0);

			// test to see if we already have a valid window class registered
			WNDCLASSEX window_class_info;
			if (GetClassInfoEx(0, WINDOW_CLASS_NAME, &window_class_info) == 0)
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
				window_class_info.lpszClassName = WINDOW_CLASS_NAME;
				window_class_info.lpszMenuName = NULL;

				window_class_info.style = CS_DBLCLKS | CS_HREDRAW | CS_OWNDC | CS_VREDRAW;

				RegisterClassEx(&window_class_info);
			}


			DWORD window_style = WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
			int window_width = 0;
			int window_height = 0;

			if (!parameters.enable_fullscreen)
			{
				RECT new_rect = {
					0,
					0,
					static_cast<LONG>(parameters.frame.width),
					static_cast<LONG>(parameters.frame.height)
				};

				window_style |= WS_MINIMIZEBOX | WS_CAPTION | WS_BORDER | WS_SYSMENU;
				if (parameters.enable_resize)
				{
					window_style |= WS_OVERLAPPEDWINDOW;
				}

				AdjustWindowRect(&new_rect, window_style, 0);
				window_width = static_cast<int>(new_rect.right - new_rect.left);
				window_height = static_cast<int>(new_rect.bottom - new_rect.top);
			}
			else
			{
				window_style |= WS_POPUP;
			}

			const char* utf8_title = parameters.window_title;
			wchar_t window_title[WINDOW_MAX_LENGTH];
			memset(window_title, 0, WINDOW_MAX_LENGTH);
			MultiByteToWideChar(CP_UTF8, 0, utf8_title, -1, window_title, WINDOW_MAX_LENGTH);

			HWND window_handle = CreateWindow(
				WINDOW_CLASS_NAME,
				window_title,
				window_style,
				static_cast<int>(parameters.frame.x),
				static_cast<int>(parameters.frame.y),
				window_width,
				window_height,
				NULL,
				NULL,
				GetModuleHandle(NULL),
				NULL
			);

			assert(window_handle);
			window->set_handle(window_handle);

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

			ShowWindow(window_handle, SW_SHOW);
			SetForegroundWindow(window_handle);
			UpdateWindow(window_handle);

			// store our window pointer as userdata
			SetWindowLongPtr(window_handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));

			return window;
		}

		void Win32WindowProvider::destroy(NativeWindow* window, DestroyWindowBehavior behavior)
		{
			win32::Window* native_window = static_cast<win32::Window*>(window);
			native_window->set_destroy_behavior(behavior);
			HWND window_handle = static_cast<HWND>(native_window->get_native_handle());
			DestroyWindow(window_handle);
			MEMORY_DELETE(native_window, get_platform_allocator());
		}

		Frame Win32WindowProvider::get_frame(NativeWindow* window) const
		{
			Frame window_frame;
			RECT rect;
			GetClientRect(static_cast<HWND>(window->get_native_handle()), &rect);
			window_frame.x = static_cast<float>(rect.left);
			window_frame.y = static_cast<float>(rect.top);
			window_frame.width = static_cast<float>(rect.right - rect.left);
			window_frame.height = static_cast<float>(rect.bottom - rect.top);
			return window_frame;
		}

		Frame Win32WindowProvider::get_render_frame(NativeWindow* window) const
		{
			return get_frame(window);
		}

		size_t Win32WindowProvider::get_screen_count() const
		{
			size_t total_screens = 0;
			EnumDisplayMonitors(nullptr,
				nullptr,
				win32::count_all_screens,
				reinterpret_cast<LPARAM>(&total_screens)
			);

			return total_screens;
		}

		Frame Win32WindowProvider::get_screen_frame(size_t screen_index) const
		{
			win32::screen_rect_info info;
			info.target_screen_index = screen_index;
			info.current_screen_index = 0;

			EnumDisplayMonitors(nullptr,
				nullptr,
				win32::get_screen_frame,
				reinterpret_cast<LPARAM>(&info)
			);

			return info.frame;
		}

		void Win32WindowProvider::dispatch_events()
		{
			MSG message;
			while (PeekMessage(&message, NULL, NULL, NULL, PM_REMOVE))
			{
				TranslateMessage(&message);
				DispatchMessage(&message);
			}
		}


		Result startup(RenderingBackend)
		{
			// create the win32 window provider instance
			_window_provider = MEMORY_NEW(Win32WindowProvider, get_platform_allocator());

			// out of memory?
			assert(_window_provider != nullptr);


			Result window_startup = _window_provider->startup();
			if (window_startup.failed())
			{
				LOGE("window_provider startup failed with error: %s\n", window_startup.message);
				return window_startup;
			}

			// try to create the graphics provider
			_graphics_provider = MEMORY_NEW(Win32GraphicsProvider, get_platform_allocator());

			// out of memory?
			assert(_graphics_provider);

			Result graphics_startup = _graphics_provider->startup(_window_provider);
			if (graphics_startup.failed())
			{
				LOGE("graphics_provider startup failed with error: %s\n", graphics_startup.message);
				return graphics_startup;
			}

			return Result::success();
		}

		void shutdown()
		{
			_graphics_provider->shutdown(_window_provider);
			MEMORY_DELETE(_graphics_provider, get_platform_allocator());

			_window_provider->shutdown();
			MEMORY_DELETE(_window_provider, get_platform_allocator());
		}

		void dispatch_events()
		{
			_window_provider->dispatch_events();
		}

		NativeWindow* create(const Parameters& window_parameters)
		{
			// allocate data for the graphics provider
			size_t graphics_data_size = _graphics_provider->get_graphics_data_size();
			void* graphics_data = nullptr;

			if (graphics_data_size)
			{
				// alloc graphics data for this window
				graphics_data = MEMORY_ALLOC(graphics_data_size, get_platform_allocator());
				memset(graphics_data, 0, sizeof(graphics_data_size));
			}

			// have the graphics provider figure out what it may need prior
			// to window creation.
			_graphics_provider->pre_window_creation(window_parameters, graphics_data);

			// create the native window and assign the graphics data
			NativeWindow* window = _window_provider->create(window_parameters, _graphics_provider->get_native_visual(graphics_data));
			window->graphics_data = graphics_data;
			window->backbuffer = window_parameters.backbuffer;

			// pass the window to the graphics API for context creation
			_graphics_provider->create_context(window);

			// another pass to create the 'surface'
			_graphics_provider->create_surface(window);

			// activate the context for newly created windows
			_graphics_provider->attach_context(window);

			return window;
		}


		void destroy(NativeWindow* window, DestroyWindowBehavior behavior)
		{
			_graphics_provider->detach_context(window);
			_graphics_provider->destroy_surface(window);
			_graphics_provider->destroy_context(window);

			MEMORY_DEALLOC(window->graphics_data, get_platform_allocator());
			_window_provider->destroy(window, behavior);
		}

		void activate_context(NativeWindow* window)
		{
			_graphics_provider->attach_context(window);
		}

		void deactivate_context(NativeWindow* window)
		{
			_graphics_provider->detach_context(window);
		}

		void swap_buffers(NativeWindow* window)
		{
			_graphics_provider->swap_buffers(window);
		}

		Frame get_frame(NativeWindow* window)
		{
			return _window_provider->get_frame(window);
		}

		Frame get_render_frame(NativeWindow* window)
		{
			return _window_provider->get_render_frame(window);
		}

		size_t screen_count()
		{
			return _window_provider->get_screen_count();
		}

		Frame screen_frame(size_t screen_index)
		{
			return _window_provider->get_screen_frame(screen_index);
		}

		void focus(NativeWindow* native_window)
		{
			win32::Window* window = static_cast<win32::Window*>(native_window);
			SetFocus(static_cast<HWND>(window->get_native_handle()));
		}

		void show_cursor(bool show_cursor)
		{
			ShowCursor(show_cursor);
		}

		void set_cursor(float x, float y)
		{
			SetCursorPos(static_cast<int>(x), static_cast<int>(y));
		}

		void get_cursor(float& x, float& y)
		{
			POINT point;
			GetCursorPos(&point);
			x = static_cast<float>(point.x);
			y = static_cast<float>(point.y);
		}

		// if enabled, the OS will generate delta mouse movement events
		void set_relative_mouse_mode(bool) {}

		void set_mouse_tracking(bool) {}
	} // namespace window
} // namespace platform
