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

#include <runtime/core.h>
#include <runtime/logging.h>
#include <runtime/filesystem.h>

#include <assert.h>

// ---------------------------------------------------------------------
// filesystem
// ---------------------------------------------------------------------
void test_filesystem()
{
	TEST_CATEGORY(filesystem);

	core::filesystem::IFileSystem* fs = core::filesystem::instance();
	TEST_VERIFY(fs != nullptr, filesystem_instance_exists);

	//	platform::PathString content_path;
	//	content_path = fs->root_directory();
	//	content_path.append(PATH_SEPARATOR_STRING).append("builds").append(PATH_SEPARATOR_STRING).append(PLATFORM_NAME);
	//	fs->content_directory(content_path);


	//	platform::PathString absolute_path;
	//	TEST_VERIFY(fs->get_absolute_path_for_content(absolute_path, "conf/shaders.conf") == false, get_absolute_path_for_content_missing);
}

// ---------------------------------------------------------------------
// logging
// ---------------------------------------------------------------------
void test_logging()
{
	TEST_CATEGORY(logging);
	TEST_VERIFY(core::logging::instance() != nullptr, log_instance_is_valid);

	LOGV("This is a test of the logging system!\n");

	LOGW("This is a warning\n");
	LOGE("This is an error!\n");

	LOGW("Warning, %i parameters missing!\n", 3);
}



// ---------------------------------------------------------------------
// configloader
// ---------------------------------------------------------------------



// ---------------------------------------------------------------------
// nom
// ---------------------------------------------------------------------
#include <nom/nom.hpp>
void test_nom()
{
	TEST_CATEGORY(nom);
	
	gui::array<int> a;
	
	size_t s = a.size();
	TEST_VERIFY(s == 0, size_empty);
	
	a.push_back(30);
	a.push_back(64);
	a.push_back(128);
	a.push_back(256);
	TEST_VERIFY(a.size() == 4, push_back_four_elements);
	
	
	TEST_VERIFY(a.back() == 256, back);
	
	a.clear();
	TEST_VERIFY(a.size() == 0, clear);
	
	a.resize(4);
	a[0] = 1;
	a[1] = 2;
	a[2] = 3;
	a[3] = 4;
	TEST_VERIFY(a.size() == 4, resize_four);
	

	for (auto& value : a)
	{
		fprintf(stdout, "item: %i\n", value);
	}
	
	a.resize(8);
	TEST_VERIFY(a.size() == 8, resize_eight);
	
	a.clear();

	
	for(size_t index = 0; index < a.size(); ++index)
	{
		fprintf(stdout, "i: %zu -> %i\n", index, a[index]);
	}
}


int main(int, char**)
{
	core::memory::startup();
	platform::Result result = platform::startup();
	assert(result.success());
	
	platform::PathString root_path;
	platform::PathString content_path;
	platform::get_program_directory(&root_path[0], root_path.max_size());
	platform::fs_content_directory(content_path, root_path);
	
	platform::PathString application_path = platform::get_user_application_directory("arcfusion.net/gemini/test_runtime");
	core::startup_filesystem();
	core::filesystem::instance()->root_directory(root_path);
	core::filesystem::instance()->content_directory(content_path);
	core::filesystem::instance()->user_application_directory(application_path);
	
	core::startup_logging();
	
	test_filesystem();
	test_logging();
	
	
	test_nom();
	
	core::shutdown();
	
	platform::shutdown();
	core::memory::shutdown();
	return 0;
}