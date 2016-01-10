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
#include "platform_internal.h"
#include "linux_backend.h"
#include "x11_window_provider.h"

#include <platform/input.h>
#include <platform/kernel.h>
#include <platform/kernel_events.h>

#include <X11/extensions/Xrandr.h>

// requires: libxinerama-dev (ubuntu)
// see if we can replace this with XRandR
#include <X11/extensions/Xinerama.h>

namespace platform
{
	namespace window
	{
		const long PLATFORM_X11_EVENT_MASK = \
			// request info when contents of lost window regions
			ExposureMask |

			// request change in input focus
			FocusChangeMask |

			// request key down and up events
			KeyPressMask | KeyReleaseMask | KeymapStateMask |

			// request pointer motion events
			PointerMotionMask |

			// request pointer button down and up events
			ButtonPressMask | ButtonReleaseMask |

			// request pointer motion while any button down
			ButtonMotionMask |

			// resize
			ResizeRedirectMask |

			// request changes to window structure
			StructureNotifyMask |

			// request change when pointer enters/leaves window
			EnterWindowMask | LeaveWindowMask;

		X11WindowProvider::X11WindowProvider() :
			display(0),
			last_x(0),
			last_y(0)
		{
		}

		X11WindowProvider::~X11WindowProvider()
		{
		}

		Result X11WindowProvider::startup()
		{
			// try to open the display
			display = XOpenDisplay(0);

			if (!display)
			{
				return Result::failure("Unable to open display 0");
			}

			// DeleteWindow must be treated as a ClientMessage. We cache
			// this for when windows are created.
			atom_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", False);

			return Result::success();
		}

		void X11WindowProvider::shutdown()
		{
			if (display)
			{
				XCloseDisplay(display);
			}
		}

		NativeWindow* X11WindowProvider::create(const Parameters& parameters, void* native_visual)
		{
			X11Window* window = MEMORY_NEW(X11Window, get_platform_allocator());
			Visual* visual = static_cast<Visual*>(native_visual);
			assert(visual != nullptr);

			XSetWindowAttributes window_attributes;
			window_attributes.border_pixel = 0;
			window_attributes.background_pixel = 0;
			window_attributes.event_mask = PLATFORM_X11_EVENT_MASK;
			window_attributes.colormap = XCreateColormap(
				display,
				RootWindow(display, DefaultScreen(display)),
				visual,
				AllocNone);

			window->native_window = XCreateWindow(
				display,
				RootWindow(display, DefaultScreen(display)),
				parameters.frame.x,
				parameters.frame.y,
				parameters.frame.width,
				parameters.frame.height,
				0,
				CopyFromParent,
				InputOutput,
				visual,
				CWColormap | CWEventMask,
				&window_attributes);

			// replaces WM_PROTOCOLS property on the window with the atom specified.
			XSetWMProtocols(display, window->native_window, &atom_delete_window, 1);

			// set the event masks
			XSelectInput(display, window->native_window, PLATFORM_X11_EVENT_MASK);

			// set the window title
			XStoreName(display, window->native_window, parameters.window_title);

			XClearWindow(display, window->native_window);
			XMapRaised(display, window->native_window);

			// manually flush the output buffer
			// happens as part of: XPending, XNextEvent, and XWindowEvent
			XFlush(display);

			return window;
		}

		void X11WindowProvider::destroy(NativeWindow* window)
		{
			void* native_handle = window->get_native_handle();
			Window* window_handle = static_cast<Window*>(native_handle);

			XDestroyWindow(display, *window_handle);
			MEMORY_DELETE(window, get_platform_allocator());
		}

		Frame X11WindowProvider::get_frame(NativeWindow* window) const
		{
			Frame frame;

			Window root_window;
			int x;
			int y;
			unsigned int width;
			unsigned int height;
			unsigned int border_width;
			unsigned int depth;

			void* native_handle = window->get_native_handle();
			Window* window_handle = static_cast<Window*>(native_handle);

			XGetGeometry(
				display,
				*window_handle,
				&root_window,
				&x, &y,
				&width, &height,
				&border_width, &depth);

			frame.x = static_cast<float>(x);
			frame.y = static_cast<float>(y);
			frame.width = static_cast<float>(width);
			frame.height = static_cast<float>(height);

			return frame;
		}


		Frame X11WindowProvider::get_render_frame(NativeWindow* window) const
		{
			return get_frame(window);
		}

		size_t X11WindowProvider::get_screen_count() const
		{
			int dummy[2];
			if (XineramaQueryExtension(display, &dummy[0], &dummy[1]))
			{
				if (XineramaIsActive(display))
				{
					int total_screens = 0;
					XineramaScreenInfo* screens = XineramaQueryScreens(display, &total_screens);
					XFree(screens);
					return total_screens;
				}
			}
			return ScreenCount(display);
		}

