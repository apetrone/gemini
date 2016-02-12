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

#import "cocoa_window_provider.h"
#import "cocoa_openglview.h"
#import "cocoa_window.h"

#include "kernel.h"
#include "input.h"

#include <core/logging.h>

namespace platform
{
	namespace window
	{
		namespace cocoa
		{
			const unsigned int kMaxKeys = 132;
			static input::Button key_map[ kMaxKeys ];

			struct CocoaState
			{
				// used to generate mouse deltas
				NSPoint last_mouse;

				// If we're generating relative mouse events
				bool is_in_relative_mouse_mode;

				// This is a nasty hack due to the way disassociation works
				// and the resulting NSEvent deltas work.
				// When we disassociate the mouse cursor, the very next
				// NSEvent has a rather large deltaX/Y -- which we need to ignore
				// to prevent the cursor from jumping..
				// Remove this hack if we determine how to solve that.
				bool ignore_next_event;

				// We need to keep track of the mouse visibliity
				// because the calls to hide/show cursor MUST match on
				// OS X!
				bool is_cursor_visible = true;

				// mouse tracking enables events outside the window to be sent when enabled
				bool is_tracking_mouse = false;
			};

			static CocoaState _state;




			// We keep this so we can share GL contexts with all subsequent
			// windows created.
			static NSOpenGLContext* _share_context = nil;

