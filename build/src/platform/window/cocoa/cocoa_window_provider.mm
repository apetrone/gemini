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

namespace platform
{
	namespace window
	{
		namespace cocoa
		{		
			const unsigned int kMaxKeys = 132;
			static input::Button key_map[ kMaxKeys ];
			static NSPoint _last_mouse;
			
			// We keep this so we can share GL contexts with all subsequent
			// windows created.
			static NSOpenGLContext* _share_context = nil;
			
			void populate_keymap()
			{
				memset(&key_map, 0, kMaxKeys*sizeof(input::Button));
				
				// values taken from <HIToolbox/Events.h>
				key_map[0x00] = input::KEY_A;
				key_map[0x01] = input::KEY_S;
				key_map[0x02] = input::KEY_D;
				key_map[0x03] = input::KEY_F;
				key_map[0x04] = input::KEY_H;
				key_map[0x05] = input::KEY_G;
				key_map[0x06] = input::KEY_Z;
				key_map[0x07] = input::KEY_X;
				key_map[0x08] = input::KEY_C;
				key_map[0x09] = input::KEY_V;
				key_map[0x0B] = input::KEY_B;
				key_map[0x0C] = input::KEY_Q;
				key_map[0x0D] = input::KEY_W;
				key_map[0x0E] = input::KEY_E;
				key_map[0x0F] = input::KEY_R;
				key_map[0x10] = input::KEY_Y;
				key_map[0x11] = input::KEY_T;
				key_map[0x12] = input::KEY_1;
				key_map[0x13] = input::KEY_2;
				key_map[0x14] = input::KEY_3;
				key_map[0x15] = input::KEY_4;
				key_map[0x16] = input::KEY_6;
				key_map[0x17] = input::KEY_5;
				key_map[0x18] = input::KEY_EQUALS;
				key_map[0x19] = input::KEY_9;
				key_map[0x1A] = input::KEY_7;
				key_map[0x1B] = input::KEY_MINUS;
				key_map[0x1C] = input::KEY_8;
				key_map[0x1D] = input::KEY_0;
				key_map[0x1E] = input::KEY_RBRACKET;
				key_map[0x1F] = input::KEY_O;
				key_map[0x20] = input::KEY_U;
				key_map[0x21] = input::KEY_LBRACKET;
				key_map[0x22] = input::KEY_I;
				key_map[0x23] = input::KEY_P;
				key_map[0x24] = input::KEY_RETURN;
				key_map[0x25] = input::KEY_L;
				key_map[0x26] = input::KEY_J;
				key_map[0x27] = input::KEY_QUOTE;
				key_map[0x28] = input::KEY_K;
				key_map[0x29] = input::KEY_SEMICOLON;
				key_map[0x2A] = input::KEY_BACKSLASH;
				key_map[0x2B] = input::KEY_COMMA;
				key_map[0x2C] = input::KEY_SLASH;
				key_map[0x2D] = input::KEY_N;
				key_map[0x2E] = input::KEY_M;
				key_map[0x2F] = input::KEY_PERIOD;
				key_map[0x30] = input::KEY_TAB;
				key_map[0x31] = input::KEY_SPACE;
				key_map[0x32] = input::KEY_TILDE; // (GRAVE -> TILDE)
				key_map[0x33] = input::KEY_DELETE;
				key_map[0x35] = input::KEY_ESCAPE;
				key_map[0x36] = input::KEY_ROSKEY;
				key_map[0x37] = input::KEY_LOSKEY;
				key_map[0x38] = input::KEY_LSHIFT;
				key_map[0x39] = input::KEY_CAPSLOCK;
				key_map[0x3A] = input::KEY_LALT; // (LOPTION -> LALT)
				key_map[0x3B] = input::KEY_LCONTROL;
				key_map[0x3C] = input::KEY_RSHIFT;
				key_map[0x3D] = input::KEY_RALT; // (ROPTION -> RALT)
				key_map[0x3E] = input::KEY_RCONTROL;
				key_map[0x3F] = input::KEY_FUNCTION;
				key_map[0x40] = input::KEY_F17;
				key_map[0x41] = input::KEY_PERIOD;
				key_map[0x43] = input::KEY_NUMPAD_MULTIPLY;
				key_map[0x45] = input::KEY_NUMPAD_PLUS;
				key_map[0x47] = input::KEY_NUMLOCK; // (CLEAR -> NUMLOCK)
		//		key_map[0x48] = input::KEY_VOLUME_UP;
		//		key_map[0x49] = input::KEY_VOLUME_DOWN;
		//		key_map[0x4A] = input::KEY_MUTE;
				key_map[0x4B] = input::KEY_NUMPAD_DIVIDE;
				key_map[0x4C] = input::KEY_NUMPAD_ENTER;
				key_map[0x4E] = input::KEY_NUMPAD_MINUS;
				key_map[0x4F] = input::KEY_F18;
				key_map[0x50] = input::KEY_F19;
				key_map[0x51] = input::KEY_NUMPAD_EQUALS;
				key_map[0x52] = input::KEY_NUMPAD0;
				key_map[0x53] = input::KEY_NUMPAD1;
				key_map[0x54] = input::KEY_NUMPAD2;
				key_map[0x55] = input::KEY_NUMPAD3;
				key_map[0x56] = input::KEY_NUMPAD4;
				key_map[0x57] = input::KEY_NUMPAD5;
				key_map[0x58] = input::KEY_NUMPAD6;
				key_map[0x59] = input::KEY_NUMPAD7;
				key_map[0x5A] = input::KEY_F20;
				key_map[0x5B] = input::KEY_NUMPAD8;
				key_map[0x5C] = input::KEY_NUMPAD9;
				key_map[0x60] = input::KEY_F5;
				key_map[0x61] = input::KEY_F6;
				key_map[0x62] = input::KEY_F7;
				key_map[0x63] = input::KEY_F3;
				key_map[0x64] = input::KEY_F8;
				key_map[0x65] = input::KEY_F9;
				key_map[0x67] = input::KEY_F11;
				key_map[0x69] = input::KEY_F13;
				key_map[0x6A] = input::KEY_F16;
				key_map[0x6B] = input::KEY_F14;
				key_map[0x6D] = input::KEY_F10;
				key_map[0x6E] = input::KEY_MENU;
				key_map[0x6F] = input::KEY_F12;
				key_map[0x71] = input::KEY_F15;
				key_map[0x72] = input::KEY_INSERT;
				key_map[0x73] = input::KEY_HOME;
				key_map[0x74] = input::KEY_PAGEUP;
				key_map[0x75] = input::KEY_DELETE;
				key_map[0x76] = input::KEY_F4;
				key_map[0x77] = input::KEY_END;
				key_map[0x78] = input::KEY_F2;
				key_map[0x79] = input::KEY_PAGEDN;
				key_map[0x7A] = input::KEY_F1;
				key_map[0x7B] = input::KEY_LEFT;
				key_map[0x7C] = input::KEY_RIGHT;
				key_map[0x7D] = input::KEY_DOWN;
				key_map[0x7E] = input::KEY_UP;
			}