		Frame X11WindowProvider::get_screen_frame(size_t screen_index) const
		{
			Frame frame;
			int dummy[2];
			if (XineramaQueryExtension(display, &dummy[0], &dummy[1]))
			{
				if (XineramaIsActive(display))
				{
					int total_screens = 0;
					XineramaScreenInfo* screens = XineramaQueryScreens(display, &total_screens);
					if (total_screens > 0)
					{
						XineramaScreenInfo& screen = screens[screen_index];
						frame.x = screen.x_org;
						frame.y = screen.y_org;
						frame.width = screen.width;
						frame.height = screen.height;
					}

					XFree(screens);
				}
			}

			return frame;
		}

		void X11WindowProvider::dispatch_events()
		{
			if (!display)
			{
				// No connection to the display.
				return;
			}

			XEvent event;
			event.type = 0;
			last_key_release.type = 0;
			last_key_release.xkey.keycode = 0;
			last_key_release.xkey.time = 0;

			// process available events
			while (XPending(display))
			{
				XNextEvent(display, &event);
				process_event(event);
			}
		}

		Display* X11WindowProvider::get_display() const
		{
			return display;
		}

		void X11WindowProvider::process_event(XEvent& event)
		{
			char buffer[32] = {0};
			int32_t length = 0;
			KeySym keysym = 0;

			kernel::SystemEvent sysevent;
			kernel::MouseEvent mouseevent;

			uint32_t x11_mouse_to_platform[] = {
				input::MOUSE_INVALID, 	// AnyButton
				input::MOUSE_LEFT,		// Button1
				input::MOUSE_MIDDLE,	// Button2 // (Yes Middle is Button2)
				input::MOUSE_RIGHT,		// Button3

				// scroll wheel
				input::MOUSE_INVALID,	// Button4
				input::MOUSE_INVALID,	// Button5

				input::MOUSE_INVALID,	// Button6
				input::MOUSE_INVALID,	// Button7

				input::MOUSE_MOUSE4,	// Button8
				input::MOUSE_MOUSE5,	// Button9
			};

			switch(event.type)
			{
				// input focus has changed:
				// If you don't have focus and you click on the GL
				// portion of the window, FocusIn will not be fired.
				// Instead, if you click on the Window title bar,
				// it will be.
				case FocusIn:
				case FocusOut:
					sysevent.subtype = (event.type == FocusIn) ? kernel::WindowGainFocus : kernel::WindowLostFocus;
					kernel::event_dispatch(sysevent);
					break;

				// window has resized
				case ResizeRequest:
					sysevent.subtype = kernel::WindowResized;
					sysevent.render_width = sysevent.window_width = event.xresizerequest.width;
					sysevent.render_height = sysevent.window_height = event.xresizerequest.height;
					kernel::event_dispatch(sysevent);
					break;

				// keyboard state has changed
				case KeymapNotify:
					break;

				case KeyPress:
				case KeyRelease:
					length = XLookupString(&event.xkey, buffer, 32, &keysym, NULL);
					fprintf(stdout, "key: %s\n", buffer);
					break;

				case ButtonPress:
				case ButtonRelease:
					mouseevent.subtype = kernel::MouseButton;
					mouseevent.is_down = (event.type == ButtonPress) ? true : false;
					mouseevent.button = x11_mouse_to_platform[event.xbutton.button];

					// handle mouse wheel
					if (event.xbutton.button == Button4 || event.xbutton.button == Button5)
					{
						mouseevent.wheel_direction = (event.xbutton.button == Button4) ? 1 : -1;
						mouseevent.subtype = kernel::MouseWheelMoved;
					}
					kernel::event_dispatch(mouseevent);
					break;

				// Pointer motion begins and ends within a single window
				case MotionNotify:
					mouseevent.subtype = kernel::MouseMoved;
					mouseevent.mx = event.xmotion.x;
					mouseevent.my = event.xmotion.y;
					mouseevent.dx = (event.xmotion.x - last_x);
					mouseevent.dy = (event.xmotion.y - last_y);

					// cache these values
					last_x = event.xmotion.x;
					last_y = event.xmotion.y;
					kernel::event_dispatch(mouseevent);
					break;

				// Pointer motion results in a change of windows
				case EnterNotify:
				case LeaveNotify:
					break;

				// The client application destroys a window
				// by calling XDestroyWindow or XDestroySubwindows.
				case DestroyNotify:
					// We have this redirected to our own atom
					break;

				case ClientMessage:
					if (static_cast<Atom>(event.xclient.data.l[0]) == atom_delete_window)
					{
						sysevent.subtype = kernel::WindowClosed;
						kernel::event_dispatch(sysevent);
					}
					break;

				case Expose:
				case MapNotify:
				case UnmapNotify:
				case ReparentNotify:
				case ConfigureNotify:
					break;

				default:
					PLATFORM_LOG(platform::LogMessageType::Error, "Unhandled X11 event type: %i\n", event.type);
					assert(0);
					break;
			}
		}
	} // namespace window
} // namespace platform
