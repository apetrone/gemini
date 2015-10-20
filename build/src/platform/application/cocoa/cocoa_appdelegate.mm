// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone
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
#import "cocoa_appdelegate.h"

#import "platform.h"
#import "kernel.h"

static bool has_started = false;

@implementation AppDelegate

-(CGFloat)calculate_titlebar_height
{
	NSWindow *main_window = [[NSApplication sharedApplication] mainWindow];
	
	NSRect frame = [main_window frame];
	NSRect content_rect = [NSWindow contentRectForFrameRect:frame styleMask:NSTitledWindowMask];
	
	return (frame.size.height - content_rect.size.height);
}

-(void)run_kernel
{
	while(kernel::instance() && kernel::instance()->is_active())
	{
		kernel::tick();
	}

	[[NSApplication sharedApplication] terminate:nil];
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
}

-(void)applicationWillResignActive:(NSNotification *)notification
{
	if (has_started)
	{
		kernel::Event<kernel::System> event;
		event.subtype = kernel::WindowLostFocus;
		kernel::event_dispatch(event);
	}
}

-(void)applicationDidBecomeActive:(NSNotification *)notification
{
	if (!has_started)
	{
		has_started = true;
			
		platform::startup();

		kernel::startup();

		[self performSelectorOnMainThread:@selector(run_kernel) withObject:self waitUntilDone:NO];
	}
	else
	{
		kernel::Event<kernel::System> event;
		event.subtype = kernel::WindowGainFocus;
		kernel::event_dispatch(event);
	}
}

-(void)applicationDidChangeScreenParameters:(NSNotification *)notification
{
	// detect GPU switching
//	NSLog( @"applicationdidChangeScreenParameters");
}

-(void)applicationWillTerminate:(NSNotification *)notification
{
	kernel::shutdown();
	platform::shutdown();
}



// ---------------------------------------------------------------------
// window events
// ---------------------------------------------------------------------

-(BOOL) windowShouldClose:(id)window
{
	// This may not be the best solution for now,
	// but I'm not sure what that is. Send an event and say a window closed?
	kernel::instance()->set_active(false);
	return YES;
}


@end
