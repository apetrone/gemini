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
	
} // startup

void iOSKernel::register_services()
{
	
} // register_services

void iOSKernel::pre_tick()
{
	NSLog( @"pre_tick" );
} // pre_tick

void iOSKernel::post_tick()
{
	NSLog( @"post_tick" );
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

void iOSKernel::setInterfaceOrientation( UIInterfaceOrientation orientation )
{
}

void iOSKernel::will_resign_active()
{
	NSLog( @"will_resign_active" );
}

void iOSKernel::did_become_active()
{
	NSLog( @"did_become_active" );
}

void iOSKernel::will_terminate()
{
	NSLog( @"will_terminate" );
}