			void populate_keymap()
			{
				memset(&key_map, 0, kMaxKeys * sizeof(input::Button));

				// values taken from <HIToolbox/Events.h>
				key_map[0x00] = input::BUTTON_A;
				key_map[0x01] = input::BUTTON_S;
				key_map[0x02] = input::BUTTON_D;
				key_map[0x03] = input::BUTTON_F;
				key_map[0x04] = input::BUTTON_H;
				key_map[0x05] = input::BUTTON_G;
				key_map[0x06] = input::BUTTON_Z;
				key_map[0x07] = input::BUTTON_X;
				key_map[0x08] = input::BUTTON_C;
				key_map[0x09] = input::BUTTON_V;
				key_map[0x0B] = input::BUTTON_B;
				key_map[0x0C] = input::BUTTON_Q;
				key_map[0x0D] = input::BUTTON_W;
				key_map[0x0E] = input::BUTTON_E;
				key_map[0x0F] = input::BUTTON_R;
				key_map[0x10] = input::BUTTON_Y;
				key_map[0x11] = input::BUTTON_T;
				key_map[0x12] = input::BUTTON_1;
				key_map[0x13] = input::BUTTON_2;
				key_map[0x14] = input::BUTTON_3;
				key_map[0x15] = input::BUTTON_4;
				key_map[0x16] = input::BUTTON_6;
				key_map[0x17] = input::BUTTON_5;
				key_map[0x18] = input::BUTTON_EQUALS;
				key_map[0x19] = input::BUTTON_9;
				key_map[0x1A] = input::BUTTON_7;
				key_map[0x1B] = input::BUTTON_MINUS;
				key_map[0x1C] = input::BUTTON_8;
				key_map[0x1D] = input::BUTTON_0;
				key_map[0x1E] = input::BUTTON_RBRACKET;
				key_map[0x1F] = input::BUTTON_O;
				key_map[0x20] = input::BUTTON_U;
				key_map[0x21] = input::BUTTON_LBRACKET;
				key_map[0x22] = input::BUTTON_I;
				key_map[0x23] = input::BUTTON_P;
				key_map[0x24] = input::BUTTON_RETURN;
				key_map[0x25] = input::BUTTON_L;
				key_map[0x26] = input::BUTTON_J;
				key_map[0x27] = input::BUTTON_QUOTE;
				key_map[0x28] = input::BUTTON_K;
				key_map[0x29] = input::BUTTON_SEMICOLON;
				key_map[0x2A] = input::BUTTON_BACKSLASH;
				key_map[0x2B] = input::BUTTON_COMMA;
				key_map[0x2C] = input::BUTTON_SLASH;
				key_map[0x2D] = input::BUTTON_N;
				key_map[0x2E] = input::BUTTON_M;
				key_map[0x2F] = input::BUTTON_PERIOD;
				key_map[0x30] = input::BUTTON_TAB;
				key_map[0x31] = input::BUTTON_SPACE;
				key_map[0x32] = input::BUTTON_TILDE; // (GRAVE -> TILDE)
				key_map[0x33] = input::BUTTON_DELETE;
				key_map[0x35] = input::BUTTON_ESCAPE;
				key_map[0x36] = input::BUTTON_ROSKEY;
				key_map[0x37] = input::BUTTON_LOSKEY;
				key_map[0x38] = input::BUTTON_LSHIFT;
				key_map[0x39] = input::BUTTON_CAPSLOCK;
				key_map[0x3A] = input::BUTTON_LALT; // (LOPTION -> LALT)
				key_map[0x3B] = input::BUTTON_LCONTROL;
				key_map[0x3C] = input::BUTTON_RSHIFT;
				key_map[0x3D] = input::BUTTON_RALT; // (ROPTION -> RALT)
				key_map[0x3E] = input::BUTTON_RCONTROL;
				key_map[0x3F] = input::BUTTON_FUNCTION;
				key_map[0x40] = input::BUTTON_F17;
				key_map[0x41] = input::BUTTON_PERIOD;
				key_map[0x43] = input::BUTTON_NUMPAD_MULTIPLY;
				key_map[0x45] = input::BUTTON_NUMPAD_PLUS;
				key_map[0x47] = input::BUTTON_NUMLOCK; // (CLEAR -> NUMLOCK)
		//		key_map[0x48] = input::BUTTON_VOLUME_UP;
		//		key_map[0x49] = input::BUTTON_VOLUME_DOWN;
		//		key_map[0x4A] = input::BUTTON_MUTE;
				key_map[0x4B] = input::BUTTON_NUMPAD_DIVIDE;
				key_map[0x4C] = input::BUTTON_NUMPAD_ENTER;
				key_map[0x4E] = input::BUTTON_NUMPAD_MINUS;
				key_map[0x4F] = input::BUTTON_F18;
				key_map[0x50] = input::BUTTON_F19;
				key_map[0x51] = input::BUTTON_NUMPAD_EQUALS;
				key_map[0x52] = input::BUTTON_NUMPAD0;
				key_map[0x53] = input::BUTTON_NUMPAD1;
				key_map[0x54] = input::BUTTON_NUMPAD2;
				key_map[0x55] = input::BUTTON_NUMPAD3;
				key_map[0x56] = input::BUTTON_NUMPAD4;
				key_map[0x57] = input::BUTTON_NUMPAD5;
				key_map[0x58] = input::BUTTON_NUMPAD6;
				key_map[0x59] = input::BUTTON_NUMPAD7;
				key_map[0x5A] = input::BUTTON_F20;
				key_map[0x5B] = input::BUTTON_NUMPAD8;
				key_map[0x5C] = input::BUTTON_NUMPAD9;
				key_map[0x60] = input::BUTTON_F5;
				key_map[0x61] = input::BUTTON_F6;
				key_map[0x62] = input::BUTTON_F7;
				key_map[0x63] = input::BUTTON_F3;
				key_map[0x64] = input::BUTTON_F8;
				key_map[0x65] = input::BUTTON_F9;
				key_map[0x67] = input::BUTTON_F11;
				key_map[0x69] = input::BUTTON_F13;
				key_map[0x6A] = input::BUTTON_F16;
				key_map[0x6B] = input::BUTTON_F14;
				key_map[0x6D] = input::BUTTON_F10;
				key_map[0x6E] = input::BUTTON_MENU;
				key_map[0x6F] = input::BUTTON_F12;
				key_map[0x71] = input::BUTTON_F15;
				key_map[0x72] = input::BUTTON_INSERT;
				key_map[0x73] = input::BUTTON_HOME;
				key_map[0x74] = input::BUTTON_PAGEUP;
				key_map[0x75] = input::BUTTON_DELETE;
				key_map[0x76] = input::BUTTON_F4;
				key_map[0x77] = input::BUTTON_END;
				key_map[0x78] = input::BUTTON_F2;
				key_map[0x79] = input::BUTTON_PAGEDN;
				key_map[0x7A] = input::BUTTON_F1;
				key_map[0x7B] = input::BUTTON_LEFT;
				key_map[0x7C] = input::BUTTON_RIGHT;
				key_map[0x7D] = input::BUTTON_DOWN;
				key_map[0x7E] = input::BUTTON_UP;
			}

