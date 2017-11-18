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

#include "input.h"
#include "kernel.h"

#include <windowsx.h> // GET_X_LPARAM, GET_Y_LPARAM

using namespace gemini;

namespace platform
{
	namespace window
	{
		Win32WindowProvider* _window_provider;
		Win32GraphicsProvider* _graphics_provider;
		RECT default_clip_rect;
		uint32_t disable_next_mouse_event = 0;

		namespace win32
		{
			struct screen_rect_info
			{
				size_t target_screen_index;
				size_t current_screen_index;
				Frame frame;
			};

			int32_t last_mousex = 0;
			int32_t last_mousey = 0;

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

			void handle_mouse_button(bool is_down, int mx, int my, NativeWindow* window, MouseButton mouse_button)
			{
				kernel::MouseEvent mevent;
				mevent.mx = mx;
				mevent.my = my;
				mevent.dx = (mevent.mx - last_mousex);
				last_mousex = mevent.mx;
				mevent.dy = (mevent.my - last_mousey);
				last_mousey = mevent.my;
				mevent.window = window;
				mevent.subtype = kernel::MouseButton;
				mevent.is_down = is_down;
				mevent.button = mouse_button;
				kernel::event_dispatch(mevent);
			}

			int handle_keyboard_event(win32::Window* window, UINT message, WPARAM wparam, LPARAM lparam)
			{
				// https://msdn.microsoft.com/en-us/library/windows/desktop/dd375731(v=vs.85).aspx

				// map windows virtual keys to gemini::Button
				const Button keymap[] = {
					Button::BUTTON_INVALID,
					Button::BUTTON_INVALID, // VK_LBUTTON
					Button::BUTTON_INVALID, // VK_RBUTTON
					Button::BUTTON_INVALID, // VK_CANCEL
					Button::BUTTON_INVALID, // VK_MBUTTON
					Button::BUTTON_INVALID, // VK_XBUTTON1
					Button::BUTTON_INVALID, // VK_XBUTTON2
					Button::BUTTON_INVALID, // undefined 0x07 unassigned
					Button::BUTTON_BACKSPACE,
					Button::BUTTON_TAB,
					Button::BUTTON_INVALID,	// reserved (0x0A-0x0B)
					Button::BUTTON_INVALID, // reserved
					Button::BUTTON_INVALID, // VK_CLEAR
					Button::BUTTON_RETURN,
					Button::BUTTON_INVALID, // undefined (0x0E-0x0F)
					Button::BUTTON_INVALID, // undefined
					Button::BUTTON_LSHIFT,
					Button::BUTTON_LCONTROL,
					Button::BUTTON_LALT,
					Button::BUTTON_PAUSE,
					Button::BUTTON_CAPSLOCK, // VK_CAPITAL
					Button::BUTTON_INVALID, // IME Kana/Hanguel mode; VK_KANA/VK_HANGUEL/VK_HANGUL
					Button::BUTTON_INVALID, // undefined
					Button::BUTTON_INVALID, // IME Junja mode; VK_JUNJA
					Button::BUTTON_INVALID, // IME final mode; VK_FINAL
					Button::BUTTON_INVALID, // IME Hanja/Kanji mode; VK_HANJA/VK_KANJI
					Button::BUTTON_INVALID, // undefined
					Button::BUTTON_ESCAPE,
					Button::BUTTON_INVALID, // IME convert; VK_CONVERT
					Button::BUTTON_INVALID, // IME nonconvert; VK_NONCONVERT
					Button::BUTTON_INVALID, // IME accept; VK_ACCEPT
					Button::BUTTON_INVALID, // IME mode change request; VK_MODECHANGE
					Button::BUTTON_SPACE,
					Button::BUTTON_PAGEUP,
					Button::BUTTON_PAGEDN,
					Button::BUTTON_END,
					Button::BUTTON_HOME,
					Button::BUTTON_LEFT,
					Button::BUTTON_UP,
					Button::BUTTON_RIGHT,
					Button::BUTTON_DOWN,
					Button::BUTTON_INVALID, // VK_SELECT
					Button::BUTTON_INVALID, // VK_PRINT
					Button::BUTTON_INVALID, // VK_EXECUTE
					Button::BUTTON_INVALID, // PRINT SCREEN key; VK_SNAPSHOT
					Button::BUTTON_INSERT,
					Button::BUTTON_DELETE,
					Button::BUTTON_INVALID, // VK_HELP
					Button::BUTTON_0,
					Button::BUTTON_1,
					Button::BUTTON_2,
					Button::BUTTON_3,
					Button::BUTTON_4,
					Button::BUTTON_5,
					Button::BUTTON_6,
					Button::BUTTON_7,
					Button::BUTTON_8,
					Button::BUTTON_9,
					Button::BUTTON_INVALID, // undefined
					Button::BUTTON_INVALID, // undefined
					Button::BUTTON_INVALID, // undefined
					Button::BUTTON_INVALID, // undefined
					Button::BUTTON_INVALID, // undefined
					Button::BUTTON_INVALID, // undefined
					Button::BUTTON_INVALID, // undefined
					Button::BUTTON_A,
					Button::BUTTON_B,
					Button::BUTTON_C,
					Button::BUTTON_D,
					Button::BUTTON_E,
					Button::BUTTON_F,
					Button::BUTTON_G,
					Button::BUTTON_H,
					Button::BUTTON_I,
					Button::BUTTON_J,
					Button::BUTTON_K,
					Button::BUTTON_L,
					Button::BUTTON_M,
					Button::BUTTON_N,
					Button::BUTTON_O,
					Button::BUTTON_P,
					Button::BUTTON_Q,
					Button::BUTTON_R,
					Button::BUTTON_S,
					Button::BUTTON_T,
					Button::BUTTON_U,
					Button::BUTTON_V,
					Button::BUTTON_W,
					Button::BUTTON_X,
					Button::BUTTON_Y,
					Button::BUTTON_Z,
					Button::BUTTON_LOSKEY,
					Button::BUTTON_ROSKEY,
					Button::BUTTON_MENU, // applications key
					Button::BUTTON_INVALID, // reserved
					Button::BUTTON_INVALID, // sleep key
					Button::BUTTON_NUMPAD0,
					Button::BUTTON_NUMPAD1,
					Button::BUTTON_NUMPAD2,
					Button::BUTTON_NUMPAD3,
					Button::BUTTON_NUMPAD4,
					Button::BUTTON_NUMPAD5,
					Button::BUTTON_NUMPAD6,
					Button::BUTTON_NUMPAD7,
					Button::BUTTON_NUMPAD8,
					Button::BUTTON_NUMPAD9,
					Button::BUTTON_NUMPAD_MULTIPLY,
					Button::BUTTON_NUMPAD_PLUS,
					Button::BUTTON_INVALID, // VK_SEPARATOR
					Button::BUTTON_NUMPAD_MINUS,
					Button::BUTTON_NUMPAD_PERIOD,
					Button::BUTTON_NUMPAD_DIVIDE,
					Button::BUTTON_F1,
					Button::BUTTON_F2,
					Button::BUTTON_F3,
					Button::BUTTON_F4,
					Button::BUTTON_F5,
					Button::BUTTON_F6,
					Button::BUTTON_F7,
					Button::BUTTON_F8,
					Button::BUTTON_F9,
					Button::BUTTON_F10,
					Button::BUTTON_F11,
					Button::BUTTON_F12,
					Button::BUTTON_F13,
					Button::BUTTON_F14,
					Button::BUTTON_F15,
					Button::BUTTON_F16,
					Button::BUTTON_F17,
					Button::BUTTON_F18,
					Button::BUTTON_F19,
					Button::BUTTON_F20,
					Button::BUTTON_F21,
					Button::BUTTON_F22,
					Button::BUTTON_F23,
					Button::BUTTON_F24,
					Button::BUTTON_INVALID, // unassigned (0x88-0x8F)
					Button::BUTTON_INVALID,
					Button::BUTTON_INVALID,
					Button::BUTTON_INVALID,
					Button::BUTTON_INVALID,
					Button::BUTTON_INVALID,
					Button::BUTTON_INVALID,
					Button::BUTTON_INVALID,
					Button::BUTTON_NUMLOCK,
					Button::BUTTON_SCROLLLOCK,
					Button::BUTTON_INVALID, // OEM specific (0x92-0x96)
					Button::BUTTON_INVALID,
					Button::BUTTON_INVALID,
					Button::BUTTON_INVALID,
					Button::BUTTON_INVALID,
					Button::BUTTON_INVALID, // 97-9F unassigned
					Button::BUTTON_INVALID,
					Button::BUTTON_INVALID,
					Button::BUTTON_INVALID,
					Button::BUTTON_INVALID,
					Button::BUTTON_INVALID,
					Button::BUTTON_INVALID,
					Button::BUTTON_INVALID,
					Button::BUTTON_INVALID, // 0x9F
					Button::BUTTON_LSHIFT,
					Button::BUTTON_RSHIFT,
					Button::BUTTON_LCONTROL,
					Button::BUTTON_RCONTROL,
					Button::BUTTON_LOSKEY, // VK_LMENU
					Button::BUTTON_ROSKEY, // VK_RMENU
					Button::BUTTON_INVALID, // VK_BROWSER_BACK
					Button::BUTTON_INVALID, // VK_BROWSER_FORWARD
					Button::BUTTON_INVALID, // VK_BROWSER_REFRESH
					Button::BUTTON_INVALID, // VK_BROWSER_STOP
					Button::BUTTON_INVALID, // VK_BROWSER_SEARCH
					Button::BUTTON_INVALID, // VK_BROWSER_FAVORITES
					Button::BUTTON_INVALID, // VK_BROWSER_HOME
					Button::BUTTON_INVALID, // VK_VOLUME_MUTE
					Button::BUTTON_INVALID, // VK_VOLUME_DOWN
					Button::BUTTON_INVALID, // VK_VOLUME_UP
					Button::BUTTON_INVALID, // VK_MEDIA_NEXT_TRACK
					Button::BUTTON_INVALID, // VK_MEDIA_PREV_TRACK
					Button::BUTTON_INVALID, // VK_MEDIA_STOP
					Button::BUTTON_INVALID, // VK_MEDIA_PLAY_PAUSE
					Button::BUTTON_INVALID, // VK_LAUNCH_MAIL
					Button::BUTTON_INVALID, // VK_LAUNCH_MEDIA_SELECT
					Button::BUTTON_INVALID, // VK_LAUNCH_APP1
					Button::BUTTON_INVALID, // VK_LAUNCH_APP2
					Button::BUTTON_INVALID, // Reserved (0xB8-0xB9)
					Button::BUTTON_INVALID,

					// All keys below are OEM reserved keys.

					Button::BUTTON_INVALID, // VK_OEM_1
					Button::BUTTON_PLUS, // VK_OEM_PLUS
					Button::BUTTON_INVALID, // VK_OEM_COMMA
					Button::BUTTON_MINUS, // VK_OEM_MINUS
					Button::BUTTON_INVALID, // VK_OEM_PERIOD
					Button::BUTTON_INVALID, // VK_OEM_2
					Button::BUTTON_INVALID, // VK_OEM_3
				};

				const size_t max_keys = sizeof(keymap) / sizeof(Button);

				if (wparam < max_keys)
				{
					const WPARAM vkey = wparam;
					const UINT scancode = static_cast<UINT>((lparam & 0xff0000) >> 16);
					const int extended = (lparam & (1 << 24)) != 0;
					const int is_repeat = (HIWORD(lparam) & KF_REPEAT) != 0;
					const int last_key = (lparam & (1 << 30)); // 0: was up; > 0 was down
					if (is_repeat && (message == WM_KEYDOWN))
					{
						return 0;
					}

					kernel::KeyboardEvent event;
					event.key = keymap[vkey];
					event.is_down = (last_key == 0) ? true : false;
					event.modifiers = 0;
					event.window = window;
					event.unicode = static_cast<uint32_t>(vkey);
					event.is_text = 0;
					kernel::event_dispatch(event);
					return 0;
				}
				else
				{
					// keymap is missing something
					LOGV("[win32] missing virtualkey in keymap: %i\n", wparam);
					return 1;
				}
			}

