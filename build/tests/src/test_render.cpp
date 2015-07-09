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

#include <platform/platform.h>
#include <platform/window.h>
#include <platform/kernel.h>

#include <renderer/renderer.h>
#include <renderer/vertexbuffer.h>
#include <renderer/vertexstream.h>

#include <platform/input.h>

#include <assert.h>

using namespace renderer;

// ---------------------------------------------------------------------
// vertexbuffer
// ---------------------------------------------------------------------
void test_vertexbuffer()
{
	TEST_CATEGORY(vertexbuffer);
	
	
	TEST_VERIFY(1, sanity);
}

// ---------------------------------------------------------------------
// vertexstream
// ---------------------------------------------------------------------
void test_vertexstream()
{
	TEST_CATEGORY(vertexstream);
	
	TEST_VERIFY(1, sanity);
	
	struct MyVertex
	{
		float position[3];
	};
	
	VertexStream stream;
	stream.desc.add(VD_FLOAT3);
	
	// reserve room for 6 vertices
	stream.create(6, 0, DRAW_TRIANGLES);
	
	TEST_VERIFY(stream.total_vertices == 6, alloc);
	
	size_t stride = sizeof(float)*3;
	size_t total_bytes = stride * 6;
	TEST_VERIFY(stream.bytes_used() == total_bytes, bytes_used);

	float pos[3] = {127.0f, 256.0f, 2048.0f};
	MyVertex* vertex = (MyVertex*)stream.request(6);
	memcpy(vertex[0].position, pos, sizeof(float)*3);
	memcpy(vertex[1].position, pos, sizeof(float)*3);
	memcpy(vertex[2].position, pos, sizeof(float)*3);
	memcpy(vertex[3].position, pos, sizeof(float)*3);
	memcpy(vertex[4].position, pos, sizeof(float)*3);
	memcpy(vertex[5].position, pos, sizeof(float)*3);
	
	bool data_matches = true;
	for (size_t index = 0; index < 6; ++index)
	{
		for (size_t v = 0; v < 3; ++v)
		{
			data_matches = data_matches && vertex[index].position[v] == pos[v];
			if (!data_matches)
				break;
		}
	}
	TEST_VERIFY(data_matches, data_copy_test);
	
	stream.destroy();
}


// ---------------------------------------------------------------------
// TestKernel
// ---------------------------------------------------------------------
class TestKernel : public kernel::IKernel,
	public kernel::IEventListener<kernel::KeyboardEvent>
{
	bool active;
	platform::window::NativeWindow* native_window;
	
public:
	virtual void event(kernel::KeyboardEvent& event)
	{
		if (event.is_down)
		{
			if (event.key == input::KEY_ESCAPE)
			{
				kernel::instance()->set_active(false);
			}
		}
	}
	
	
public:

	TestKernel()
	{
		native_window = nullptr;
		active = true;
	}

	virtual bool is_active() const { return active; }
	virtual void set_active(bool isactive) { active = isactive; }
	
	virtual kernel::Error startup()
	{
		platform::PathString root_path;
		platform::PathString content_path;
		platform::get_program_directory(&root_path[0], root_path.max_size());
		platform::fs_content_directory(content_path, root_path);
		
		platform::PathString application_path = platform::get_user_application_directory("arcfusion.net/gemini/test_render");
		core::startup_filesystem();
		core::filesystem::instance()->root_directory(root_path);
		core::filesystem::instance()->content_directory(content_path);
		core::filesystem::instance()->user_application_directory(application_path);
		
		core::startup_logging();
		
		
		input::startup();

		platform::window::startup(platform::window::RenderingBackend_Default);

		// create a test window
		platform::window::Parameters params;
		params.window.width = 512;
		params.window.height = 512;
		params.window_title = "test_render";
		
		native_window = platform::window::create(params);
		platform::window::focus(native_window);
		
		// startup the renderer -- just so that it can be used by our classes.
		// It will try to load conf/shaders.conf; but it can be ignored as
		// I don't plan on actually rendering anything yet.
		renderer::RenderSettings render_settings;
		render_settings.gamma_correct = false;
		renderer::startup(renderer::OpenGL, render_settings);
	
	
		// run tests...
		test_vertexbuffer();
		test_vertexstream();
	
	
		return kernel::NoError;
	}
	
	virtual void tick()
	{
		input::update();
		platform::window::dispatch_events();
		set_active(false);
	}


	virtual void shutdown()
	{
		renderer::shutdown();

		platform::window::destroy(native_window);
		platform::window::shutdown();

		input::shutdown();
		
		core::shutdown();
	}
};



int main(int, char**)
{
	platform::MainParameters mainparameters;
	platform::set_mainparameters(mainparameters);
	return platform::run_application(new TestKernel());
}