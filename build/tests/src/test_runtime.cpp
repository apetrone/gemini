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

#include <runtime/runtime.h>
#include <runtime/filesystem.h>
#include <runtime/jobqueue.h>
#include <runtime/standaloneresourcecache.h>

#include <platform/platform.h>

#include <core/core.h>
#include <core/logging.h>
#include <core/mathlib.h>
#include <core/typespec.h>

#include <ui/panel.h>
#include <ui/compositor.h>

#include <assert.h>

#if 0
// ---------------------------------------------------------------------
// jobqueue
// ---------------------------------------------------------------------

void print_string(const char* data)
{
	LOGV("thread: 0x%x, string: %s\n", (size_t)platform::thread_id(), data);
}

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
// ---------------------------------------------------------------------
// user interface
// ---------------------------------------------------------------------
class DummyRenderer : public gui::Renderer
{
public:
	virtual void increment_depth() {}

	virtual void startup(gui::Compositor*) {}
	virtual void shutdown(gui::Compositor*) {}

	virtual void begin_frame(gui::Compositor*) {}
	virtual void end_frame() {}

	virtual gui::TextureResult texture_create(const char*, gui::TextureHandle&)
	{
		return gui::TextureResult_Success;
	}

	virtual void texture_destroy(const gui::TextureHandle&)
	{
	}

	virtual gui::TextureResult texture_info(const gui::TextureHandle&, uint32_t&, uint32_t&, uint8_t&)
	{
		return gui::TextureResult_Success;
	}

	virtual gui::FontResult font_create(const char*, gui::FontHandle&)
	{
		return gui::FontResult_Success;
	}

	virtual void font_destroy(const gui::FontHandle&)
	{
	}

	virtual gui::FontResult font_measure_string(const gui::FontHandle&, const char*, size_t, gui::Rect&)
	{
		return gui::FontResult_Success;
	}

	virtual void font_metrics(const gui::FontHandle&, size_t&, int&, int&)
	{
	}

	virtual size_t font_draw(const gui::FontHandle&, const char*, size_t, const gui::Rect&, const gemini::Color&, gui::render::Vertex*, size_t)
	{
		return 0;
	}

	virtual size_t font_count_vertices(const gui::FontHandle&, size_t)
	{
		return 0;
	}

	virtual void draw_commands(gui::render::CommandList*, Array<gui::render::Vertex>&)
	{
	}
}; // DummyRenderer


class TestPanel : public gui::Panel
{
public:
	TestPanel(gui::Panel* parent)
		: gui::Panel(parent)
	{
	}

	template <class Collector>
	void typespec_info(Collector& collector, uint32_t /*version*/)
	{
		collector += TYPESPEC_PROPERTY(flags);
		collector += TYPESPEC_PROPERTY(origin);

//		collector += TYPESPEC_PROPERTY(large_value);
//		collector += TYPESPEC_PROPERTY(my_number);
		//collector += TYPESPEC_PROPERTY(c_string);

//		if (version > 0)
//		{
//			collector += TYPESPEC_PROPERTY(flags);
//		}
	}
}; // TestPanel


class MyC : public gemini::Collector<MyC>
{
private:
	Array<uint8_t> buffer;
	size_t index;
public:

	MyC()
	{
		buffer.resize(1024 * 1024);
		index = 0;
	}

	template <class T>
	void write_pod(T* value)
	{
		memcpy(&buffer[index], value, sizeof(T));
		index += sizeof(T);
	}

	template <class T>
	void write(T* value)
	{
		write_pod(value);
	}

	template <class T>
	void read_pod(T* value)
	{
		memcpy(value, &buffer[index], sizeof(T));
		index += sizeof(T);
	}

	template <class T>
	void read(T* value)
	{
		read_pod(value);
	}

	void reset()
	{
		index = 0;
	}
};


// try to expose math types.
namespace gemini
{
	template <class Collector>
	void typespec_info(Collector& c, uint32_t version, glm::vec3* instance)
	{
		c += instance->x;
		c += instance->y;
		c += instance->z;
	}
}




UNITTEST(user_interface)
{







	DummyRenderer renderer;
	renderer::StandaloneResourceCache resource_cache;
	gui::Compositor* compositor = new gui::Compositor(128, 128, &resource_cache, &renderer);
	compositor->set_name("compositor");

	TestPanel* test = new TestPanel(compositor);


	MyC collector;
	collector << test;


	delete compositor;
}

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
