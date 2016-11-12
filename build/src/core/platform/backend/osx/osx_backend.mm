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

#include <core/logging.h>

// I haven't found a way to remove a variable from an Objective-C
// implementation. For now, we'll just disable that warning in objc.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

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
				LOGW("Unable to obtain master port\n");
				return 0;
			}

			CFMutableDictionaryRef classes_to_match = IOServiceMatching("IOHIDSystem");
			if (!classes_to_match)
			{
				LOGW("IOServiceMatching returned a NULL dictionary");
				return 0;
			}


			// find all IOHIDSystem classes
			kern_result = IOServiceGetMatchingServices(master_port, classes_to_match, &matching_services);
			if (kern_result != KERN_SUCCESS)
			{
				// No such device was found.
				LOGW("IOServiceGetMatchingServices returned %d\n", kern_result);
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
					LOGW("IOServiceOpen return error 0x%X\n", kern_result);
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
		} // acquire_mouse_device


		NSString* to_nsstring(const char* input)
		{
			return [NSString stringWithUTF8String:input];
		}

		NSArray* to_nsarray(Array<PathString>& values)
		{
			NSMutableArray* output = [[NSMutableArray alloc] init];

			for (PathString& item : values)
			{
				[output addObject: to_nsstring(item())];
			}

			return output;
		}

		char* cfstringref_to_utf8(CFStringRef string_ref)
		{
			assert(string_ref);

			CFIndex length = CFStringGetLength(string_ref);
			CFIndex size = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
			char* buffer = (char*)malloc(size);
			assert(buffer);

			if (cfstringref_to_buffer(string_ref, buffer, size))
			{
				return buffer;
			}

			free(buffer);
			return nullptr;
		} // cfstringref_to_utf8

		bool cfstringref_to_buffer(CFStringRef string_ref, char* buffer, size_t max_buffer_size)
		{
			if (CFStringGetCString(string_ref, buffer, max_buffer_size, kCFStringEncodingUTF8))
			{
				return true;
			}

			return false;
		} // cfstringref_to_buffer
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
				LOGW("Unable to fetch mouse acceleration from device!\n");
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

	void backend_update(float delta_milliseconds)
	{
		platform::window::dispatch_events();

		// TODO@APP: Poll joysticks?
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


	// ---------------------------------------------------------------------
	//
	// ---------------------------------------------------------------------
	Result show_open_dialog(const char* title, uint32_t open_flags, Array<PathString>& paths)
	{
		PathString data;
		NSOpenPanel* panel 				= [NSOpenPanel openPanel];
		panel.title 					= cocoa::to_nsstring(title);
		panel.showsResizeIndicator 		= YES;
		panel.showsHiddenFiles 			= (open_flags & OpenDialogFlags::ShowHiddenFiles) ? YES : NO;
		panel.canChooseDirectories 		= (open_flags & OpenDialogFlags::CanChooseDirectories) ? YES : NO;
		panel.canChooseFiles 			= (open_flags & OpenDialogFlags::CanChooseFiles) ? YES : NO;
		panel.canCreateDirectories 		= (open_flags & OpenDialogFlags::CanCreateDirectories) ? YES : NO;
		panel.allowsMultipleSelection 	= (open_flags & OpenDialogFlags::AllowMultiselect) ? YES : NO;
		//panel.allowedFileTypes = @[@"conf"];

		[panel setAllowsOtherFileTypes:YES];

		NSInteger modal_result = [panel runModal];

		if (modal_result == NSFileHandlingPanelOKButton)
		{
			NSURL* selection = panel.URLs[0];

			// We don't yet support multiple paths
			assert(panel.allowsMultipleSelection == NO);

			PathString path = cocoa::nsstring_to_stackstring<PathString>([selection.path stringByResolvingSymlinksInPath]);
			paths.push_back(path);

			return Result::success();
		}

		return Result::failure("User cancelled dialog");
	}


	Result show_save_dialog(const char* title,
							uint32_t save_flags,
							const Array<PlatformExtensionDescription>& extensions,
							const PathString& default_extension,
							PathString& filename)
	{

		NSSavePanel* panel 				= [NSSavePanel savePanel];
		panel.title 					= cocoa::to_nsstring(title);
		panel.showsHiddenFiles			= (save_flags & SaveDialogFlags::ShowHiddenFiles) ? YES : NO;
		panel.canCreateDirectories		= (save_flags & SaveDialogFlags::CanCreateDirectories) ? YES : NO;

		[panel setAllowsOtherFileTypes:YES];

		NSMutableArray* filetypes = [[NSMutableArray alloc] init];
		for(size_t index = 0; index < extensions.size(); ++index)
		{
			const PlatformExtensionDescription& description = extensions[index];
			[filetypes addObject:cocoa::to_nsstring(description.extension)];
		}


		[panel setAllowedFileTypes: filetypes];

		NSInteger modal_result = [panel runModal];
		if (modal_result == NSFileHandlingPanelOKButton)
		{
			filename = cocoa::nsstring_to_stackstring<PathString>([panel.URL.path stringByResolvingSymlinksInPath]);
			return Result::success();
		}

		return Result::failure("User cancelled dialog");
	}


	class CocoaProcess : public Process
	{
	public:
		CocoaProcess(NSString* launch_path, NSArray* arguments, NSString* working_directory)
		{
			task_completed = false;

			stdout_pipe = [[NSPipe alloc] init];
			NSFileHandle* standard_output_handle = [stdout_pipe fileHandleForReading];

			stderr_pipe = [[NSPipe alloc] init];
			NSFileHandle* standard_error_handle = [stderr_pipe fileHandleForReading];

			task = [[NSTask alloc] init];
			[task setLaunchPath: launch_path];
			[task setStandardOutput: stdout_pipe];
			[task setStandardError: stderr_pipe];
			[task setArguments: arguments];

			if (working_directory)
			{
				[task setCurrentDirectoryPath: working_directory];
			}

			// notify when stdout has data
			[[NSNotificationCenter defaultCenter]
				addObserverForName:NSFileHandleDataAvailableNotification
				object:standard_output_handle
				queue:nil
				usingBlock:^(NSNotification* notification) {
					NSData* data = [standard_output_handle availableData];
//					if (data && [data length])
					{
						on_stdout(data);
					}

					[standard_output_handle waitForDataInBackgroundAndNotify];
				}];

			// notify when stderr has data
			[[NSNotificationCenter defaultCenter]
				addObserverForName:NSFileHandleDataAvailableNotification
				object:standard_error_handle
				queue:nil
				usingBlock:^(NSNotification* notification) {
					NSData* data = [standard_error_handle availableData];
//					if (data && [data length])
					{
						on_stderr(data);
					}

					[standard_error_handle waitForDataInBackgroundAndNotify];
				}];

			// notify when the task finishes
			[[NSNotificationCenter defaultCenter]
				addObserverForName:NSTaskDidTerminateNotification
				object:task
				queue:nil
				usingBlock:^(NSNotification* notification) {
					task_completed = true;
				}];

			dispatch_async(
				dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_LOW, 0),
			   ^{
					[standard_output_handle waitForDataInBackgroundAndNotify];
					[standard_error_handle waitForDataInBackgroundAndNotify];

					@try
					{
						[task launch];
						[task waitUntilExit];
					}
					@catch (NSException* exception)
					{
						assert(0);
						@throw exception;
					}
			   });
		}


		void on_stdout(NSData* data)
		{
			NSString* output_string = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
			NSLog(@"stdout: %@ [length: %lu]", output_string, (unsigned long)[data length]);
		}

		void on_stderr(NSData* data)
		{
			NSString* error_string = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
			NSLog(@"stderr: %@ [length: %lu]", error_string, (unsigned long)[data length]);
		}

		void on_completed()
		{
			task_completed = true;
		}

		bool has_completed()
		{
			return task_completed;
		}

		void terminate()
		{
			[task terminate];
		}

	private:
		NSTask* task;

		NSPipe* stdout_pipe;
		NSPipe* stderr_pipe;

		bool task_completed;
	};

	Process* process_create(const char* executable_path, const Array<PathString>& arguments, const char* working_directory)
	{
		NSArray* argument_array = cocoa::to_nsarray(const_cast<Array<PathString>&>(arguments));

		NSString* requested_working_directory = nil;
		if (working_directory)
		{
			requested_working_directory = cocoa::to_nsstring(working_directory);
		}

		Process* proc = MEMORY_NEW(CocoaProcess, core::memory::global_allocator())(
			cocoa::to_nsstring(executable_path),
			argument_array,
			requested_working_directory
		);
		return proc;
	}

	void process_destroy(Process* process)
	{
		CocoaProcess* instance = static_cast<CocoaProcess*>(process);
		if (process && process_is_running(process))
		{
			instance->terminate();
		}
		MEMORY_DELETE(instance, core::memory::global_allocator());
	}

	bool process_is_running(Process* process)
	{
		CocoaProcess* instance = static_cast<CocoaProcess*>(process);
		return !instance->has_completed();
	}

} // namespace platform

#pragma GCC diagnostic pop