			void constrain_mouse_in_relative_mode(HWND hwnd, int32_t mousex, int32_t mousey)
			{
				if (_window_provider->in_relative_mode())
				{
					// Must reset the mouse coords here. However,
					// these are in local window coordinates.
					last_mousex = mousex;
					last_mousey = mousey;

					// We have to convert these to screen
					// coordinates in order to use them with the
					// win32 SetCursorPos API.
					POINT new_mouse;
					new_mouse.x = mousex;
					new_mouse.y = mousey;
					ClientToScreen(hwnd, &new_mouse);

					set_cursor(static_cast<float>(new_mouse.x), static_cast<float>(new_mouse.y));
				}
			}

			const gemini::MouseButton xmouse[] = {
				gemini::MouseButton::MOUSE_INVALID,
				gemini::MouseButton::MOUSE_MOUSE5,
				gemini::MouseButton::MOUSE_MOUSE4
			};

			LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
			{
				// fetch our window pointer from hwnd
				win32::Window* window = reinterpret_cast<win32::Window*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
				if (window)
				{
					const int32_t prev_mouse[2] = { last_mousex, last_mousey };



					switch (message)
					{
					case WM_INPUT:
					{
						// handle raw input
						UINT size = 0;
						BYTE data[48];

						UINT raw_result = GetRawInputData(reinterpret_cast<HRAWINPUT>(lparam),
							RID_INPUT,
							NULL,
							&size,
							sizeof(RAWINPUTHEADER)
						);
						assert(raw_result != static_cast<UINT>(-1));
						assert(size <= 48);

						raw_result = GetRawInputData(reinterpret_cast<HRAWINPUT>(lparam),
							RID_INPUT,
							data,
							&size,
							sizeof(RAWINPUTHEADER)
						);
						assert(raw_result == size);

						RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(data);
						if (raw->header.dwType == RIM_TYPEMOUSE)
						{
							int relx = raw->data.mouse.lLastX;
							int rely = raw->data.mouse.lLastY;
							//LOGV("%i, %i %i\n", raw->data.mouse.usFlags, relx, rely);

							last_mousex += relx;
							last_mousey += rely;

							kernel::MouseEvent mevent;
							mevent.mx = last_mousex;
							mevent.my = last_mousey;
							mevent.dx = relx;
							mevent.dy = rely;
							mevent.wheel_direction = 0;
							mevent.window = window;
							mevent.subtype = kernel::MouseDelta;

							kernel::event_dispatch(mevent);
							return 0;
						}

						break;
					}

					case WM_MOUSEMOVE:
					{
						// handle mouse events
						if (disable_next_mouse_event)
						{
							disable_next_mouse_event = 0;
							return 0;
						}
						kernel::MouseEvent mevent;

						// According to MSDN; these coordinates are window-local.
						mevent.mx = GET_X_LPARAM(lparam);
						mevent.my = GET_Y_LPARAM(lparam);
						mevent.dx = (mevent.mx - last_mousex);
						last_mousex = mevent.mx;
						mevent.dy = (mevent.my - last_mousey);
						last_mousey = mevent.my;
						mevent.wheel_direction = 0;
						mevent.window = window;
						mevent.subtype = _window_provider->in_relative_mode() ? kernel::MouseDelta : kernel::MouseMoved;
						kernel::event_dispatch(mevent);

						constrain_mouse_in_relative_mode(hwnd, prev_mouse[0], prev_mouse[1]);
						return 0;
						break;
					}


					case WM_MOUSEWHEEL:
					{
						// -1 is towards the user
						// 1 is away from the user
						kernel::MouseEvent mevent;
						mevent.mx = GET_X_LPARAM(lparam);
						mevent.my = GET_Y_LPARAM(lparam);
						mevent.dx = (mevent.mx - last_mousex);
						last_mousex = mevent.mx;
						mevent.dy = (mevent.my - last_mousey);
						last_mousey = mevent.my;
						mevent.wheel_direction = (static_cast<int16_t>(HIWORD(wparam)) > 0) ? 1 : -1;
						mevent.window = window;
						mevent.subtype = kernel::MouseWheelMoved;
						kernel::event_dispatch(mevent);

						constrain_mouse_in_relative_mode(hwnd, prev_mouse[0], prev_mouse[1]);
						return 0;
						break;
					}

					case WM_LBUTTONDBLCLK:
					case WM_LBUTTONDOWN: handle_mouse_button(true, LOWORD(lparam), HIWORD(lparam), window, gemini::MouseButton::MOUSE_LEFT); break;
					case WM_RBUTTONDBLCLK:
					case WM_RBUTTONDOWN: handle_mouse_button(true, LOWORD(lparam), HIWORD(lparam), window, gemini::MouseButton::MOUSE_RIGHT); break;
					case WM_MBUTTONDBLCLK:
					case WM_MBUTTONDOWN: handle_mouse_button(true, LOWORD(lparam), HIWORD(lparam), window, gemini::MouseButton::MOUSE_MIDDLE); break;
					case WM_XBUTTONDBLCLK:
					case WM_XBUTTONDOWN:
					{
						const WORD button_index = HIWORD(wparam);
						assert(button_index >= 0 && button_index <= 2);
						handle_mouse_button(true, LOWORD(lparam), HIWORD(lparam), window, xmouse[button_index]);
						return 0;
						break;
					}

					case WM_LBUTTONUP: handle_mouse_button(false, LOWORD(lparam), HIWORD(lparam), window, gemini::MouseButton::MOUSE_LEFT); break;
					case WM_RBUTTONUP: handle_mouse_button(false, LOWORD(lparam), HIWORD(lparam), window, gemini::MouseButton::MOUSE_RIGHT); break;
					case WM_MBUTTONUP: handle_mouse_button(false, LOWORD(lparam), HIWORD(lparam), window, gemini::MouseButton::MOUSE_MIDDLE); break;
					case WM_XBUTTONUP:
					{
						const WORD button_index = HIWORD(wparam);
						assert(button_index >= 0 && button_index <= 2);
						handle_mouse_button(false, LOWORD(lparam), HIWORD(lparam), window, xmouse[button_index]);
						return 0;
						break;
					}

					// handle keyboard events
					case WM_SYSKEYDOWN:
					{
						if (wparam == VK_F4)
						{
							// Handle Alt+F4
							return DefWindowProc(hwnd, message, wparam, lparam);
						}
					}

					case WM_CHAR:
					{
						kernel::KeyboardEvent event;
						event.key = 0;
						event.is_down = true;
						event.modifiers = 0;
						event.window = window;
						event.is_text = 1;
						event.unicode = static_cast<uint32_t>(wparam);
						kernel::event_dispatch(event);
						return 0;
					}

					case WM_SYSKEYUP:
					case WM_KEYDOWN:
					case WM_KEYUP:
					{
						return handle_keyboard_event(window, message, wparam, lparam);
						break;
					}

					// handle system events
					case WM_SETFOCUS:
					case WM_KILLFOCUS:
					{
						kernel::SystemEvent sysevent;
						sysevent.subtype = (message == WM_KILLFOCUS) ? kernel::WindowLostFocus : kernel::WindowGainFocus;
						sysevent.window = window;
						kernel::event_dispatch(sysevent);
						return 0;
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
							return 0;
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

						WPARAM resize_type = wparam;
						if (resize_type == SIZE_MAXIMIZED)
						{
							// The window has been maximized.
							kernel::event_dispatch(sysevent);
							return 0;
						}
						else if (resize_type == SIZE_MINIMIZED)
						{
							window->minimized(true);

							// The window has been minimized.
							sysevent.subtype = kernel::WindowMinimized;
							kernel::event_dispatch(sysevent);
							return 0;
						}
						else if (resize_type == SIZE_RESTORED)
						{
							if (window->minimized())
							{
								window->minimized(false);
								sysevent.subtype = kernel::WindowRestored;
								kernel::event_dispatch(sysevent);
							}
							// Resized window, but neither Maximized nor Minimized applies.
							sysevent.subtype = kernel::WindowResized;
							kernel::event_dispatch(sysevent);
							return 0;
						}

						break;
					}

					case WM_WINDOWPOSCHANGED:
					{
						// Handle when the user moves the Window.
						WINDOWPOS* new_position = reinterpret_cast<WINDOWPOS*>(lparam);

						// re-center the last mouse position if in relative
						// mode.
						if (_window_provider->in_relative_mode())
						{
							POINT center;
							center.x = static_cast<int32_t>((new_position->cx) / 2.0f);
							center.y = static_cast<int32_t>((new_position->cy) / 2.0f);

							// convert the point to screen space
							ClientToScreen(hwnd, &center);
							win32::last_mousex = center.x;
							win32::last_mousey = center.y;
							return 0;
						}
						break;
					}

					case WM_SYSCOMMAND:
					{
						//four low order bits of wparam are used
						// internally, so we have to mask off wparam.
						if ((wparam & 0xfff0) == SC_SCREENSAVE)
						{
							// Tell windows we want to disable the screen
							// saver by returning 0.
							return 0;
						}
						else if ((wparam & 0xfff0) == SC_MONITORPOWER)
						{
							// 1: display is going to low power
							// 2: display is being shut off
							if (lparam > 0)
							{
								// We handled this, don't disable monitor.
								return 0;
							}
						}
						break;
					}
					} // switch
				} // window

				return DefWindowProc(hwnd, message, wparam, lparam);
			} // WindowProc
		} // namespace win32

