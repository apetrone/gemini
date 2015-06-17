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

#include <runtime/core.h>
#include <runtime/logging.h>

#include <platform/platform.h>
#include <platform/kernel.h>
#include <platform/windowlibrary.h>
#include <platform/input.h>

#include <core/hashset.h>
#include <core/stackstring.h>

#include <runtime/filesystem.h>

#include <nom/nom.hpp>
#include <nom/compositor.hpp>
#include <nom/graph.hpp>
#include <nom/button.hpp>


using namespace platform;


namespace render2
{
	typedef core::StackString<128> param_string;
	typedef HashSet<param_string, param_string, core::util::hash> RenderParameters;
	
	struct command_buffer
	{
		void clear_color(float r, float g, float b, float a)
		{
			
		}
		void clear()
		{
			
		}
	};
	
	struct command_queue
	{
		void submit_buffers(const command_buffer* buffers, size_t total_buffers)
		{
		}
	};
	
	struct context
	{
		
	};
	
	struct pipeline_state
	{
		
	};
	
	struct render_target
	{
	};
	
	struct device
	{
		void pipeline(const pipeline_state& state)
		{
		}
	

		
		void activate_render_target(const render_target& rt) {}
		void deactivate_render_target(const render_target& rt) {}
		
		void activate_context(const context& c) {}
		void swap_context_buffers(const context& c) {}
	};
	

	
	device* create_device(const RenderParameters& params)
	{
		// determine the renderer
		assert(params.has_key("renderer"));
		
		const param_string& renderer = params["renderer"];
		LOGV("create renderer '%s'\n", renderer());
	
	
		return nullptr;
	}
	
	void destroy_device(device* device)
	{
		
	}
}


class EditorKernel : public kernel::IKernel,
public kernel::IEventListener<kernel::KeyboardEvent>
{
private:
	bool active;
	platform::IWindowLibrary* window_interface;
	platform::NativeWindow* main_window;
	
	render2::device* device;
	render2::command_buffer commandbuffer;
	
public:
	EditorKernel() :
		active(true),
		window_interface(0)
	{
	}
	
	virtual ~EditorKernel() {}
	
	virtual bool is_active() const { return active; }
	virtual void set_active(bool isactive) { active = isactive; }
	
	virtual void resolution_changed(int width, int height)
	{
		LOGV("resolution_changed %i %i\n", width, height);
	}

	virtual kernel::Error startup()
	{
		platform::PathString root_path;
		platform::PathString content_path;
		platform::get_program_directory(&root_path[0], root_path.max_size());
		platform::fs_content_directory(content_path, root_path);
		
		platform::PathString application_path = platform::get_user_application_directory("arcfusion.net/orion");
		core::startup_filesystem();
		core::fs::instance()->root_directory(root_path);
		core::fs::instance()->content_directory(content_path);
		core::fs::instance()->user_application_directory(application_path);
		
		core::startup_logging();
		
		// create a platform window
		{
			window_interface = platform::create_window_library();
			window_interface->startup(kernel::parameters());
		
			platform::WindowParameters window_params;
			window_params.window_width = 800;
			window_params.window_height = 600;
			window_params.window_title = "orion";
//			window_params.enable_fullscreen = true;
			main_window = window_interface->create_window(window_params);
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
			params["renderer"] = "opengl";
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
		}
		
		// setup editor assets / content paths
		{
//			fs->add_virtual_path("editor/assets");
		}

		
		// load the gui
		{
			// gui layout

			core::filesystem::IFileSystem* fs = core::fs::instance();
			
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
		window_interface->process_events();
		
		
		commandbuffer.clear_color(1.0f, 1.0f, 1.0f, 1.0f);
		commandbuffer.clear();
		
		render2::pipeline_state state;
		device->pipeline(state);
		
		render2::command_queue queue;
		queue.submit_buffers(&commandbuffer, 1);
		
		window_interface->swap_buffers(main_window);
	}
	
	virtual void shutdown()
	{
		// shutdown the render device
		destroy_device(device);
	
		window_interface->destroy_window(main_window);
		window_interface->shutdown();
		platform::destroy_window_library();
		core::shutdown();
	}
	
	
	virtual void event(kernel::KeyboardEvent& event)
	{
		if (event.key == input::KEY_ESCAPE && event.is_down)
		{
			set_active(false);
		}
	}
	
};



PLATFORM_MAIN
{
	int return_code;
	return_code = platform::run_application(new EditorKernel());
	return return_code;
}