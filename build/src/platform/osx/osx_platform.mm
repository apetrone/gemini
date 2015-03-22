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

#include "platform.h"

#if TARGET_OS_MAC && !TARGET_OS_IPHONE
	#import <Cocoa/Cocoa.h>
	#import <AppKit/AppKit.h>
#else
	#import <Foundation/Foundation.h>
#endif

#include "osx_platform.h"



namespace platform
{
	NSAutoreleasePool* pool;
	
	Result osx_startup()
	{
		pool = [[NSAutoreleasePool alloc] init];
//		
//		NSProcessInfo* processInfo = [NSProcessInfo processInfo];
//		
//		const int MB = (1024*1024);
//		const int GB = MB*1024;
//		NSLog(@"physical memory: %lluMB", processInfo.physicalMemory/MB);
//		
//		NSLog(@"%@", processInfo.operatingSystemName);
//		NSLog(@"%@", processInfo.operatingSystemVersionString);
//		NSTimeInterval uptime = processInfo.systemUptime;
//
//		NSLog(@"uptime: %f", uptime);
		
		return Result(Result::Success);
	}
	
	void osx_shutdown()
	{
		[pool release];
		pool = 0;
	}
	
	int osx_run_application(int argc, const char* argv[])
	{
		return ::NSApplicationMain(argc, argv);
	}
	
	Result osx_program_directory(char* path, size_t size)
	{
		Result result(Result::Success);
		NSString * bundle_path = [[NSBundle mainBundle] bundlePath];
		if (bundle_path)
		{
			[bundle_path getCString:path maxLength:size encoding:NSUTF8StringEncoding];
		}
		else
		{
			result = Result(Result::Failure, "Unable mainBundle reference is invalid!");
		}
		return result;
	}
} // namespace platform