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

#import "osx_openglview.h"
#import "osx_window.h"

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
	
	NSPoint mouse_location = [[the_event window] mouseLocationOutsideOfEventStream];
	
	// subtract the window height from the contentView height
	// this will give us the size of the title bar
	NSRect render_frame = [[window screen] convertRectToBacking: [window frame]];
	title_bar_height = render_frame.size.height - render_frame.size.height;
	fixed_height = render_frame.size.height - title_bar_height;
	
	kernel::MouseEvent event;
	event.subtype = kernel::MouseMoved;
	event.mx = mouse_location.x;
	event.my = fixed_height - mouse_location.y; // top left is the origin, so we correct this here.
	
	if (event.mx >= 0 && event.my >= 0 && (event.mx <= render_frame.size.width) && (event.my <= fixed_height))
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