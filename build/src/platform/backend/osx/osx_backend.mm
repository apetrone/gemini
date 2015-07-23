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
#include "platform_internal.h"
#include "cocoa_common.h"

namespace platform
{
	namespace cocoa
	{
		// ---------------------------------------------------------------------
		// utilities
		// ---------------------------------------------------------------------
		PathString to_string(NSString* string)
		{
			PathString output;

			// NSUInteger length = [string length];
			// assuming PathString can be resized...

			[string getCString:&output[0] maxLength:output.max_size() encoding:NSUTF8StringEncoding];
			output.recompute_size(&output[0]);
			return output;
		}
	}

	// ---------------------------------------------------------------------
	// backend
	// ---------------------------------------------------------------------
	Result backend_startup()
	{
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

		return Result::success();
	}

	int backend_run_application(int argc, const char** argv)
	{
		return ::NSApplicationMain(argc, argv);
	}

	void backend_shutdown()
	{
	}

	void backend_log(LogMessageType, const char* message)
	{
		NSLog(@"%@", [NSString stringWithUTF8String:message]);
	}
} // namespace platform
