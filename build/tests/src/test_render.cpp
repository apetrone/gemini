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
#include <platform/input.h>

#include <renderer/renderer.h>
#include <renderer/vertexbuffer.h>
#include <renderer/vertexstream.h>

#include <assert.h>

using namespace renderer;

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
		platform::PathString root_path = platform::get_program_directory();
		platform::PathString content_path = platform::fs_content_directory();
		platform::PathString user_application_path = platform::get_user_application_directory("arcfusion.net/gemini/test_render");
		core::startup_filesystem();
		core::filesystem::instance()->root_directory(root_path);
		core::filesystem::instance()->content_directory(content_path);
		core::filesystem::instance()->user_application_directory(user_application_path);
		
		core::startup_logging();
		
		LOGV("root_path: %s\n", root_path());
		LOGV("content_path: %s\n", content_path());
		LOGV("user_application_path: %s\n", user_application_path());
		
		
		input::startup();

		platform::window::startup(platform::window::RenderingBackend_Default);



		size_t total_displays = platform::window::screen_count();
		PLATFORM_LOG(platform::LogMessageType::Info, "-> total screens: %lu\n", total_displays);
		
		for (size_t i = 0; i < total_displays; ++i)
		{
			platform::window::Frame frame = platform::window::screen_frame(i);
			PLATFORM_LOG(platform::LogMessageType::Info, "display %lu rect = %u, %u, %u x %u\n", (unsigned long)i, frame.x, frame.y, frame.width, frame.height);
		}



		// create a test window
		platform::window::Parameters params;
		params.window.width = 512;
		params.window.height = 512;
		params.window_title = "test_render";
		
		native_window = platform::window::create(params);
		assert(native_window != nullptr);
		
		PLATFORM_LOG(platform::LogMessageType::Info, "window dimensions: %i %i\n", native_window->dimensions.width, native_window->dimensions.height);
		
		platform::window::focus(native_window);

		// initialize render device
		render2::RenderParameters render_parameters;
		render_parameters["rendering_backend"] = "default";
		render_parameters["gamma_correct"] = "true";
		
		device = render2::create_device(render_parameters);
		assert(device != nullptr);
		
		
		// setup the pipeline
		render2::PipelineDescriptor desc;
		desc.shader = device->create_shader("test");
		desc.vertex_description.add("in_position", render2::VD_FLOAT, 3); // position
		desc.vertex_description.add("in_color", render2::VD_FLOAT, 4); // color
		desc.input_layout = device->create_input_layout(desc.vertex_description, desc.shader);
		pipeline = device->create_pipeline(desc);
		
		// create a vertex buffer and populate it with data
		float width = (float)native_window->dimensions.width;
		float height = (float)native_window->dimensions.height;
		
		// Draw a triangle on screen with the wide part of the base at the bottom
		// of the screen.
		size_t total_bytes = sizeof(MyVertex) * 4;
		vertex_buffer = device->create_vertex_buffer(total_bytes);
		assert(vertex_buffer != nullptr);
		MyVertex vertices[4];
		vertices[0].set_position(0, height, 0);
		vertices[0].set_color(1.0f, 0.0f, 0.0f, 1.0f);
		
		vertices[1].set_position(width, height, 0);
		vertices[1].set_color(0.0f, 1.0f, 0.0f, 1.0f);
		
		vertices[2].set_position(width/2, 0, 0);
		vertices[2].set_color(0.0f, 0.0f, 1.0f, 1.0f);
		
		vertices[3].set_position(0, 0, 0);
		vertices[3].set_color(0.0f, 1.0f, 1.0f, 1.0f);
		
		device->buffer_upload(vertex_buffer, vertices, total_bytes);


		// setup constant buffer
		// this needs to be much more flexible, but for testng it works.
		ConstantData cd;
		cd.modelview_matrix = glm::mat4(1.0f);
		cd.projection_matrix = glm::ortho(0.0f, width, height, 0.0f, -1.0f, 1.0f);
		pipeline->constants()->assign(&cd, sizeof(ConstantData));
	
		return kernel::NoError;
	}
	
	virtual void tick()
	{
		// update our input
		input::update();

		// dispatch all window events
		platform::window::dispatch_events();
	
		// sanity check
		assert(device);
		assert(pipeline);
		assert(vertex_buffer);
		
		render2::Pass render_pass;
		render_pass.target = device->default_render_target();
		render_pass.color(0.0f, 0.0f, 0.0f, 1.0f);
		
		// create a command queue
		render2::CommandQueue queue(&render_pass);
		
		// create a command serializer for the queue
		render2::CommandSerializer* serializer = device->create_serializer(queue);
		assert(serializer);
		
		// add commands to the queue
		serializer->pipeline(pipeline);
		serializer->viewport(0, 0, native_window->dimensions.width, native_window->dimensions.height);
		serializer->vertex_buffer(vertex_buffer);
//		serializer->draw_indexed_primitives(index_buffer, 3);
		serializer->draw(0, 3);
		device->destroy_serializer(serializer);
		
		// queue the buffer with our device
		device->queue_buffers(&queue, 1);

		// submit the queues to the GPU
		platform::window::begin_rendering(native_window);
		device->submit();
		platform::window::end_rendering(native_window);
	}


	virtual void shutdown()
	{
		device->destroy_buffer(vertex_buffer);
		device->destroy_pipeline(pipeline);
		render2::destroy_device(device);
	
		renderer::shutdown();

		platform::window::destroy(native_window);
		platform::window::shutdown();

		input::shutdown();
		
		core::shutdown();
	}
	
	
private:
	struct MyVertex
	{
		float position[3];
		float color[4];
		
		void set_position(float x, float y, float z)
		{
			position[0] = x;
			position[1] = y;
			position[2] = z;
		}
		
		void set_color(float red, float green, float blue, float alpha)
		{
			color[0] = red;
			color[1] = green;
			color[2] = blue;
			color[3] = alpha;
		}
	};
	
	struct ConstantData
	{
		glm::mat4 modelview_matrix;
		glm::mat4 projection_matrix;
	};
	
	render2::Device* device;
	render2::Pipeline* pipeline;
	render2::Buffer* vertex_buffer;
};

PLATFORM_MAIN
{
	PLATFORM_IMPLEMENT_PARAMETERS();
	
	PLATFORM_RETURN(platform::run_application(new TestKernel()));
}