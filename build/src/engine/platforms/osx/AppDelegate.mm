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
#import "AppDelegate.h"

#import <platform/platform.h>
#import "platforms/desktop/kernel_desktop.h"

static bool has_started = false;
gemini::DesktopKernel _desktop_kernel( 0, 0 );

@implementation AppDelegate

-(CGFloat)calculate_titlebar_height
{
	NSWindow *mainWindow = [[NSApplication sharedApplication] mainWindow];
	
	NSRect frame = [mainWindow frame];
	NSRect content_rect = [NSWindow contentRectForFrameRect:frame styleMask:NSTitledWindowMask];
	
	return (frame.size.height - content_rect.size.height);
}

-(void)run_kernel
{
	// On Mac, the window created is actually larger than requested
	// to account for the added height of the title bar.
	// Unfortunately, the OpenGL drawable surface contains the area
	// used by the title bar, so we have to use that in our screen-space
	// calculations.
	gemini::kernel::instance()->parameters().titlebar_height = [self calculate_titlebar_height];
	
	while( gemini::kernel::instance()->is_active() )
	{
		gemini::kernel::tick();
	}

	[[NSApplication sharedApplication] terminate:nil];
}



- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
//	NSLog( @"applicationDidFinishLaunching" );
}

-(void)applicationWillResignActive:(NSNotification *)notification
{
//	NSLog( @"applicationWillResignActive" );
	if ( has_started )
	{
		gemini::kernel::Event<gemini::kernel::System> event;
		event.subtype = gemini::kernel::WindowLostFocus;
		gemini::kernel::event_dispatch(event);
	}
}

-(void)applicationDidBecomeActive:(NSNotification *)notification
{
//	NSLog( @"applicationDidBecomeActive" );	
	
	if ( !has_started )
	{
		has_started = true;
		gemini::kernel::startup( &_desktop_kernel );

// http://fredandrandall.com/blog/2011/09/08/how-to-make-your-app-open-in-full-screen-on-lion/
//	[window setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
//	[window toggleFullScreen:nil];

		[self performSelectorOnMainThread:@selector(run_kernel) withObject:self waitUntilDone:NO];
	}
	else
	{
		gemini::kernel::Event<gemini::kernel::System> event;
		event.subtype = gemini::kernel::WindowGainFocus;
		gemini::kernel::event_dispatch(event);
	}
}

-(void)applicationDidChangeScreenParameters:(NSNotification *)notification
{
	// detect GPU switching
	NSLog( @"applicationdidChangeScreenParameters");
}

-(void)applicationWillTerminate:(NSNotification *)notification
{
//	NSLog( @"applicationWillTerminate" );
	gemini::kernel::shutdown();
	platform::shutdown();
}

@end
