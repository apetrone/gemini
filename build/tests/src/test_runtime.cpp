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

#include <core/core.h>
#include <core/logging.h>

#include <platform/platform.h>

#include <runtime/runtime.h>
#include <runtime/filesystem.h>
#include <runtime/jobqueue.h>

#include <runtime/http.h>

#include <assert.h>

#include <core/mathlib.h>

// ---------------------------------------------------------------------
// jobqueue
// ---------------------------------------------------------------------

void print_string(const char* data)
{
	LOGV("thread: 0x%x, string: %s\n", (size_t)platform::thread_id(), data);
}

#if 0
UNITTEST(jobqueue)
{
	const size_t MAX_JOB_ITERATIONS = 6;
	const char* iterations[] = {
		"ALPHA: 0",
		"ALPHA: 1",
		"ALPHA: 2",
		"ALPHA: 3",
		"ALPHA: 4",
		"ALPHA: 5",
		"ALPHA: 6",
		"ALPHA: 7",
		"ALPHA: 8",
		"ALPHA: 9",
		"BETA: 0",
		"BETA: 1",
		"BETA: 2",
		"BETA: 3",
		"BETA: 4",
		"BETA: 5",
		"BETA: 6",
		"BETA: 7",
		"BETA: 8",
		"BETA: 9",
		"DELTA: 0",
		"DELTA: 1",
		"DELTA: 2",
		"DELTA: 3",
		"DELTA: 4",
		"DELTA: 5",
		"DELTA: 6",
		"DELTA: 7",
		"DELTA: 8",
		"DELTA: 9",
		"EPSILON: 0",
		"EPSILON: 1",
		"EPSILON: 2",
		"EPSILON: 3",
		"EPSILON: 4",
		"EPSILON: 5",
		"EPSILON: 6",
		"EPSILON: 7",
		"EPSILON: 8",
		"EPSILON: 9",
		"FOXTROT: 0",
		"FOXTROT: 1",
		"FOXTROT: 2",
		"FOXTROT: 3",
		"FOXTROT: 4",
		"FOXTROT: 5",
		"FOXTROT: 6",
		"FOXTROT: 7",
		"FOXTROT: 8",
		"FOXTROT: 9",
		"GAMMA: 0",
		"GAMMA: 1",
		"GAMMA: 2",
		"GAMMA: 3",
		"GAMMA: 4",
		"GAMMA: 5",
		"GAMMA: 6",
		"GAMMA: 7",
		"GAMMA: 8",
		"GAMMA: 9"
	};

	gemini::JobQueue jq;
	jq.create_workers(3);

	for (size_t index = 0; index < MAX_JOB_ITERATIONS; ++index)
	{
		jq.push_back(print_string, iterations[(index * 10) + 0]);
		jq.push_back(print_string, iterations[(index * 10) + 1]);
		jq.push_back(print_string, iterations[(index * 10) + 2]);
		jq.push_back(print_string, iterations[(index * 10) + 3]);
		jq.push_back(print_string, iterations[(index * 10) + 4]);
		jq.push_back(print_string, iterations[(index * 10) + 5]);
		jq.push_back(print_string, iterations[(index * 10) + 6]);
		jq.push_back(print_string, iterations[(index * 10) + 7]);
		jq.push_back(print_string, iterations[(index * 10) + 8]);
		jq.push_back(print_string, iterations[(index * 10) + 9]);

		jq.wait_for_jobs_to_complete();

		platform::thread_sleep(250);
	}

	LOGV("destroying workers...\n");
	jq.destroy_workers();
}
#endif

#if 0
// ---------------------------------------------------------------------
// filesystem
// ---------------------------------------------------------------------
UNITTEST(filesystem)
{
	core::filesystem::IFileSystem* fs = core::filesystem::instance();
	TEST_ASSERT(fs != nullptr, filesystem_instance_exists);

	//	platform::PathString content_path;
	//	content_path = fs->root_directory();
	//	content_path.append(PATH_SEPARATOR_STRING).append("builds").append(PATH_SEPARATOR_STRING).append(PLATFORM_NAME);
	//	fs->content_directory(content_path);


	//	platform::PathString absolute_path;
	//	TEST_ASSERT(fs->get_absolute_path_for_content(absolute_path, "conf/shaders.conf") == false, get_absolute_path_for_content_missing);
}
#endif

// ---------------------------------------------------------------------
// http
// ---------------------------------------------------------------------
UNITTEST(http)
{
	platform::net_startup();

	gemini::http_startup();

	gemini::http_request_file("http://apetrone-win7:8000/essential_cyberpunk.jpg", "./downloads/download.jpg", "Test");

	while (gemini::http_active_download_count() > 0)
	{
		gemini::http_update();
	}

	gemini::http_shutdown();

	platform::net_shutdown();
}


#if 0
// ---------------------------------------------------------------------
// logging
// ---------------------------------------------------------------------
UNITTEST(logging)
{
	TEST_ASSERT(core::logging::instance() != nullptr, log_instance_is_valid);

	LOGV("This is a test of the logging system!\n");

	LOGW("This is a warning\n");
	LOGE("This is an error!\n");

	LOGW("Warning, %i parameters missing!\n", 3);
}
#endif

int main(int, char**)
{
	gemini::core_startup();
	gemini::runtime_startup("arcfusion.net/gemini/test_runtime");

	using namespace gemini;

	unittest::UnitTest::execute();

	gemini::runtime_shutdown();
	gemini::core_shutdown();
	return 0;
}
