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
#if TARGET_OS_MAC && !TARGET_OS_IPHONE
	#import <Cocoa/Cocoa.h>
#else
	#import <Foundation/Foundation.h>
#endif

#include "osx_platform.h"

namespace platform
{
	NSAutoreleasePool* pool;
	
	core::Result osx_startup()
	{
		pool = [[NSAutoreleasePool alloc] init];
		return core::Result(core::Result::Success);
	}
	
	void osx_shutdown()
	{
		[pool release];
		pool = 0;
	}
	
	core::Result osx_program_directory(char* path, size_t size)
	{
		core::Result result(core::Result::Success);
		NSString * bundle_path = [[NSBundle mainBundle] bundlePath];
		if (bundle_path)
		{
			[bundle_path getCString:path maxLength:size encoding:NSUTF8StringEncoding];
		}
		else
		{
			result = core::Result(core::Result::Failure, "Unable mainBundle reference is invalid!");
		}
		return result;
	}
}; // namespace platform