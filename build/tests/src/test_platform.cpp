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
#include "unit_test.h"

#include <platform/platform.h>

using namespace platform;

// ---------------------------------------------------------------------
// platform
// ---------------------------------------------------------------------
UNITTEST(platform)
{
	platform::Result result = platform::startup();
	TEST_ASSERT(result.succeeded(), platform_startup);

	PLATFORM_LOG(platform::LogMessageType::Info, "PLATFORM_NAME: %s\n", PLATFORM_NAME);
	PLATFORM_LOG(platform::LogMessageType::Info, "PLATFORM_COMPILER: %s, version: %s\n", PLATFORM_COMPILER, PLATFORM_COMPILER_VERSION);
}

// ---------------------------------------------------------------------
// dynamic library
// ---------------------------------------------------------------------
UNITTEST(dynamic_library)
{
	const char* library_extension = platform::dylib_extension();
	TEST_ASSERT(library_extension, dylib_extension);
}

// ---------------------------------------------------------------------
// filesystem
// ---------------------------------------------------------------------
UNITTEST(filesystem)
{
	platform::PathString filename = platform::get_program_directory();

	// see if we can verify directories exist; otherwise, we cannot verify
	// other functions
	PLATFORM_LOG(LogMessageType::Info, "checking program directory exists: '%s'\n", filename());
	TEST_ASSERT(platform::fs_directory_exists(filename()), fs_directory_exists);

	filename.append(PATH_SEPARATOR_STRING);
	filename.append("test.conf");

	platform::File file = platform::fs_open(filename(), platform::FileMode_Write);
	TEST_ASSERT(file.handle != 0, fs_open_for_write);
	TEST_ASSERT(file.is_open(), file_is_open);

	size_t bytes_written = 0;
	if (file.is_open())
	{
		const char buffer[] = "Hello, this is a test\n";
		bytes_written = platform::fs_write(file, buffer, 22, 1);
		platform::fs_close(file);
	}
	TEST_ASSERT(bytes_written == 22, fs_write);

	file = platform::fs_open(filename(), platform::FileMode_Read);
	TEST_ASSERT(file.handle != 0, fs_open_for_read);
	size_t bytes_read = 0;
	if (file.is_open())
	{
		core::StackString<32> buffer;
		bytes_read = platform::fs_read(file, &buffer[0], 22, 1);
		platform::fs_close(file);
	}
	TEST_ASSERT(bytes_read == 22, fs_read);


	platform::PathString content_directory = platform::fs_content_directory();
	TEST_ASSERT(!content_directory.is_empty(), fs_content_directory);

	// test directories
	platform::Result result;
	platform::PathString program_directory = platform::get_program_directory();
	TEST_ASSERT(!program_directory.is_empty(), get_program_directory);

	// this currently fails on subsequent runs because the directory
	// is never removed.
	result = platform::make_directory("test_directory");
	TEST_ASSERT(result.succeeded(), make_directory);

	const char* user_home = nullptr;
#if defined(PLATFORM_WINDOWS)
	user_home = platform::get_environment_variable("%USERPROFILE%");
#elif defined(PLATFORM_APPLE) || defined(PLATFORM_LINUX)
	user_home = platform::get_environment_variable("HOME");
#else
	#error I do not know how to test this platform.
#endif

	TEST_ASSERT(user_home != nullptr, get_environment_variable);
//	fprintf(stdout, "get_environment_variable [%%USERPROFILE%% / $HOME]: '%s'\n", user_home);


	platform::PathString user_directory = platform::get_user_directory();
	TEST_ASSERT(!user_directory.is_empty(), get_user_directory);
//	fprintf(stdout, "get_user_directory: '%s'\n", user_directory);


	platform::PathString temp_directory = platform::get_user_temp_directory();
	TEST_ASSERT(!temp_directory.is_empty(), get_user_temp_directory);
}

// ---------------------------------------------------------------------
// logging
// ---------------------------------------------------------------------
UNITTEST(logging)
{
	// This isn't something I know how to reliably test without installing
	// some mock platform handler.

	platform::log_message(platform::LogMessageType::Info, "test info log message\n");
	platform::log_message(platform::LogMessageType::Warning, "test warning log message\n");
	platform::log_message(platform::LogMessageType::Error, "test error log message\n");

	TEST_ASSERT(true, logging_sanity);
}


// ---------------------------------------------------------------------
// serial
// ---------------------------------------------------------------------
UNITTEST(serial)
{

}

// ---------------------------------------------------------------------
// system
// ---------------------------------------------------------------------
UNITTEST(system)
{
	size_t page_size = platform::system_pagesize_bytes();

	PLATFORM_LOG(platform::LogMessageType::Info, "page size: %i bytes\n", page_size);
	TEST_ASSERT(page_size > 0, page_size);

	size_t total_processors = platform::system_processor_count();
	PLATFORM_LOG(platform::LogMessageType::Info, "total processors: %i\n", total_processors);
	TEST_ASSERT(total_processors >= 1, system_processor_count);

	size_t uptime_seconds = platform::system_uptime_seconds();
	PLATFORM_LOG(platform::LogMessageType::Info, "system_uptime_seconds: %i\n", uptime_seconds);
	TEST_ASSERT(uptime_seconds > 0, system_uptime_seconds);

	core::StackString<64> version = platform::system_version_string();
	PLATFORM_LOG(platform::LogMessageType::Info, "system_version_string: %s\n", version());
	TEST_ASSERT(!version.is_empty(), system_version_string);
}

// ---------------------------------------------------------------------
// thread
// ---------------------------------------------------------------------
void test_thread(void* data)
{
	platform::ThreadId thread_id = platform::thread_id();
	PLATFORM_LOG(platform::LogMessageType::Info, "test_thread enter: %i\n", thread_id);

	PLATFORM_LOG(platform::LogMessageType::Info, "test_thread exit\n");
}

UNITTEST(thread)
{
	platform::Thread thread;
	platform::Result result = platform::thread_create(thread, test_thread, nullptr);
	TEST_ASSERT(result.succeeded(), thread_create);

	platform::thread_join(thread);
}

// ---------------------------------------------------------------------
// time
// ---------------------------------------------------------------------
UNITTEST(datetime)
{
	platform::DateTime dt;
	platform::datetime(dt);

	bool maybe_valid = \
		(dt.day != 0 ||
		dt.dayOfWeek != 0 ||
		dt.hour != 0 ||
		dt.milliseconds != 0 ||
		dt.minute != 0 ||
		dt.month != 0 ||
		dt.second != 0) &&
	dt.year != 0;

	// this isn't something I know how to reliably test. here goes nothing.
	TEST_ASSERT(maybe_valid, datetime_sanity);

	uint64_t us = platform::microseconds();
	TEST_ASSERT(us != 0, microseconds);

	PLATFORM_LOG(platform::LogMessageType::Info, "waiting three seconds...\n");

	uint64_t last = us;
	while((last - us) < (3 * MicrosecondsPerSecond))
	{
		last = platform::microseconds();
	}

	PLATFORM_LOG(platform::LogMessageType::Info, "three seconds have passed!\n");
}



int main(int, char**)
{
	unittest::UnitTest::execute();

	// the matching 'startup' to this is in the platform unit test.
	platform::shutdown();

	return 0;
}