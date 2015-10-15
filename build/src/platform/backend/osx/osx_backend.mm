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

		void close_service_and_release_object(io_connect_t device)
		{
			IOServiceClose(device);
			IOObjectRelease(device);
		}

		io_connect_t acquire_mouse_device()
		{
			mach_port_t master_port;
			kern_return_t kern_result;
			io_iterator_t matching_services;

			kern_result = IOMasterPort(MACH_PORT_NULL, &master_port);
			if (kern_result != KERN_SUCCESS)
			{
				PLATFORM_LOG(LogMessageType::Warning, "Unable to obtain master port\n");
				return 0;
			}

			CFMutableDictionaryRef classes_to_match = IOServiceMatching("IOHIDSystem");
			if (!classes_to_match)
			{
				PLATFORM_LOG(LogMessageType::Warning, "IOServiceMatching returned a NULL dictionary");
				return 0;
			}


			// find all IOHIDSystem classes
			kern_result = IOServiceGetMatchingServices(master_port, classes_to_match, &matching_services);
			if (kern_result != KERN_SUCCESS)
			{
				// No such device was found.
				PLATFORM_LOG(LogMessageType::Warning, "IOServiceGetMatchingServices returned %d\n", kern_result);
				return 0;
			}

			io_object_t service_interface;
			io_connect_t device = 0;

			if ((service_interface = IOIteratorNext(matching_services)))
			{
				// try to open a connection to the HIDSystem User client
				kern_result = IOServiceOpen(service_interface, mach_task_self(), kIOHIDParamConnectType, &device);
				if (kern_result != KERN_SUCCESS)
				{
					PLATFORM_LOG(LogMessageType::Warning, "IOServiceOpen return error 0x%X\n", kern_result);
					IOObjectRelease(device);
				}
				else
				{
					// try to fetch mouse acceleration from the device
					// if this succeeds, the device is a mouse.
					double acceleration = 0.0;
					kern_result = IOHIDGetMouseAcceleration(device, &acceleration);
					if (kern_result != KERN_SUCCESS)
					{
						// this is not a mouse device.
						close_service_and_release_object(device);
					}
					else
					{
						// this device responded correctly
						return device;
					}
				}
			}

			// failure condition
			return 0;
		}
	} // namespace cocoa

	// ---------------------------------------------------------------------
	// backend
	// ---------------------------------------------------------------------
	Result backend_startup()
	{
		cocoa::_mouse_device = cocoa::acquire_mouse_device();
		if (cocoa::_mouse_device)
		{
			kern_return_t kern_result = IOHIDGetMouseAcceleration(cocoa::_mouse_device, &cocoa::_saved_mouse_acceleration);
			if (kern_result != KERN_SUCCESS)
			{
				PLATFORM_LOG(LogMessageType::Warning, "Unable to fetch mouse acceleration from device!\n");
				cocoa::close_service_and_release_object(cocoa::_mouse_device);
				cocoa::_mouse_device = 0;
			}
			else
			{
				// disable mouse acceleration
				double acceleration = 0.0f;
				IOHIDSetMouseAcceleration(cocoa::_mouse_device, acceleration);
			}
		}

		// disable mouse coalescing
		[NSEvent setMouseCoalescingEnabled:NO];

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
			// restore the saved mouse acceleration
			IOHIDSetMouseAcceleration(cocoa::_mouse_device, cocoa::_saved_mouse_acceleration);

			// close the mouse/device service
			cocoa::close_service_and_release_object(cocoa::_mouse_device);
		}
	}

	void backend_log(LogMessageType, const char* message)
	{
		NSLog(@"%@", [NSString stringWithUTF8String:message]);
	}




	// ---------------------------------------------------------------------
	// system
	// ---------------------------------------------------------------------
	size_t system_pagesize_bytes()
	{
		return getpagesize();
	}

	size_t system_processor_count()
	{
		return [[NSProcessInfo processInfo] processorCount];
	}

	double system_uptime_seconds()
	{
		// [uptime] is always specified in seconds, yielding sub-millisecond
		// precision over a range of 10,000 years. (NSTimeInterval docs)
		return [[NSProcessInfo processInfo] systemUptime];
	}

	core::StackString<64> system_version_string()
	{
		return cocoa::nsstring_to_stackstring<core::StackString<64>>(
			[[NSProcessInfo processInfo] operatingSystemVersionString]
		);
	}
} // namespace platform