			input::Button convert_keycode(unsigned short keycode)
			{
				input::Button button = key_map[keycode];

				if (button == 0)
				{
					LOGV("UNKNOWN KEYCODE: %i\n", keycode);
					return input::Button::BUTTON_INVALID;
				}

				return button;
			}

			// We need to store a copy of the keymod state due to the way MacOS X
			// sets its flags.
			static uint16_t _keymod_state = 0;
			uint16_t keymod_state()
			{
				return _keymod_state;
			}

			void keymod_state(uint16_t keymods)
			{
				_keymod_state = keymods;
			}

			struct cocoa_native_window : public NativeWindow
			{
				cocoa_native_window(const Parameters& /*parameters*/) :
					NativeWindow(),
					cw(nil),
					context(nil)
				{
				}

				virtual void* get_native_handle() override
				{
					return (__bridge void*)cw;
				}

				CocoaWindow* cw;
				NSOpenGLContext* context;
			};

			Result create_window(cocoa_native_window* native_window, const Parameters& params)
			{
				CocoaWindow* window;

				// create a frame for the new window
				NSRect frame = NSMakeRect(params.frame.x, params.frame.y, params.frame.width, params.frame.height);

				// determine the window mask based on parameters
				NSUInteger window_mask = 0;
				if (params.enable_fullscreen)
				{
					window_mask = NSBorderlessWindowMask;
				}
				else
				{
					window_mask = NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask;
					if (params.enable_resize)
					{
						window_mask |= NSResizableWindowMask;
					}
				}



				// create a new window
				window = [[CocoaWindow alloc] initWithContentRect:frame styleMask:window_mask backing:NSBackingStoreBuffered defer:NO];
				if (!window)
				{
					return platform::Result::failure("create window failed");
				}

				// link the window to our handle
				window.instance = native_window;
				native_window->cw = window;

				[window setBackgroundColor:[NSColor whiteColor]];
				[window makeKeyAndOrderFront: nil];
				[window setTitle: [NSString stringWithUTF8String: params.window_title]];
				[window setAcceptsMouseMovedEvents: YES];
				[window setReleasedWhenClosed: YES];

				id delegate = [NSApp delegate];
				[window setDelegate: delegate];

				NSPoint origin = NSMakePoint(params.frame.x, params.frame.y);
				[window setFrameOrigin: origin];

				// http://fredandrandall.com/blog/2011/09/08/how-to-make-your-app-open-in-full-screen-on-lion/
				//	[window setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
				//	[window toggleFullScreen:nil];
				if (params.enable_fullscreen)
				{
					[window setLevel: CGShieldingWindowLevel()];
				}

				return platform::Result::success();
			}

