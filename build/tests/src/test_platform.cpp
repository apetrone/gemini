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
#include <platform/window.h>

#if defined(PLATFORM_RASPBERRYPI)
	#include <GLES2/gl2.h>
#endif

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

	core::StackString<MAX_PATH_SIZE> filename;
	platform::get_program_directory(&filename[0], filename.max_size());
	
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
}

// ---------------------------------------------------------------------
// serial
// ---------------------------------------------------------------------

// ---------------------------------------------------------------------
// thread
// ---------------------------------------------------------------------

// ---------------------------------------------------------------------
// time
// ---------------------------------------------------------------------

// ---------------------------------------------------------------------
// window
// ---------------------------------------------------------------------
void test_window()
{
	platform::window::Parameters params;
	params.window_title = "none";
	params.depth_size = 16;


	fprintf(stdout, "-> creating window...\n");
	platform::window::NativeWindow* window = platform::window::create(params);

#if defined(PLATFORM_RASPBERRYPI)
	fprintf(stdout, "GL_VENDOR: %s\n", glGetString(GL_VENDOR));
	fprintf(stdout, "GL_RENDERER: %s\n", glGetString(GL_RENDERER));
#endif

	fprintf(stdout, "-> render to window and wait 1000 ms...\n");
	// while(true)
	{
		platform::window::begin_rendering(window);
		platform::window::dispatch_events();
		
#if defined(PLATFORM_RASPBERRYPI)
		glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif

		platform::window::end_rendering(window);

		platform::thread_sleep(1000);
	}
	

	fprintf(stdout, "-> destroying window...\n");
	platform::window::destroy(window);
}

int main(int, char**)
{
	TEST_CATEGORY(platform);

	platform::Result result = platform::startup();
	TEST_VERIFY(result.success(), platform_startup);

	test_dynamic_library();
	test_filesystem();
	
	result = platform::window::startup(platform::window::RenderingBackend_Default);
	TEST_VERIFY(result.success(), platform_window_startup);
	
	test_window();
	platform::window::shutdown();
	
	platform::shutdown();
	return 0;
}