		Result Win32WindowProvider::startup()
		{
			is_in_relative_mode = false;


			// register for raw input
			// keyboard: usagepage = 1, usage = 6
#if 0

			RAWINPUTDEVICELIST* device_list = nullptr;
			UINT total_devices;
			// query total devices
			UINT result = GetRawInputDeviceList(NULL,
				&total_devices,
				sizeof(RAWINPUTDEVICELIST)
			);

			assert(result != static_cast<UINT>(-1));

			device_list = reinterpret_cast<RAWINPUTDEVICELIST*>(
				MEMORY2_ALLOC(get_platform_allocator2(),
					sizeof(RAWINPUTDEVICELIST) * total_devices)
			);

			// retrieve the full array of devices
			result = GetRawInputDeviceList(device_list,
				&total_devices,
				sizeof(RAWINPUTDEVICELIST)
			);

			assert(result == total_devices);


			// iterate over the list
			for (UINT index = 0; index < total_devices; ++index)
			{
				HANDLE device = device_list[index].hDevice;
				DWORD type = device_list[index].dwType;
				if (type == RIM_TYPEKEYBOARD)
				{
					LOGV("found keyboard\n");
				}
				else if (type == RIM_TYPEMOUSE)
				{
					LOGV("found mouse\n");
				}
				else
				{
					LOGV("found HID device\n");
				}


				// get device name
				char* buffer = nullptr;
				UINT buffer_size = 0;
				UINT info_result = GetRawInputDeviceInfoA(device,
					RIDI_DEVICENAME,
					NULL,
					&buffer_size
				);

				assert(buffer_size > 0);

				buffer = reinterpret_cast<char*>(
					MEMORY2_ALLOC(get_platform_allocator2(), buffer_size)
				);

				info_result = GetRawInputDeviceInfoA(device,
					RIDI_DEVICENAME,
					buffer,
					&buffer_size
				);

				assert(info_result > 0);
				LOGV("device_name: %s\n", buffer);

				MEMORY2_DEALLOC(get_platform_allocator2(), buffer);
			}

			MEMORY2_DEALLOC(get_platform_allocator2(), device_list);
#endif
			return Result::success();
		}