			void create_context(cocoa_native_window* window, const Parameters& window_parameters)
			{
				// ---------------------------------------------------------------------
				// setup the opengl context
				// ---------------------------------------------------------------------
				NSOpenGLPixelFormatAttribute attributes[18];

				// https://developer.apple.com/library/prerelease/mac/documentation/Cocoa/Reference/ApplicationKit/Classes/NSOpenGLPixelFormat_Class/index.html#//apple_ref/c/tdef/NSOpenGLPixelFormatAttribute

				size_t index = 0;

				attributes[index++] = NSOpenGLPFADepthSize;
				attributes[index++] = window_parameters.backbuffer.depth_size;

				size_t color_size = \
					window_parameters.backbuffer.red_size +
					window_parameters.backbuffer.green_size +
					window_parameters.backbuffer.blue_size;

				attributes[index++] = NSOpenGLPFAColorSize;
				attributes[index++] = color_size;

				attributes[index++] = NSOpenGLPFAOpenGLProfile;
				attributes[index++] = NSOpenGLProfileVersion3_2Core;

				attributes[index++] = NSOpenGLPFAStencilSize;
				attributes[index++] = window_parameters.backbuffer.stencil_size;

	//			attributes[index++] = NSOpenGLPFAMultisample;
	//			attributes[index++] = NSOpenGLPFASampleBuffers;
	//			attributes[index++] = (NSOpenGLPixelFormatAttribute)1;
	//
	//			attributes[index++] = NSOpenGLPFASamples;
	//			attributes[index++] = (NSOpenGLPixelFormatAttribute)16;

				attributes[index++] = NSOpenGLPFAAccelerated;
				attributes[index++] = NSOpenGLPFADoubleBuffer;

				// terminate
				attributes[index++] = 0;

				// TODO: add error handling
				NSOpenGLPixelFormat* format = [[NSOpenGLPixelFormat alloc] initWithAttributes: attributes];
				assert(format != nil);


				window->context = [[NSOpenGLContext alloc] initWithFormat: format shareContext: _share_context];

				if (_share_context == nil)
				{
					_share_context = window->context;
				}

				GLint opacity = 1; // 1: opaque; 0: transparent
				GLint wait_for_vsync = window_parameters.enable_vsync; // 1: on, 0: off

				[window->context setValues:&opacity forParameter:NSOpenGLCPSurfaceOpacity];
				[window->context setValues:&wait_for_vsync forParameter:NSOpenGLCPSwapInterval];

				// ---------------------------------------------------------------------
				// setup NSOpenGLView
				// ---------------------------------------------------------------------

				// create our custom view
				CocoaOpenGLView* view = [[CocoaOpenGLView alloc] initWithFrame: [[window->cw contentView] frame]];
				[view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];

				// set context of the view
				[view setContext: window->context];

				// use the highest possible resolution on 'Retina' screens
				[view setWantsBestResolutionOpenGLSurface: YES];

				// receive windowResized events
				[[NSNotificationCenter defaultCenter] addObserver: view selector:@selector(windowResized:) name:NSWindowDidResizeNotification object:window->cw];

				// make the view this window's first responder and
				// set it as the content view
				[window->cw setContentView: view];
				[window->cw makeFirstResponder: view];
			}


			void attach_cocoa_context(cocoa_native_window* window)
			{
				[window->context makeCurrentContext];
			}

			void detach_cocoa_context(cocoa_native_window* /*window*/)
			{
				[NSOpenGLContext clearCurrentContext];
			}

			void swap_buffers(cocoa_native_window* window)
			{
				[window->context flushBuffer];
			}


			void center_cursor_inside_key_window()
			{
				NSWindow* key_window = [[NSApplication sharedApplication] keyWindow];
				if (key_window)
				{
					NSRect bounds = [[key_window contentView] frame];
					bounds = [key_window convertRectToScreen: bounds];

					// y needs to be flipped
					NSScreen* screen = [key_window screen];
					NSRect screen_frame = [screen frame];

					NSPoint window_center = NSMakePoint(
						bounds.origin.x + (bounds.size.width / 2.0),
						screen_frame.size.height - (bounds.origin.y + (bounds.size.height / 2.0))
								);
					CGWarpMouseCursorPosition(window_center);
					cocoa::_state.last_mouse = window_center;
					cocoa::_state.ignore_next_event = true;
					CGAssociateMouseAndMouseCursorPosition(true);
					CGAssociateMouseAndMouseCursorPosition(false);

					[key_window makeKeyAndOrderFront:nil];
					[key_window makeMainWindow];
				}
			}


			// Convert an NS mouse-Y location (based in the lower left)
			// to one based in the upper left.
			// This has to take the content rect of the window into account.
			CGFloat convert_nsmousey_to_platformy(NSWindow* window, CGFloat location_y)
			{
				NSRect frame = [window frame];
				NSRect content_rect = [NSWindow contentRectForFrameRect:frame styleMask:[window styleMask]];

				CGFloat title_bar_height = (frame.size.height - content_rect.size.height);

				// We subtract height from mouse location y to invert the y-axis; since
				// NSPoint's origin is in the lower left corner.
				// The fixed height is the window frame minus the title bar height
				// and we also subtract 1.0 because convertPoint starts from a base of 1
				// according to the Cocoa docs.
				CGFloat fixed_height = frame.size.height - title_bar_height - 1.0;

				return (fixed_height - location_y);
			}

