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
// logging
// ---------------------------------------------------------------------

// ---------------------------------------------------------------------
// filesystem
// ---------------------------------------------------------------------
void test_filesystem()
{
	TEST_CATEGORY(filesystem);
	
	core::filesystem::IFileSystem* fs = core::fs::instance();
	TEST_VERIFY(fs != nullptr, filesystem_instance_exists);
	
//	platform::PathString content_path;
//	content_path = fs->root_directory();
//	content_path.append(PATH_SEPARATOR_STRING).append("builds").append(PATH_SEPARATOR_STRING).append(PLATFORM_NAME);
//	fs->content_directory(content_path);
	
	
//	platform::PathString absolute_path;
//	TEST_VERIFY(fs->get_absolute_path_for_content(absolute_path, "conf/shaders.conf") == false, get_absolute_path_for_content_missing);
}

// ---------------------------------------------------------------------
// configloader
// ---------------------------------------------------------------------

int main(int, char**)
{
	platform::memory::startup();
	platform::Result result = platform::startup();
	assert(result.success());
	
	platform::PathString root_path;
	platform::PathString content_path;
	platform::get_program_directory(&root_path[0], root_path.max_size());
	platform::fs_content_directory(content_path, root_path);
	
	platform::PathString application_path = platform::get_user_application_directory();
	application_path.append(PATH_SEPARATOR_STRING);
	application_path.append("net.arcfusion.test_runtime");
	
	core::startup(root_path, content_path, application_path);
	
	test_filesystem();
	
	core::shutdown();
	
	platform::shutdown();
	platform::memory::shutdown();
	return 0;
}