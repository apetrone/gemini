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

using namespace platform::window::cocoa;

@implementation CocoaOpenGLView

@synthesize context = _context;

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
	CocoaWindow* window = static_cast<CocoaWindow*>([notification object]);
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
	CocoaOpenGLView* view = [window contentView];
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
	uint16_t last_keymods = keymod_state();
	uint16_t keymods = convert_cocoa_keymods([event modifierFlags]);

	kernel::KeyboardEvent ev;
	ev.key = convert_keycode([event keyCode]);
	ev.is_down = (keymods > last_keymods);
	ev.modifiers = keymods;
	kernel::event_dispatch(ev);

	// set the new keymod state
	keymod_state(keymods);

	[super flagsChanged:event];
}


@end