		void Win32WindowProvider::shutdown()
		{
		}

		NativeWindow* Win32WindowProvider::create(const Parameters& parameters, void* /*native_visual*/)
		{
			const wchar_t WINDOW_CLASS_NAME[] = L"platform_window_class";
			const size_t WINDOW_MAX_LENGTH = 128;

			win32::Window* window = MEMORY2_NEW(get_platform_allocator2(), win32::Window)(parameters);
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
				window_style |= WS_MINIMIZEBOX | WS_CAPTION | WS_BORDER | WS_SYSMENU;
				if (parameters.enable_resize)
				{
					window_style |= WS_OVERLAPPEDWINDOW;
				}
			}
			else
			{
				window_style |= WS_POPUP;
			}

			RECT new_rect = {
				0,
				0,
				static_cast<LONG>(parameters.frame.width),
				static_cast<LONG>(parameters.frame.height)
			};

			AdjustWindowRect(&new_rect, window_style, 0);
			window_width = static_cast<int>(new_rect.right - new_rect.left);
			window_height = static_cast<int>(new_rect.bottom - new_rect.top);

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
			BringWindowToTop(window_handle);
			SetForegroundWindow(window_handle);
			UpdateWindow(window_handle);
			SetFocus(window_handle);

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
			MEMORY2_DELETE(get_platform_allocator2(), native_window);
		}

