// -------------------------------------------------------------
// Copyright (C) 2015- Adam Petrone
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

#if TARGET_OS_MAC && !TARGET_OS_IPHONE
	#import <Cocoa/Cocoa.h>
	#import <AppKit/AppKit.h>
#else
	#import <Foundation/Foundation.h>
#endif

#include "cocoa_common.h"

namespace platform
{
	PathString get_program_directory()
	{
		Result result;
		NSString* bundle_path = [[NSBundle mainBundle] bundlePath];
		assert(bundle_path);
		return cocoa::nsstring_to_stackstring<PathString>(bundle_path);
	}

	Result make_directory(const char* path)
	{
		return posix_make_directory(path);
	}

	Result remove_directory(const char* path)
	{
		return posix_remove_directory(path);
	}

	const char* get_environment_variable(const char* name)
	{
		return posix_get_environment_variable(name);
	}

	PathString get_user_directory()
	{
		return posix_get_user_directory();
	}

	PathString get_user_application_directory(const char* application_data_path)
	{
		NSArray* directories = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
		NSString* application_support_directory = [directories lastObject];

		NSString* application_path = [NSString stringWithUTF8String:application_data_path];
		application_support_directory = [application_support_directory stringByAppendingPathComponent: application_path];
		return cocoa::nsstring_to_stackstring<PathString>(application_support_directory);
	}

	PathString get_user_temp_directory()
	{
		NSArray* directories = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
		NSString* user_temp_directory = [directories lastObject];
		return cocoa::nsstring_to_stackstring<PathString>(user_temp_directory);
	}

	PathString make_absolute_path(const char* path)
	{
		return posix_make_absolute_path(path);
	}

	platform::File fs_open(const char* path, FileMode mode)
	{
		return posix_fs_open(path, mode);
	}

	void fs_close(platform::File file)
	{
		return posix_fs_close(file);
	}

	size_t fs_read(platform::File file, void* destination, size_t size, size_t count)
	{
		return posix_fs_read(file, destination, size, count);
	}

	size_t fs_write(platform::File file, const void* source, size_t size, size_t count)
	{
		return posix_fs_write(file, source, size, count);
	}

	int32_t fs_seek(platform::File file, long int offset, FileSeek origin)
	{
		return posix_fs_seek(file, offset, origin);
	}

	long int fs_tell(platform::File file)
	{
		return posix_fs_tell(file);
	}

	bool fs_file_exists(const char* path)
	{
		return posix_fs_file_exists(path);
	}

	bool fs_directory_exists(const char* path)
	{
		return posix_fs_directory_exists(path);
	}

	PathString fs_content_directory()
	{
		// On Mac/iOS, the root directory points to the app bundle
		PathString content_path = get_program_directory();
		content_path.append(PATH_SEPARATOR_STRING);
		content_path.append("Contents");
		content_path.append(PATH_SEPARATOR_STRING);
		content_path.append("Resources");
		return content_path;
	}
} // namespace platform
