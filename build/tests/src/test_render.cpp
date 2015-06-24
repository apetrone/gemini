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
#include <platform/windowlibrary.h>
#include <platform/kernel.h>

#include <renderer/renderer.h>
#include <renderer/vertexbuffer.h>
#include <renderer/vertexstream.h>

#include <platform/input.h>

#include <assert.h>

// ---------------------------------------------------------------------
// vertexbuffer
// ---------------------------------------------------------------------


// ---------------------------------------------------------------------
// vertexstream
// ---------------------------------------------------------------------



// ---------------------------------------------------------------------
// TestKernel
// ---------------------------------------------------------------------
class TestKernel : public kernel::IKernel,
	public kernel::IEventListener<kernel::KeyboardEvent>
{
	bool active;
	platform::NativeWindow* native_window;
	platform::IWindowLibrary* window_library;
	
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
		window_library = nullptr;
		active = true;
	}

	virtual bool is_active() const { return active; }
	virtual void set_active(bool isactive) { active = isactive; }
	
	virtual void resolution_changed(int width, int height)
	{
	}
	
	virtual kernel::Error startup()
	{
		platform::memory::startup();
		platform::Result result = platform::startup();
		assert(result.success());
		
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
		
		// create a test window
		window_library = platform::create_window_library();
		window_library->startup(kernel::parameters());
		
		platform::WindowParameters params;
		params.window_width = 512;
		params.window_height = 512;
		params.window_title = "test_render";
		native_window = window_library->create_window(params);
	
		window_library->activate_window(native_window);
		window_library->focus_window(native_window);
		window_library->show_mouse(true);
	
		return kernel::NoError;
	}
	
	virtual void tick()
	{
		input::update();
		window_library->process_events();
	}


	virtual void shutdown()
	{
		window_library->destroy_window(native_window);
		window_library->shutdown();
		platform::destroy_window_library();
		
		input::shutdown();
		
		core::shutdown();
		platform::shutdown();
		platform::memory::shutdown();
	}
};



int main(int, char**)
{
	platform::MainParameters mainparameters;
	platform::set_mainparameters(mainparameters);
	return platform::run_application(new TestKernel());
}