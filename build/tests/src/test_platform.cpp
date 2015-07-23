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

// ---------------------------------------------------------------------
// dynamic library
// ---------------------------------------------------------------------
void test_dynamic_library()
{
	TEST_CATEGORY(dynamic_library);

	const char* library_extension = platform::dylib_extension();
	TEST_VERIFY(library_extension, dylib_extension);
}

// ---------------------------------------------------------------------
// filesystem
// ---------------------------------------------------------------------
void test_filesystem()
{
	TEST_CATEGORY(filesystem);

	platform::PathString filename = platform::get_program_directory();

	// see if we can verify directories exist; otherwise, we cannot verify
	// other functions
	TEST_VERIFY(platform::fs_directory_exists(filename()), fs_directory_exists);

	filename.append(PATH_SEPARATOR_STRING);
	filename.append("test.conf");

	platform::File file = platform::fs_open(filename(), platform::FileMode_Write);
	TEST_VERIFY(file.handle != 0, fs_open_for_write);
	TEST_VERIFY(file.is_open(), file_is_open);

	if (file.is_open())
	{
		const char buffer[] = "Hello, this is a test\n";
		size_t bytes_written = platform::fs_write(file, buffer, 22, 1);
		TEST_VERIFY(bytes_written == 22, fs_write);
		platform::fs_close(file);
	}

	file = platform::fs_open(filename(), platform::FileMode_Read);
	TEST_VERIFY(file.handle != 0, fs_open_for_read);
	if (file.is_open())
	{
		core::StackString<32> buffer;
		size_t bytes_read = platform::fs_read(file, &buffer[0], 22, 1);
		TEST_VERIFY(bytes_read == 22, fs_read);
		platform::fs_close(file);
	}


	// test directories
	{
		platform::Result result;
		platform::PathString program_directory = platform::get_program_directory();
		TEST_VERIFY(!program_directory.is_empty(), get_program_directory);

		result = platform::make_directory("test_directory");
		TEST_VERIFY(result.succeeded(), make_directory);

		const char* user_home = nullptr;
#if defined(PLATFORM_WINDOWS)
		user_home = platform::get_environment_variable("%USERPROFILE%");
#elif defined(PLATFORM_APPLE) || defined(PLATFORM_LINUX)
		user_home = platform::get_environment_variable("HOME");
#else
	#error I do not know how to test this platform.
#endif

		TEST_VERIFY(user_home != nullptr, get_environment_variable);
//		fprintf(stdout, "get_environment_variable [%%USERPROFILE%% / $HOME]: '%s'\n", user_home);


		platform::PathString user_directory = platform::get_user_directory();
		TEST_VERIFY(!user_directory.is_empty(), get_user_directory);
//		fprintf(stdout, "get_user_directory: '%s'\n", user_directory);


		platform::PathString temp_directory = platform::get_user_temp_directory();
		TEST_VERIFY(!temp_directory.is_empty(), get_user_temp_directory);
	}

}

// ---------------------------------------------------------------------
// logging
// ---------------------------------------------------------------------
void test_logging()
{
	TEST_CATEGORY(logging);

	// This isn't something I know how to reliably test without installing
	// some mock platform handler.

	platform::log_message(platform::LogMessageType::Info, "test info log message\n");
	platform::log_message(platform::LogMessageType::Warning, "test warning log message\n");
	platform::log_message(platform::LogMessageType::Error, "test error log message\n");

	TEST_VERIFY(true, logging_sanity);
}


// ---------------------------------------------------------------------
// serial
// ---------------------------------------------------------------------
void test_serial()
{

}

// ---------------------------------------------------------------------
// thread
// ---------------------------------------------------------------------

// ---------------------------------------------------------------------
// time
// ---------------------------------------------------------------------
void test_datetime()
{
	TEST_CATEGORY(datetime);

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
	TEST_VERIFY(maybe_valid, datetime_sanity);

	uint64_t ms = platform::microseconds();
	TEST_VERIFY(ms != 0, microseconds);
}

int main(int, char**)
{
	TEST_CATEGORY(platform);

	platform::Result result = platform::startup();
	TEST_VERIFY(result.succeeded(), platform_startup);

	test_dynamic_library();
	test_filesystem();
	test_logging();
	test_serial();
	test_datetime();

	platform::shutdown();
	return 0;
}