			input::Button convert_keycode(unsigned short keycode)
			{
				input::Button button = key_map[keycode];
			
				if (button == 0)
				{
					PLATFORM_LOG(platform::LogMessageType::Info, "UNKNOWN KEYCODE: %i\n", keycode);
					return input::Button::KEY_INVALID;
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
				cocoa_native_window(const WindowDimensions& window_dimensions) :
					NativeWindow(window_dimensions),
					cw(nil),
					context(nil)
				{
				}
				
				virtual void* get_native_handle() const
				{
					return cw;
				}
				
				CocoaWindow* cw;
				NSOpenGLContext* context;
			};

			Result create_window(cocoa_native_window* native_window, const Parameters& params)
			{
				CocoaWindow* window;
				
				// create a frame for the new window
				NSRect frame = NSMakeRect(params.window.x, params.window.y, params.window.width, params.window.height);
				
				// get the screen frame where the window will be placed
				NSRect screen_frame = [[[NSScreen screens] objectAtIndex:params.target_display] frame];
				
				// determine the window mask based on parameters
				NSUInteger window_mask = 0;
				if (params.enable_fullscreen)
				{
					window_mask = NSBorderlessWindowMask;
				}
				else
				{
					window_mask = NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask;
				}
				
				if (!params.enable_resize)
				{
					window_mask &= ~NSResizableWindowMask;
				}
				
				// create a new window
				window = [[CocoaWindow alloc] initWithContentRect:frame styleMask:window_mask backing:NSBackingStoreBuffered defer:NO];
				if (!window)
				{
					return platform::Result(platform::Result::Failure, "create window failed");
				}
				
				// link the window to our handle
				window.instance = native_window;
				native_window->cw = window;
				
				[window setBackgroundColor:[NSColor redColor]];
				[window makeKeyAndOrderFront: nil];
				[window setTitle: [NSString stringWithUTF8String: params.window_title]];
				[window setAcceptsMouseMovedEvents: YES];
				[window setReleasedWhenClosed: YES];
				
				id delegate = [NSApp delegate];
				[window setDelegate: delegate];

				// try to center the window in the screen
				{
					NSPoint origin = NSMakePoint(
												 screen_frame.origin.x + (screen_frame.size.width/2) - (params.window.width/2),
												 screen_frame.origin.y + (screen_frame.size.height/2) - (params.window.height/2)
												 );
					[window center];
					[window setFrameOrigin: origin];
				}
				
				if (params.enable_fullscreen)
				{
					[window setLevel: CGShieldingWindowLevel()];
				}
				
				return platform::Result(platform::Result::Success);
			}

			void create_context(cocoa_native_window* window)
			{
				// ---------------------------------------------------------------------
				// setup the opengl context
				// ---------------------------------------------------------------------
				NSOpenGLPixelFormatAttribute attributes[16];
				
				// https://developer.apple.com/library/prerelease/mac/documentation/Cocoa/Reference/ApplicationKit/Classes/NSOpenGLPixelFormat_Class/index.html#//apple_ref/c/tdef/NSOpenGLPixelFormatAttribute
				
				size_t index = 0;
				
				attributes[index++] = NSOpenGLPFADepthSize;
				attributes[index++] = 24;
				
				attributes[index++] = NSOpenGLPFAColorSize;
				attributes[index++] = 24;
				
				attributes[index++] = NSOpenGLPFAOpenGLProfile;
				attributes[index++] = NSOpenGLProfileVersion3_2Core;
				
	//			attributes[index++] = NSOpenGLPFAMultisample;
	//			attributes[index++] = NSOpenGLPFASampleBuffers;
	//			attributes[index++] = (NSOpenGLPixelFormatAttribute)1;
	//
	//			attributes[index++] = NSOpenGLPFASamples;
	//			attributes[index++] = (NSOpenGLPixelFormatAttribute)16;
				
				attributes[index++] = NSOpenGLPFAAccelerated;
				attributes[index++] = NSOpenGLPFADoubleBuffer;
				
				// terminate
				attributes[index++] = nil;
				
				// TODO: add error handling
				NSOpenGLPixelFormat* format = [[NSOpenGLPixelFormat alloc] initWithAttributes: attributes];
				assert(format != nil);
				
				
				window->context = [[NSOpenGLContext alloc] initWithFormat: format shareContext: _share_context];
				
				if (_share_context == nil)
				{
					_share_context = window->context;
				}
				
				GLint opacity = 1; // 1: opaque; 0: transparent
				GLint wait_for_vsync = 1; // 1: on, 0: off
				
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
			
			void detach_cocoa_context(cocoa_native_window* window)
			{
				[NSOpenGLContext clearCurrentContext];
			}
			
			void swap_buffers(cocoa_native_window* window)
			{
				[window->context flushBuffer];
			}
			
			
			void dispatch_mouse_event(NSEvent* event)
			{
				// ignore events outside of the client area
				if (!NSPointInRect([event locationInWindow], [[[event window] contentView] frame]))
					return;

				kernel::MouseEvent ev;
				ev.is_down = false;
				ev.subtype = kernel::MouseButton;

				switch([event type])
				{
					case NSLeftMouseDragged:
					case NSRightMouseDragged:
					case NSOtherMouseDragged:
						return;
				
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
			
			// This expects the current coordinates from (mouseLocation)
			// as they're in screen coordinates.
			NSPoint compute_mouse_delta(const NSPoint& current)
			{
				return NSMakePoint(
					(current.x - _last_mouse.x),
					(_last_mouse.y - current.y)
				);
			}
			
			void dispatch_mouse_moved_event(NSEvent* the_event)
			{
				CocoaWindow* window = static_cast<CocoaWindow*>([the_event window]);
				
				CGFloat title_bar_height;
				CGFloat fixed_height;
				
				// convert mouse from window to view
				NSPoint mouse_location = [[window contentView] convertPoint: [the_event locationInWindow] fromView:nil];
				
				// calculate title bar height of the window
				NSRect frame = [window frame];
				NSRect content_rect = [NSWindow contentRectForFrameRect:frame styleMask:NSTitledWindowMask];
				title_bar_height = (frame.size.height - content_rect.size.height);
				
				// We subtract height from mouse location y to invert the y-axis; since
				// NSPoint's origin is in the lower left corner.
				// The fixed height is the window frame minus the title bar height
				// and we also subtract 1.0 because convertPoint starts from a base of 1
				// according to the Cocoa docs.
				fixed_height = frame.size.height - title_bar_height - 1.0f;
				
				kernel::MouseEvent event;
				event.subtype = kernel::MouseMoved;
				event.mx = mouse_location.x;
				event.my = fixed_height - mouse_location.y;
				
				// compute the delta using screen coordinates;
				// which is what mouseLocation returns.
				NSPoint delta = compute_mouse_delta([NSEvent mouseLocation]);
				event.dx = (int)delta.x;
				event.dy = (int)delta.y;
				_last_mouse = [NSEvent mouseLocation];
				
				// don't dispatch any events outside of the window
				if (event.mx >= 0 && event.my >= 0 && (event.mx <= frame.size.width) && (event.my <= fixed_height))
				{
					kernel::event_dispatch(event);
					return;
				}
			}
			
			void track_mouse_location(const NSPoint& pt)
			{
				// track the mouse position at a global scope
				// NS mouse coordinates have a lower left origin.
				// CG mouse coordinates have an upper left origin.
							
				// compute deltas
				// y axis is inverted on purpose to preserve 0,0 topleft.
	//			NSPoint delta = compute_mouse_delta(pt);
				
	//			NSLog(@"mouse pos: %f %f [%f %f]", pt.x, pt.y, delta.x, delta.y);
				
	//			_last_mouse = pt;

				// TODO: Investigate generating custom events for mouse capture
				// http://stackoverflow.com/questions/5840873/how-to-tell-the-difference-between-a-user-tapped-keyboard-event-and-a-generated
			}
			
			void process_event_loop()
			{
				while(true)
				{
					NSEvent* event = [NSApp
									  nextEventMatchingMask:NSAnyEventMask
									  untilDate:[NSDate distantPast]
									  inMode:NSDefaultRunLoopMode
									  dequeue:YES
									  ];
					
					if (event == nil)
					{
						break;
					}
					
					switch([event type])
					{
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
							dispatch_mouse_event(event);
							break;
							
							//				case NSMouseMoved:
							//					track_mouse_location([NSEvent mouseLocation]);
							//					break;
							
						case NSKeyDown:
						case NSKeyUp:
							dispatch_key_event(event);
							break;
						default:
							break;
					}
					
					[NSApp sendEvent: event];
				}
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
				return Result(Result::Failure, "The requested rendering backend is not supported");
			}
			
			// populate the osx keymap
			cocoa::populate_keymap();
			
			return Result(Result::Success);
		}
		
		void shutdown()
		{
			
		}
		
		void dispatch_events()
		{
			cocoa::process_event_loop();
		}


		NativeWindow* create(const Parameters& window_parameters)
		{
			cocoa::cocoa_native_window* window = MEMORY_NEW(cocoa::cocoa_native_window, get_platform_allocator())(window_parameters.window);
			
			platform::Result result = create_window(window, window_parameters);
			if (result.failed())
			{
				PLATFORM_LOG(LogMessageType::Error, "create_window failed: %s\n", result.message);
				return 0;
			}

			// create a context for this window
			create_context(window);
			
			// activate the context
			attach_cocoa_context(window);
			
			NSRect bounds = [[window->cw contentView] bounds];
			bounds = [[window->cw contentView] convertRectToBacking: bounds];
			
			window->dimensions.render_width = bounds.size.width;
			window->dimensions.render_height = bounds.size.height;
			
			return window;
		}
		
		void destroy(NativeWindow* window)
		{
			MEMORY_DELETE(window, get_platform_allocator());
		}
	
		void activate_context(NativeWindow* window)
		{
			cocoa::cocoa_native_window* cocoawindow = cocoa::from(window);
			attach_cocoa_context(cocoawindow);
		}
	
		void deactivate_context(NativeWindow* window)
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
			Frame frame;
			return frame;
		}

		Frame get_render_frame(NativeWindow* window)
		{
			Frame frame;
			return frame;
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
			// this is DEPRECATED!
			//				CFDictionaryRef mode_info;
			//				int refresh_rate;
			//				mode_info = CGDisplayCurrentMode(screen_index);
			//				if (mode_info)
			//				{
			//					CFNumberRef value = (CFNumberRef)CFDictionaryGetValue(mode_info, kCGDisplayRefreshRate);
			//					if (value)
			//					{
			//						CFNumberGetValue(value, kCFNumberIntType, &refresh_rate);
			//						if (refresh_rate == 0)
			//						{
			//							// this is an LCD screen
			//							refresh_rate = 60;
			//						}
			//					}
			//				}
			//
			//				PLATFORM_LOG(LogMessageType::Info, "refresh rate of screen %i is %i Hz\n", screen_index, refresh_rate);
		}
		
		void focus(NativeWindow* window)
		{
			cocoa::cocoa_native_window* cocoawindow = cocoa::from(window);
			[cocoawindow->cw makeKeyAndOrderFront:nil];
		}
		
		void show_cursor(bool enable)
		{
			if (enable)
				[NSCursor unhide];
			else
				[NSCursor hide];
		}
		
		// set the cursor (absolute screen coordinates)
		void set_cursor(int x, int y)
		{
			
		}
		
		// get the cursor (absolute screen coordinates
		void get_cursor(int& x, int& y)
		{
			
		}
	} // namespace window
} // namespace platform
