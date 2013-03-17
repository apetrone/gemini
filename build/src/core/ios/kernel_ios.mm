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
#include "kernel_ios.h"
#include <string.h>
#include <stdio.h>

#import <Foundation/Foundation.h>

void iOSKernel::startup()
{
	// get the current status bar notification and send that to the kernel on startup
//	UIInterfaceOrientation startup_orientation = [[UIApplication sharedApplication] statusBarOrientation];
	
	// start generating orientation notifications
	[[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
	

	UIScreen * mainscreen = [UIScreen mainScreen];
	UIScreenMode * screenmode = [mainscreen currentMode];
	CGSize size = [screenmode size];
	NSLog( @"device resolution: %g x %g", size.width, size.height );
	
	parameters().render_width = size.width;
	parameters().render_height = size.height;
	
	// setup device flags
	if ( [mainscreen scale] > 1.0 )
	{
		parameters().device_flags |= (kernel::DeviceSupportsRetinaDisplay);
	}
	
	if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone)
	{
		parameters().device_flags |= kernel::DeviceiPhone;
	}
	else
	{
		parameters().device_flags |= kernel::DeviceiPad;
	}
} // startup

void iOSKernel::register_services()
{
	
} // register_services

void iOSKernel::pre_tick()
{
} // pre_tick

void iOSKernel::post_tick()
{
} // post_tick

void iOSKernel::post_application_config( kernel::ApplicationResult result )
{
} // post_application_config

void iOSKernel::post_application_startup( kernel::ApplicationResult result )
{
} // post_application_startup

void iOSKernel::shutdown()
{
} // shutdown

void iOSKernel::set_view_size( int width, int height )
{
	
} // set_view_size

void iOSKernel::set_interface_orientation( UIInterfaceOrientation orientation )
{
} // set_interface_orientation

BOOL iOSKernel::should_change_orientation( UIInterfaceOrientation orientation )
{
	return YES;
} // should_change_orientation

void iOSKernel::will_resign_active()
{
	NSLog( @"will_resign_active" );
	[[UIDevice currentDevice] endGeneratingDeviceOrientationNotifications];
}

void iOSKernel::did_become_active()
{
	NSLog( @"did_become_active" );
	[[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];	
}

void iOSKernel::will_terminate()
{
	NSLog( @"will_terminate" );
}

void iOSKernel::did_receive_memory_warning()
{
    // Release any cached data, images, etc. that aren't in use.
	NSLog( @"did_receive_memory_warning" );
}