		Frame Win32WindowProvider::get_frame(NativeWindow* window) const
		{
			Frame window_frame;
			HWND hwnd = static_cast<HWND>(window->get_native_handle());

			RECT window_rect;
			GetWindowRect(hwnd, &window_rect);

			RECT client_rect;
			GetClientRect(hwnd, &client_rect);

			// We have to convert rect from client to screen as get_frame must
			// return screen coordinates.
			ClientToScreen(hwnd, reinterpret_cast<LPPOINT>(&client_rect.left));
			ClientToScreen(hwnd, reinterpret_cast<LPPOINT>(&client_rect.right));

			window_frame.x = static_cast<float>(window_rect.left);
			window_frame.y = static_cast<float>(window_rect.top);
			window_frame.width = static_cast<float>(client_rect.right - client_rect.left);
			window_frame.height = static_cast<float>(client_rect.bottom - client_rect.top);
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
			// If you hit this assert, startup was called more than once.
			assert(_window_provider == nullptr);

			// create the win32 window provider instance
			_window_provider = MEMORY2_NEW(get_platform_allocator2(), Win32WindowProvider);

			// out of memory?
			assert(_window_provider != nullptr);


			Result window_startup = _window_provider->startup();
			if (window_startup.failed())
			{
				LOGE("window_provider startup failed with error: %s\n", window_startup.message);
				return window_startup;
			}

			// try to create the graphics provider
			_graphics_provider = MEMORY2_NEW(get_platform_allocator2(), Win32GraphicsProvider);

			// out of memory?
			assert(_graphics_provider);

			Result graphics_startup = _graphics_provider->startup(_window_provider);
			if (graphics_startup.failed())
			{
				LOGE("graphics_provider startup failed with error: %s\n", graphics_startup.message);
				return graphics_startup;
			}

			// store the default clip rect
			GetClipCursor(&default_clip_rect);

			return Result::success();
		}

