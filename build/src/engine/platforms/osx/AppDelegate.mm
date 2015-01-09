// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM,OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
// -------------------------------------------------------------
#import "AppDelegate.h"

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
	gemini::memory::shutdown();
}

@end
