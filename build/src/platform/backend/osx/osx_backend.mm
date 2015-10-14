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

#include <unistd.h>

#include <IOKit/hid/IOHIDKeys.h>
#include <IOKit/hidsystem/IOHIDShared.h>

namespace platform
{
	namespace cocoa
	{
		// during startup, we save the old mouse acceleration so we can
		// restore it during shutdown.
		static double _saved_mouse_acceleration = 0.0f;
		static io_connect_t _mouse_device = 0;

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

		// try to find the HIDsystem
#if 0

		mach_port_t master_port;
		kern_return_t kern_result;
		io_iterator_t matching_services;


		kern_result = IOMasterPort(MACH_PORT_NULL, &master_port);
		if (kern_result != KERN_SUCCESS)
		{
			return Result::failure("Unable to obtain master port");
		}

		CFMutableDictionaryRef classes_to_match = IOServiceMatching("IOHIDSystem");
		if (!classes_to_match)
		{
			return Result::failure("IOServiceMatching returned a NULL dictionary");
		}


		// find all IOHIDSystem classes
		kern_result = IOServiceGetMatchingServices(master_port, classes_to_match, &matching_services);
		if (kern_result != KERN_SUCCESS)
		{
			// No such device was found.
			return Result::failure(core::str::format("IOServiceGetMatchingServices returned %d\n", kern_result));
		}

		io_object_t service_interface;

		if ((service_interface = IOIteratorNext(matching_services)))
		{
			// try to open a connection to the HIDSystem User client
			kern_result = IOServiceOpen(service_interface, mach_task_self(), kIOHIDParamConnectType, &cocoa::_mouse_device);
			if (kern_result != KERN_SUCCESS)
			{
				PLATFORM_LOG(LogMessageType::Warning, "IOServiceOpen return error 0x%X\n", kern_result);
				IOObjectRelease(cocoa::_mouse_device);
			}
			else
			{
				// try to fetch mouse acceleration from the device
				// if this succeeds, the device is a mouse.
				double acceleration = 0.0;
				kern_result = IOHIDGetMouseAcceleration(cocoa::_mouse_device, &acceleration);
				if (kern_result != KERN_SUCCESS)
				{
					// this is not a mouse device.
					IOServiceClose(cocoa::_mouse_device);
					IOObjectRelease(cocoa::_mouse_device);
				}
				else
				{
					// we have found a mouse
					PLATFORM_LOG(LogMessageType::Info, "Found mouse; acceleration is: %2.2f\n", acceleration);

					cocoa::_saved_mouse_acceleration = acceleration;

					acceleration = 0.0f;
					IOHIDSetMouseAcceleration(cocoa::_mouse_device, acceleration);

					IOHIDGetMouseAcceleration(cocoa::_mouse_device, &acceleration);
					PLATFORM_LOG(LogMessageType::Info, "Reset Mouse Acceleration to %2.2f\n", acceleration);
				}
			}
		}


		// disable mouse coalescing
		[NSEvent setMouseCoalescingEnabled:NO];
#endif
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
		if (cocoa::_mouse_device != 0)
		{
			IOHIDSetMouseAcceleration(cocoa::_mouse_device, cocoa::_saved_mouse_acceleration);
		}
	}

	void backend_log(LogMessageType, const char* message)
	{
		NSLog(@"%@", [NSString stringWithUTF8String:message]);
	}




	// ---------------------------------------------------------------------
	// system
	// ---------------------------------------------------------------------
	size_t system_pagesize()
	{
		return getpagesize();
	}

	size_t system_processor_count()
	{
		return [[NSProcessInfo processInfo] processorCount];
	}

} // namespace platform