		void shutdown()
		{
			_graphics_provider->shutdown(_window_provider);
			MEMORY2_DELETE(get_platform_allocator2(), _graphics_provider);

			_window_provider->shutdown();
			MEMORY2_DELETE(get_platform_allocator2(), _window_provider);
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
				graphics_data = MEMORY2_ALLOC(get_platform_allocator2(), graphics_data_size);
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

			MEMORY2_DEALLOC(get_platform_allocator2(), window->graphics_data);
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
			// SetCursorPos will cause a mouse move event. We don't want to
			// dispatch it, so ignore it.
			disable_next_mouse_event = 1;

			// Horizontal border is just the size of the frame.
			SetCursorPos(static_cast<int>(x), static_cast<int>(y));
		}

		void get_cursor(float& x, float& y)
		{
			POINT point;
			GetCursorPos(&point);
			x = static_cast<float>(point.x);
			y = static_cast<float>(point.y);
		}

		void set_relative_mouse_mode(NativeWindow* window, bool enable)
		{
			_window_provider->set_relative_mode(enable);

			RAWINPUTDEVICE rid[1];

			// register mice for raw input
			rid[0].usUsagePage = 1;
			rid[0].usUsage = 2;
			rid[0].dwFlags = RIDEV_NOLEGACY /*| RIDEV_INPUTSINK*/;
			rid[0].hwndTarget = 0;

			if (enable)
			{
				//if (RegisterRawInputDevices(rid, 1, sizeof(RAWINPUTDEVICE)) == FALSE)
				//{
				//	LOGE("Failed to register RawInput devices. %i\n", GetLastError());
				//}

				assert(window);

				HWND hwnd = reinterpret_cast<HWND>(window->get_native_handle());

				RECT window_rect;
				GetWindowRect(hwnd, &window_rect);

				// this should center the cursor inside window
				RECT client_rect;
				GetClientRect(hwnd, &client_rect);

				ClientToScreen(hwnd, reinterpret_cast<LPPOINT>(&client_rect.left));
				ClientToScreen(hwnd, reinterpret_cast<LPPOINT>(&client_rect.right));

				const float centerx = (client_rect.right - client_rect.left) / 2.0f;
				const float centery = (client_rect.bottom - client_rect.top) / 2.0f;

				// set new clip rect
				ClipCursor(&client_rect);
				win32::last_mousex = static_cast<int32_t>((client_rect.right - client_rect.left) / 2.0f);
				win32::last_mousey = static_cast<int32_t>((client_rect.bottom - client_rect.top) / 2.0f);

				set_cursor(static_cast<float>(window_rect.left + centerx),
					static_cast<float>(window_rect.top + centery)
				);
			}
			else
			{
				// restore the default clip rect
				ClipCursor(&default_clip_rect);

				//// unregister raw input for mouse
				//rid[0].dwFlags = RIDEV_REMOVE;
				//RegisterRawInputDevices(rid, 1, sizeof(RAWINPUTDEVICE));
			}
		}

		void set_mouse_tracking(bool)
		{
		}
	} // namespace window
} // namespace platform