			void dispatch_mouse_event(NSEvent* event)
			{
				// ignore events outside of the client area when not in relative mouse mode
				if (!_state.is_tracking_mouse && !_state.is_in_relative_mouse_mode && !NSPointInRect([event locationInWindow], [[[event window] contentView] frame]))
				{
					return;
				}

				kernel::MouseEvent ev;
				ev.is_down = false;
				ev.subtype = kernel::MouseButton;

				switch([event type])
				{
					case NSLeftMouseDragged:
					case NSRightMouseDragged:
					case NSOtherMouseDragged:
					case NSMouseMoved:
					{
						ev.subtype = kernel::MouseMoved;
						ev.mx = 0;
						ev.my = 0;
						if (cocoa::_state.is_in_relative_mouse_mode)
						{
							ev.subtype = kernel::MouseDelta;
							if (cocoa::_state.ignore_next_event)
							{
								cocoa::_state.ignore_next_event = false;
								return;
							}

							NSPoint mouse;
							mouse.x = [event deltaX];
							mouse.y = [event deltaY];
							ev.dx = static_cast<int>(mouse.x);
							ev.dy = static_cast<int>(mouse.y);
							cocoa::_state.last_mouse = [NSEvent mouseLocation];
							break;
						}
						else
						{
							NSPoint location = [event locationInWindow];
							NSPoint mouse_location = location;
							ev.mx = location.x;
							ev.my = convert_nsmousey_to_platformy([event window], location.y);
							ev.dx = (location.x - cocoa::_state.last_mouse.x);
							ev.dy = (mouse_location.y - cocoa::_state.last_mouse.y);
							cocoa::_state.last_mouse = mouse_location;
						}
					}
					case NSLeftMouseDown:
						ev.button = input::MOUSE_LEFT;
						ev.is_down = true;
						break;
					case NSRightMouseDown:
						ev.button = input::MOUSE_RIGHT;
						ev.is_down = true;
						break;
					case NSOtherMouseDown:
						ev.button = input::MOUSE_MIDDLE;
						ev.is_down = true;
						break;

					case NSLeftMouseUp:
						ev.button = input::MOUSE_LEFT;
						break;
					case NSRightMouseUp:
						ev.button = input::MOUSE_RIGHT;
						break;
					case NSOtherMouseUp:
						ev.button = input::MOUSE_MIDDLE;
						break;

					case NSScrollWheel:
						ev.subtype = kernel::MouseWheelMoved;
						ev.wheel_direction = ([event deltaY] > 0) ? 1 : -1;
						break;
				}

				kernel::event_dispatch(ev);
			}

			void dispatch_key_event(NSEvent* event)
			{
				// ignore repeating characters
				if (![event isARepeat])
				{
					kernel::KeyboardEvent ev;
					ev.is_down = ([event type] == NSKeyDown) ? true : false;
					ev.key = convert_keycode([event keyCode]);
					ev.modifiers = keymod_state();
					kernel::event_dispatch(ev);
				}
			}

			uint16_t convert_cocoa_keymods(NSUInteger modifier_flags)
			{
				uint16_t keymods = 0;

				if (modifier_flags & NX_DEVICELCTLKEYMASK)
				{
					keymods |= input::MOD_LEFT_CONTROL;
				}
				if (modifier_flags & NX_DEVICERCTLKEYMASK)
				{
					keymods |= input::MOD_RIGHT_CONTROL;
				}

				if (modifier_flags & NX_DEVICELSHIFTKEYMASK)
				{
					keymods |= input::MOD_LEFT_SHIFT;
				}
				if (modifier_flags & NX_DEVICERSHIFTKEYMASK)
				{
					keymods |= input::MOD_RIGHT_SHIFT;
				}

				if (modifier_flags & NX_DEVICELALTKEYMASK)
				{
					keymods |= input::MOD_LEFT_ALT;
				}
				if (modifier_flags & NX_DEVICERALTKEYMASK)
				{
					keymods |= input::MOD_RIGHT_ALT;
				}

				return keymods;
			}

