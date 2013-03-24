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
#include "log.h"

#import <Foundation/Foundation.h>
#import <TargetConditionals.h>

void iOSKernel::startup()
{
	// get the current status bar notification and send that to the kernel on startup
	last_orientation = 0;
	set_interface_orientation( [[UIApplication sharedApplication] statusBarOrientation] );

	UIScreen * mainscreen = [UIScreen mainScreen];
	
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
	
	LOGV( "startup orientations (Interface): %s, (Device): %s\n", UIInterfaceOrientationToString(last_orientation), UIDeviceOrientationToString([[UIDevice currentDevice] orientation]) );
	
	// hide the status bar
	[[UIApplication sharedApplication] setStatusBarHidden:YES withAnimation:UIStatusBarAnimationFade];
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
	UIScreen * mainscreen = [UIScreen mainScreen];
	UIScreenMode * screenmode = [mainscreen currentMode];
	CGSize size = [screenmode size];
	NSLog( @"device resolution: %g x %g", size.width, size.height );
	
	parameters().window_width = size.width;
	parameters().window_height = size.height;
	parameters().render_width = size.width;
	parameters().render_height = size.height;
	
	// the orientation seems to be incorrect between launching the simulator and on the device.
	// this makes it consistent by rotating the window coords when running in the simulator.
#if	TARGET_IPHONE_SIMULATOR
	rotate_window_coordinates();
#endif
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

void iOSKernel::set_interface_orientation( UIInterfaceOrientation current_orientation )
{
	LOGV( "setting interface orientation to: %s\n", ::UIInterfaceOrientationToString(last_orientation));
	// this swaps the width and height of the window to adjust for landscape <-> portrait changes
	int previous_orientation_type = UIInterfaceOrientationIsLandscape(this->last_orientation);
	int current_orientation_type = UIInterfaceOrientationIsLandscape(current_orientation);
	
	if ( previous_orientation_type != current_orientation_type )
	{
		LOGV( "[ORIENTATION CHANGE] %s -> %s\n", UIInterfaceOrientationToString(current_orientation),
		UIInterfaceOrientationToString((UIInterfaceOrientation)current_orientation) );
		this->rotate_window_coordinates();
	}
	this->last_orientation = current_orientation;
} // set_interface_orientation

BOOL iOSKernel::should_change_orientation( UIInterfaceOrientation orientation )
{
	rotate_mask = 0;
	//kernelState.rotateMask |= (1 << UIInterfaceOrientationPortrait);
	//kernelState.rotateMask |= (1 << UIInterfaceOrientationPortraitUpsideDown);
	//kernelState.rotateMask |= (1 << UIInterfaceOrientationLandscapeLeft);
	rotate_mask |= (1 << UIInterfaceOrientationLandscapeRight);
	
	return rotate_mask & (1 << orientation);
} // should_change_orientation

void iOSKernel::device_orientation_changed( UIInterfaceOrientation orientation )
{
	set_interface_orientation( orientation );
} // device_orientation_changed

void iOSKernel::rotate_window_coordinates()
{
	int tmp = params.window_height;
	params.window_height = params.window_width;
	params.window_width = tmp;
	
	tmp = params.render_height;
	params.render_height = params.render_width;
	params.render_width = tmp;
	LOGV( "swapping width and height. now: %i %i | %i x %i\n", params.window_width, params.window_height, params.render_width, params.render_height );
}

void iOSKernel::will_resign_active()
{
	NSLog( @"will_resign_active" );
	[[UIDevice currentDevice] endGeneratingDeviceOrientationNotifications];
}

void iOSKernel::did_become_active()
{
	NSLog( @"did_become_active" );
	// start generating orientation notifications	
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

const char * UIInterfaceOrientationToString( UIInterfaceOrientation orientation )
{
	switch( orientation )
	{
		case UIInterfaceOrientationPortrait: return "UIInterfaceOrientationPortrait";
		case UIInterfaceOrientationPortraitUpsideDown: return "UIInterfaceOrientationPortraitUpsideDown";
		case UIInterfaceOrientationLandscapeLeft: return "UIInterfaceOrientationLandscapeLeft";
		case UIInterfaceOrientationLandscapeRight: return "UIInterfaceOrientationLandscapeRight";
	}
	
	return "Unknown";
} // UIInterfaceOrientationToString

const char * UIDeviceOrientationToString( UIDeviceOrientation orientation )
{
	switch( orientation )
	{
		case UIDeviceOrientationUnknown: return "UIDeviceOrientationUnknown";
		case UIDeviceOrientationPortrait: return "UIDeviceOrientationPortrait";
		case UIDeviceOrientationPortraitUpsideDown: return "UIDeviceOrientationPortraitUpsideDown";
		case UIDeviceOrientationLandscapeLeft: return "UIDeviceOrientationLandscapeLeft";
		case UIDeviceOrientationLandscapeRight: return "UIDeviceOrientationLandscapeRight";
		case UIDeviceOrientationFaceUp: return "UIDeviceOrientationFaceUp";
		case UIDeviceOrientationFaceDown: return "UIDeviceOrientationFaceDown";
	}
	
	return "Unknown";
} // UIDeviceOrientationToString