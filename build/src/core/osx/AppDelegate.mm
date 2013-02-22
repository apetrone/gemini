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

#import "core/desktop/kernel_desktop.hpp"
#import <xwl/xwl.h>

DesktopKernel _desktop_kernel( 0, 0 );

@implementation AppDelegate

-(void)run_kernel
{
	while( kernel::instance()->is_active() )
	{
		kernel::tick();
	}

	[[NSApplication sharedApplication] terminate:nil];
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
	NSLog( @"applicationDidFinishLaunching" );
}

-(void)applicationDidBecomeActive:(NSNotification *)notification
{
	NSLog( @"applicationDidBecomeActive" );	
	
	kernel::startup( &_desktop_kernel, "TestUniversal" );

// http://fredandrandall.com/blog/2011/09/08/how-to-make-your-app-open-in-full-screen-on-lion/
//	[window setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
//	[window toggleFullScreen:nil];

	[self performSelectorOnMainThread:@selector(run_kernel) withObject:self waitUntilDone:NO];
}

-(void)applicationDidChangeScreenParameters:(NSNotification *)notification
{
	// detect GPU switching
	NSLog( @"applicationdidChangeScreenParameters");
}

-(void)applicationWillTerminate:(NSNotification *)notification
{
	NSLog( @"applicationWillTerminate" );
	kernel::shutdown();
	memory::shutdown();
}

@end