			void dispatch_mouse_moved_event(NSEvent* the_event)
			{
				CocoaWindow* window = static_cast<CocoaWindow*>([the_event window]);

				// convert mouse from window to view
				NSPoint mouse_location = [[window contentView] convertPoint: [the_event locationInWindow] fromView:nil];
				if (mouse_location.x == cocoa::_state.last_mouse.x &&
					mouse_location.y == cocoa::_state.last_mouse.y)
				{
					return;
				}

				cocoa::_state.last_mouse = mouse_location;

				kernel::MouseEvent event;
				event.subtype = kernel::MouseMoved;
				event.mx = mouse_location.x;
				event.my = convert_nsmousey_to_platformy(window, mouse_location.y);



//				// don't dispatch any events outside of the window
//				if (event.mx >= 0 && event.my >= 0 && (event.mx <= frame.size.width) && (event.my <= fixed_height))
//				{
//					kernel::event_dispatch(event);
//				}
			}

			cocoa_native_window* from(NativeWindow* window)
			{
				return static_cast<cocoa_native_window*>(window);
			}
		} // namespace cocoa


		Result startup(RenderingBackend backend)
		{
			if (backend == RenderingBackend_Default)
			{
				backend = RenderingBackend_OpenGL;
			}

			if (backend != RenderingBackend_OpenGL)
			{
				return Result::failure("The requested rendering backend is not supported");
			}

			// populate the osx keymap
			cocoa::populate_keymap();

			return Result::success();
		}

		void shutdown()
		{

		}

		void dispatch_events()
		{
			while(true)
			{
				NSEvent* event = [NSApp
								  nextEventMatchingMask:NSAnyEventMask
								  untilDate:[NSDate distantPast]
								  inMode:NSEventTrackingRunLoopMode
								  dequeue:YES
								  ];

				if (event == nil)
				{
					break;
				}

				switch([event type])
				{
					case NSMouseMoved:
					case NSLeftMouseDown:
					case NSLeftMouseUp:
					case NSLeftMouseDragged:
					case NSRightMouseDown:
					case NSRightMouseUp:
					case NSRightMouseDragged:
					case NSOtherMouseDown:
					case NSOtherMouseUp:
					case NSOtherMouseDragged:
					case NSScrollWheel:
						cocoa::dispatch_mouse_event(event);
						break;

					case NSKeyDown:
					case NSKeyUp:
						cocoa::dispatch_key_event(event);
						break;
					default:
						break;
				}

				[NSApp sendEvent: event];
			}
		}


		NativeWindow* create(const Parameters& window_parameters)
		{
			cocoa::cocoa_native_window* window = MEMORY_NEW(cocoa::cocoa_native_window, get_platform_allocator())(window_parameters);

			platform::Result result = create_window(window, window_parameters);
			if (result.failed())
			{
				LOGE("create_window failed: %s\n", result.message);
				return 0;
			}

			// create a context for this window
			create_context(window, window_parameters);

			// activate the context
			attach_cocoa_context(window);

			// make this window main
			[window->cw makeMainWindow];

			return window;
		}

		void destroy(NativeWindow* window, DestroyWindowBehavior)
		{
			MEMORY_DELETE(window, get_platform_allocator());
		}

		void activate_context(NativeWindow* window)
		{
			cocoa::cocoa_native_window* cocoawindow = cocoa::from(window);
			attach_cocoa_context(cocoawindow);
		}

		void deactivate_context(NativeWindow* /*window*/)
		{
			// Don't need a window pointer since GL just has a global context
			cocoa::detach_cocoa_context(nullptr);
		}

		void swap_buffers(NativeWindow* window)
		{
			cocoa::cocoa_native_window* cocoawindow = cocoa::from(window);
			swap_buffers(cocoawindow);
		}

		Frame get_frame(NativeWindow* window)
		{
			cocoa::cocoa_native_window* cocoa_window = cocoa::from(window);

			// grab the window's frame rectangle in screen coordinates (includes titlebar)
			NSRect window_frame = [cocoa_window->cw frame];

			// convert the frame to a content rect (usable size)
			NSRect content_frame = [cocoa_window->cw contentRectForFrameRect:window_frame];
			NSScreen* screen = [cocoa_window->cw screen];
			NSRect screen_frame = [screen visibleFrame];

			// the window's screen space origin is in the lower left
			// of the window's rect.
			// We accomodate for this by:
			// - subtracting the height of the window frame
			// - flipping the y coordinate (such that the origin is in the top left)
			// - adding the visible screen rect y origin to this (take the menu bar in account)
			Frame frame;
			frame.x = window_frame.origin.x;

			CGFloat top_left_y = (screen_frame.size.height - window_frame.origin.y - window_frame.size.height);
			frame.y = screen_frame.origin.y + top_left_y;
			frame.width = content_frame.size.width;
			frame.height = content_frame.size.height;
			return frame;
		}

