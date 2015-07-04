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
#include "platform.h"
#include "os/osx/osx_common.h"

#import "cocoa_openglview.h"
#import "cocoa_window.h"

#include "kernel.h"
#include "input.h"

@implementation cocoa_openglview

@synthesize context = _context;


// ---------------------------------------------------------------------
// utils
// ---------------------------------------------------------------------

void dispatch_mouse_moved_event(NSEvent* the_event)
{
	cocoa_window* window = static_cast<cocoa_window*>([the_event window]);
	
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

	// don't dispatch any events outside of the window
	if (event.mx >= 0 && event.my >= 0 && (event.mx <= frame.size.width) && (event.my <= fixed_height))
	{
		kernel::event_dispatch(event);
		return;
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



// ---------------------------------------------------------------------
// openglview implementation
// ---------------------------------------------------------------------

-(id)initWithFrame:(NSRect)frameRect
{
	self = [super initWithFrame: frameRect];
	if (self == nil)
		return nil;
	
	return self;
}

-(void)lockFocus
{
	[super lockFocus];
	
	if ([self.context view] != self)
	{
		[self.context setView: self];
	}
	
	[self.context makeCurrentContext];
}

-(void)dealloc
{
	self.context = nil;
}

-(BOOL) isOpaque
{
	return YES;
}

-(BOOL) acceptsFirstResponder
{
	return YES;
}

// ---------------------------------------------------------------------
// events
// ---------------------------------------------------------------------

-(void) windowResized:(NSNotification*) notification
{
	cocoa_window* window = static_cast<cocoa_window*>([notification object]);
	NSRect frame = [[window contentView] frame];
	
	kernel::SystemEvent ev;
	ev.subtype = kernel::WindowResized;
	ev.window_width = frame.size.width;
	ev.window_height = frame.size.height;

	// also fetch render size for this window
	NSRect render_frame = [[window screen] convertRectToBacking: frame];
	ev.render_width = render_frame.size.width;
	ev.render_height = render_frame.size.height;
	
	// update the rendering context
	cocoa_openglview* view = [window contentView];
	[[view context] update];
	
	kernel::event_dispatch(ev);
}

-(void) mouseMoved:(NSEvent*) event
{
	dispatch_mouse_moved_event(event);
}

-(void) mouseDragged:(NSEvent*) event
{
	dispatch_mouse_moved_event(event);
}

-(void) rightMouseDragged:(NSEvent *) event
{
	dispatch_mouse_moved_event(event);
}

-(void) otherMouseDragged:(NSEvent *) event
{
	dispatch_mouse_moved_event(event);
}

-(BOOL)resignFirstResponder
{
	return NO;
}

-(void)flagsChanged:(NSEvent *)event
{
	uint16_t last_keymods = platform::osx::keymod_state();
	uint16_t keymods = convert_cocoa_keymods([event modifierFlags]);

	kernel::KeyboardEvent ev;
	ev.key = platform::osx::convert_keycode([event keyCode]);
	ev.is_down = (keymods > last_keymods);
	ev.modifiers = keymods;
	kernel::event_dispatch(ev);

	// set the new keymod state
	platform::osx::keymod_state(keymods);

	[super flagsChanged:event];
}


@end