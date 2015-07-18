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

#include <renderer/debug_draw.h>
#include <renderer/renderer.h>

#include <runtime/core.h>
#include <runtime/logging.h>

#include <platform/platform.h>
#include <platform/window.h>
#include <platform/kernel.h>
#include <platform/input.h>

#include <core/hashset.h>
#include <core/stackstring.h>
#include <core/fixedarray.h>

#include <runtime/filesystem.h>

#include <nom/nom.hpp>
#include <nom/compositor.hpp>
#include <nom/graph.hpp>
#include <nom/button.hpp>

// when defined; uses the old method for creating windows
//#define USE_WINDOW_LIBRARY

#if defined(PLATFORM_SDL2_SUPPORT)
	#include <platform/windowlibrary.h>
#endif


using namespace platform;
using namespace renderer;

namespace render2
{
	template <class O, class I>
	O convert(const I& input)
	{
		O a;
		return a;
	}
	
	template <>
	int convert(const param_string& s)
	{
		return atoi(s());
	}
}

class EditorKernel : public kernel::IKernel,
public kernel::IEventListener<kernel::KeyboardEvent>,
public kernel::IEventListener<kernel::MouseEvent>,
public kernel::IEventListener<kernel::SystemEvent>
{
private:
	bool active;
#if defined(USE_WINDOW_LIBRARY)
	platform::IWindowLibrary* window_interface;
#endif
	platform::window::NativeWindow* main_window;
	
	render2::Device* device;
	render2::Buffer* triangle_stream;
	render2::Buffer* index_buffer;
	render2::Pipeline* pipeline;
	
//	GLsync fence;
	
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
	
public:
	EditorKernel() :
		active(true)
#if defined(USE_WINDOW_LIBRARY)
		, window_interface(0)
#endif
	{
	}
	
	virtual ~EditorKernel() {}
	
	virtual bool is_active() const { return active; }
	virtual void set_active(bool isactive) { active = isactive; }
	
	virtual void event(kernel::SystemEvent& event)
	{
		if (event.subtype == kernel::WindowGainFocus)
		{
			LOGV("window gained focus\n");
		}
		else if (event.subtype == kernel::WindowLostFocus)
		{
			LOGV("window lost focus\n");
		}
		else if (event.subtype == kernel::WindowResized)
		{
			LOGV("resolution_changed %i %i\n", event.render_width, event.render_height);
		}
	}
	
	virtual void event(kernel::MouseEvent& event)
	{
		if (event.subtype == kernel::MouseWheelMoved)
		{
			LOGV("wheel direction: %i\n", event.wheel_direction);
		}
		else if (event.subtype == kernel::MouseMoved)
		{
			LOGV("mouse moved: %i %i [%i %i]\n", event.mx, event.my, event.dx, event.dy);
		}
		else if (event.subtype == kernel::MouseButton)
		{
			LOGV("mouse button: %s, %i -> %s\n", event.is_down ? "Yes" : "No", event.button, input::mouse_button_name(event.button));
		}
		else
		{
			LOGV("mouse event: %i\n", event.subtype);
		}
	}

	virtual kernel::Error startup()
	{
		platform::PathString root_path;
		platform::PathString content_path = platform::get_program_directory();
		platform::fs_content_directory(content_path, root_path);
		
		platform::PathString application_path = platform::get_user_application_directory("arcfusion.net/orion");
		core::startup_filesystem();
		core::filesystem::instance()->root_directory(root_path);
		core::filesystem::instance()->content_directory(content_path);
		core::filesystem::instance()->user_application_directory(application_path);
		
		core::startup_logging();
		
		// create a platform window
#if USE_WINDOW_LIBRARY
		{
			window_interface = platform::create_window_library();
			window_interface->startup(kernel::parameters());
		
			platform::WindowParameters window_params;
			window_params.window.width = 800;
			window_params.window.height = 600;
			window_params.window_title = "orion";
//			window_params.enable_fullscreen = true;
			main_window = window_interface->create_window(window_params);
			window_interface->show_mouse(true);
		}
#else
		{
			platform::window::startup(platform::window::RenderingBackend_Default);

			PLATFORM_LOG(LogMessageType::Info, "total screens: %zu\n", platform::window::screen_count());
			
			for (size_t screen = 0; screen < platform::window::screen_count(); ++screen)
			{
				platform::window::Frame frame = platform::window::screen_frame(screen);
				PLATFORM_LOG(LogMessageType::Info, "screen rect: %zu, origin: %i, %i; resolution: %i x %i\n", screen, frame.x, frame.y, frame.width, frame.height);
			}
			
			platform::window::Parameters params;
			params.window.width = 800;
			params.window.height = 600;
			params.window_title = "orion";
			params.target_display = 0;
			main_window = platform::window::create(params);
		}
#endif

		// old renderer initialize
		{
			renderer::RenderSettings render_settings;
			render_settings.gamma_correct = true;
						
			renderer::startup(renderer::OpenGL, render_settings);
			
			// clear errors
//			gl.CheckError("before render startup");

//			fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		}
		
		// initialize the renderer
		{
			using namespace render2;
			RenderParameters params;
			
			// set some options
			params["vsync"] = "true";
			params["double_buffer"] = "true";
			params["depth_size"] = "24";
			params["multisample"] = "4";

			// set opengl specific options
			params["rendering_backend"] = "opengl";
			params["opengl.major"] = "3";
			params["opengl.minor"] = "2";
			params["opengl.profile"] = "core";
			params["opengl.share_context"] = "true";
			
			for (RenderParameters::Iterator it = params.begin(); it != params.end(); ++it)
			{
				const param_string& key = it.key();
				const param_string& value = it.value();
				LOGV("'%s' -> '%s'\n", key(), value());
			}
			
			
			device = create_device(params);
			
			device->init(800, 600);

			// setup shaders
			render2::PipelineDescriptor desc;
			desc.shader = device->create_shader("test");
			
			render2::VertexDescriptor& vertex_format = desc.vertex_description;
			vertex_format.add("in_position", render2::VD_FLOAT, 3);
			vertex_format.add("in_color", render2::VD_FLOAT, 4);
			desc.input_layout = device->create_input_layout(vertex_format, desc.shader);

			
			
			pipeline = device->create_pipeline(desc);

			size_t total_bytes = sizeof(MyVertex) * 4;
			triangle_stream = device->create_vertex_buffer(total_bytes);
			MyVertex* vertex = reinterpret_cast<MyVertex*>(device->buffer_lock(triangle_stream));
			
//			MyVertex vertex[4];
			
			vertex[0].set_position(0, 600, 0);
			vertex[0].set_color(1.0f, 0.0f, 0.0f, 1.0f);
			
			vertex[1].set_position(800, 600, 0);
			vertex[1].set_color(0.0f, 1.0f, 0.0f, 1.0f);
			
			vertex[2].set_position(400, 0, 0);
			vertex[2].set_color(0.0f, 0.0f, 1.0f, 1.0f);
			
			vertex[3].set_position(0, 0, 0);
			vertex[3].set_color(0.0f, 1.0f, 1.0f, 1.0f);

			device->buffer_unlock(triangle_stream);

			
			index_buffer = device->create_index_buffer(sizeof(uint16_t) * 3);
			uint16_t indices[] = {0, 1, 2};
			device->buffer_upload(index_buffer, indices, sizeof(uint16_t)*3);

			
			

			
			
			
			// load shaders from disk
			
		}
		
		// setup editor assets / content paths
		{
//			fs->add_virtual_path("editor/assets");
		}

		
		// load the gui
		{
			// gui layout

			core::filesystem::IFileSystem* fs = core::filesystem::instance();
			
			platform::File handle = platform::fs_open("ui/main.ui", platform::FileMode_Read);
			if (handle.is_open())
			{
//				core::DataStream* stream = fs->memory_from_file(handle);
			
				// create the gui elements from a file
//				compositor->create_layout_from_memory(stream->get_data(), stream->get_data_size());
			
				platform::fs_close(handle);
			}
			
		}
		
		return kernel::NoError;
	}
	

	
	virtual void tick()
	{
#if defined(USE_WINDOW_LIBRARY)
		if (window_interface)
			window_interface->process_events();
#else
		platform::window::dispatch_events();
#endif
		
				
		static float value = 0.0f;
		static float multiplifer = 1.0f;
		
		value += 0.01f * multiplifer;
		value = glm::clamp(value, 0.0f, 1.0f);
		if (value == 0.0f || value == 1.0f)
			multiplifer *= -1;
		
		
		struct leconstants
		{
			glm::mat4 modelview_matrix;
			glm::mat4 projection_matrix;
		};
		
		leconstants cb;
		cb.modelview_matrix = glm::mat4(1.0f);
		cb.projection_matrix = glm::ortho(0.0f, 800.0f, 600.0f, 0.0f, -1.0f, 1.0f);
		pipeline->constants()->assign(&cb, sizeof(leconstants));

		value = 0.0f;

		render2::Pass render_pass;
		render_pass.target = device->default_render_target();
		render_pass.color(value, value, value, 1.0f);

		render2::CommandQueue queue(&render_pass);
		render2::CommandSerializer* serializer = device->create_serializer(queue);
		
		serializer->pipeline(pipeline);
		serializer->vertex_buffer(triangle_stream);
//		serializer->draw_indexed_primitives(index_buffer, 3);
		serializer->draw(0, 3);
		
#if !defined(USE_WINDOW_LIBRARY)
		platform::window::begin_rendering(main_window);
#endif
				
		device->queue_buffers(&queue, 1);
		device->submit();
				
		device->destroy_serializer(serializer);
		
#if !defined(USE_WINDOW_LIBRARY)
		platform::window::end_rendering(main_window);
#endif

//		glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 2000);
		
#if defined(USE_WINDOW_LIBRARY)
		if (window_interface)
		{
			window_interface->swap_buffers(main_window);
		}
#endif
	}
	
	
	virtual void shutdown()
	{
		// shutdown the render device
		device->destroy_buffer(triangle_stream);
		device->destroy_buffer(index_buffer);
		device->destroy_pipeline(pipeline);
		
		
		destroy_device(device);
		
//		glDeleteSync(fence);
		
		renderer::shutdown();
#if defined(USE_WINDOW_LIBRARY)
		if (window_interface)
		{
			window_interface->destroy_window(main_window);
			window_interface->shutdown();
			platform::destroy_window_library();
		}
#else
		platform::window::destroy(main_window);
		platform::window::shutdown();
#endif

		core::shutdown();
	}
	
	
	virtual void event(kernel::KeyboardEvent& event)
	{
		if (event.key == input::KEY_ESCAPE && event.is_down)
		{
			set_active(false);
		}
		else
		{
			LOGV("key is_down: '%s', name: '%s', modifiers: %zu\n", event.is_down ? "Yes" : "No", input::key_name(event.key), event.modifiers);
		}
	}

};

PLATFORM_MAIN
{
	PLATFORM_IMPLEMENT_PARAMETERS();
	PLATFORM_RETURN(platform::run_application(new EditorKernel()));
}