		Frame get_render_frame(NativeWindow* window)
		{
			// Until I can test this on a retina display...
			return get_frame(window);
		}

		size_t screen_count()
		{
			return [[NSScreen screens] count];
		}

		Frame screen_frame(size_t screen_index)
		{
			NSScreen* screen = [[NSScreen screens] objectAtIndex:screen_index];
			NSRect frame = [screen convertRectToBacking:[screen frame]];
			Frame out_frame;
			out_frame.x = frame.origin.x;
			out_frame.y = frame.origin.y;
			out_frame.width = frame.size.width;
			out_frame.height = frame.size.height;
			return out_frame;
		}

		void focus(NativeWindow* window)
		{
			cocoa::cocoa_native_window* cocoawindow = cocoa::from(window);
			[cocoawindow->cw makeKeyAndOrderFront:nil];
		}

		void show_cursor(bool enable)
		{
			// https://developer.apple.com/library/mac/documentation/Cocoa/Reference/ApplicationKit/Classes/NSCursor_Class/
			// Each call to hide cursor must have a corresponding unhide call.

			if (cocoa::_state.is_cursor_visible != enable)
			{
				cocoa::_state.is_cursor_visible = enable;

				if (enable)
				{
					[NSCursor unhide];
				}
				else
				{
					[NSCursor hide];
				}
			}
		}

		// set the cursor (absolute screen coordinates)
		void set_cursor(float x, float y)
		{
			CGDirectDisplayID display = 0;
			CGPoint point;

			// this point should have the origin in the upper left.
			point.x = x;
			point.y = y;

			NSScreen* screen = [[[NSApplication sharedApplication] mainWindow] screen];
			assert([[NSApplication sharedApplication] mainWindow]);
			NSRect frame = [screen frame];
			cocoa::_state.last_mouse = NSMakePoint(x, frame.size.height - y);

			// As per the documentation, this won't generate any mouse movement
			// events.

			// Disassociate before the move and re-associate the cursor after.
			// This is very important, otherwise delta mouse values won't be
			// correctly generated!
			CGAssociateMouseAndMouseCursorPosition(false);
			CGDisplayMoveCursorToPoint(display, point);
			CGAssociateMouseAndMouseCursorPosition(true);
			CGAssociateMouseAndMouseCursorPosition(!cocoa::_state.is_in_relative_mouse_mode);
		}

		// get the cursor (absolute screen coordinates
		void get_cursor(float& x, float& y)
		{
			// mouseLocation returns the current mouse position in screen
			// coordinates. However, we must flip the Y-axis because
			// the origin is in the lower-left. Ours is the upper left.
			NSPoint current = [NSEvent mouseLocation];
			x = current.x;
			y = current.y-1;

			NSUInteger index = 0;
			for(NSScreen* screen in [NSScreen screens])
			{
				if (NSMouseInRect(current, [screen frame], NO))
				{
					y = [screen frame].size.height - y;
					return;
					break;
				}

				++index;
			}

			LOGW("Could not find screen for mouse location [%2.2f, %2.2f]\n", x, y);
		}

		void set_relative_mouse_mode(bool enable)
		{
			cocoa::_state.last_mouse = [NSEvent mouseLocation];
			CGAssociateMouseAndMouseCursorPosition(!enable);

			cocoa::_state.is_in_relative_mouse_mode = enable;
			cocoa::_state.ignore_next_event = true;

			// we must store the key_window in order to
			// constrain the mouse movement to a rect.
			if (enable)
			{
				// if we are entering relative mode,
				// we must center the cursor inside the key window.
				cocoa::center_cursor_inside_key_window();
			}
		}

		void set_mouse_tracking(bool enable)
		{
			cocoa::_state.is_tracking_mouse = enable;
		}

	} // namespace window
} // namespace platform
