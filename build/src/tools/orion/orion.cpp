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

#include <renderer/renderer.h>
#include <renderer/font.h>
#include <renderer/guirenderer.h>
#include <renderer/standaloneresourcecache.h>

#include <runtime/core.h>
#include <runtime/logging.h>
#include <runtime/filesystem.h>

#include <platform/platform.h>
#include <platform/window.h>
#include <platform/kernel.h>
#include <platform/input.h>

#include <core/hashset.h>
#include <core/stackstring.h>
#include <core/fixedarray.h>

#include <ui/ui.h>
#include <ui/compositor.h>
#include <ui/graph.h>
#include <ui/button.h>




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


namespace gui
{
	class TimelineScrubber : public Panel
	{
	public:
		TimelineScrubber(Panel* parent)
			: Panel(parent)
		{
			flags |= Flag_CursorEnabled;
			set_name("TimelineScrubber");
		}

		virtual void render(gui::Compositor* compositor, gui::Renderer* renderer, gui::render::CommandList& render_commands) override
		{
			// TODO: we should get this from the style
			core::Color scrubber_highlight = core::Color::from_rgba(255, 128, 0, 32);
			core::Color scrubber_outline = core::Color::from_rgba(255, 128, 0, 192);

			// draw the main highlight fill
			render_commands.add_rectangle(geometry[0], geometry[1], geometry[2], geometry[3], gui::render::WhiteTexture, scrubber_highlight);

			// draw the outline
			render_commands.add_line(geometry[0], geometry[1], scrubber_outline);
			render_commands.add_line(geometry[1], geometry[2], scrubber_outline);
			render_commands.add_line(geometry[2], geometry[3], scrubber_outline);
			render_commands.add_line(geometry[3], geometry[0], scrubber_outline);
		}
	};

	class Timeline : public Panel
	{
	public:
		Timeline(Panel* parent)
			: Panel(parent)
			, left_margin(0)
			, current_frame(0)
			, frame_width_pixels(0.0f)
		{
			flags |= Flag_CursorEnabled;
			set_name("Timeline");

			scrubber = new TimelineScrubber(this);
			scrubber->set_origin(0.0f, 0.0f);
		}

		gui::DelegateHandler<size_t> on_scrubber_changed;

		virtual void handle_event(EventArgs& args) override
		{
			last_position = args.local;
			if (args.type == Event_CursorDrag || args.type == Event_CursorButtonPressed)
			{
				// snap to the closest point
				size_t last_frame = current_frame;

				int next_frame = (((int)args.local.x) - 1) / frame_width_pixels;

				if (next_frame < 0)
					current_frame = 0;
				else if (next_frame > (int)total_frames)
					current_frame = total_frames;
				else
					current_frame = next_frame;

				if ( current_frame <= 0 )
				{
					current_frame = 0;
				}
				else if ( current_frame > total_frames-1 )
				{
					current_frame = total_frames-1;
				}

				if (last_frame != current_frame)
				{
					on_scrubber_changed(current_frame);
				}
			}
		} // handle_event

		virtual void update(gui::Compositor* compositor, float delta_seconds) override
		{
			Point dimensions = scrubber->dimensions_from_pixels(Point(frame_width_pixels, bounds.size.height));

			scrubber->set_dimensions(dimensions);
			scrubber->set_origin((current_frame * frame_width_pixels), 0.0f);

			Panel::update(compositor, delta_seconds);
		} // update

		virtual void render(gui::Compositor* compositor, gui::Renderer* renderer, gui::render::CommandList& render_commands) override
		{
			// TODO: should get this from the style
			const core::Color frame_color(96, 96, 96, 255);

			// assuming a horizontal timeline
			if (frame_width_pixels == 0)
			{
				// recompute the distance here
				frame_width_pixels = (bounds.size.width / (float)total_frames);
			}

			// should be updated before rendering
			assert(frame_width_pixels > 0);

			// draw the background
			render_commands.add_rectangle(geometry[0], geometry[1], geometry[2], geometry[3], gui::render::WhiteTexture, core::Color(64, 64, 64, 255));

			// add a top rule line to separate this panel
			render_commands.add_line(geometry[0], geometry[3], core::Color(0, 0, 0, 255), 1.0f);

			Rect frame;
			get_screen_bounds(frame);

			float origin_x = frame.origin.x + left_margin;
			float origin_y = frame.origin.y + 1.0f;

			// center the individual frames
			Rect block;
			block.set(origin_x, origin_y, 1.0f, frame.size.height - 2.0f);

			for (size_t index = 0; index < total_frames; ++index)
			{
				// draw frame ticks until we reach the end of the panel
				if (block.origin.x + block.size.width >= (frame.origin.x + frame.size.width))
				{
					break;
				}

				Point points[4];
				points[0].x = block.origin.x;
				points[0].y = block.origin.y;

				points[1].x = block.origin.x + block.size.width;
				points[1].y = block.origin.y;

				points[2].x = block.origin.x + block.size.width;
				points[2].y = block.origin.y + block.size.height;

				points[3].x = block.origin.x;
				points[3].y = block.origin.y + block.size.height;

				// draw each frame's area
				render_commands.add_rectangle(points[0], points[1], points[2], points[3], gui::render::WhiteTexture, frame_color);

				block.origin.x += frame_width_pixels;
			}

			render_children(compositor, renderer, render_commands);
		} // render

		void set_frame_range(int lower_frame_limit, int upper_frame_limit)
		{
			lower_limit = lower_frame_limit;
			upper_limit = upper_frame_limit;

			assert(upper_limit > lower_limit);
			total_frames = (upper_limit - lower_limit);

			// force a recalculate on the next render call
			frame_width_pixels = 0;
		} // set_frame_range

	private:
		size_t left_margin;
		size_t current_frame;
		size_t total_frames;

		// frame limits
		int lower_limit;
		int upper_limit;

		// width of a clickable 'frame'
		float frame_width_pixels;

		Point last_position;

		TimelineScrubber* scrubber;
	}; // Timeline


	// This uses a render target to present data
	class RenderableSurface : public gui::Panel
	{
	public:
		RenderableSurface(Panel* parent)
			: Panel(parent)
			, target(nullptr)
			, handle(render::WhiteTexture)
		{
			flags |= Flag_CursorEnabled;
			set_name("RenderableSurface");
		}

		void set_render_target(render2::RenderTarget* render_target) { target = render_target; }
		render2::RenderTarget* get_render_target() const { return target; }
		void set_texture_handle(int ref) { handle = ref; }

		virtual void render(gui::Compositor* compositor, gui::Renderer* renderer, gui::render::CommandList& render_commands) override
		{
			on_render_content(target);

			render_commands.add_rectangle(geometry[0], geometry[1], geometry[2], geometry[3], handle, core::Color::from_rgba(255, 255, 255, 255));
		}

		// invoked when the handler should render its content to the render
		// target.
		gui::DelegateHandler<render2::RenderTarget*> on_render_content;

	private:
		//
		render2::RenderTarget* target;
		int handle;
	}; // RenderableSurface
}

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

class LogWindow : public gui::Panel
{
public:
	LogWindow(Panel* parent)
		: Panel(parent)
	{
		flags |= Flag_CanMove | Flag_CursorEnabled;
	}

	virtual void update(gui::Compositor* compositor, float delta_seconds) override
	{
		Panel::update(compositor, delta_seconds);
	} // update

	virtual void render(gui::Compositor* compositor, gui::Renderer* renderer, gui::render::CommandList& render_commands) override
	{
		render_commands.add_rectangle(geometry[0], geometry[1], geometry[2], geometry[3], gui::render::WhiteTexture, core::Color::from_rgba(255, 255, 255, 255));
	}
};


class EditorKernel : public kernel::IKernel,
public kernel::IEventListener<kernel::KeyboardEvent>,
public kernel::IEventListener<kernel::MouseEvent>,
public kernel::IEventListener<kernel::SystemEvent>
{
private:
	bool active;
	platform::window::NativeWindow* main_window;

	render2::Device* device;
	render2::Buffer* vertex_buffer;

	render2::Pipeline* pipeline;

	glm::mat4 modelview_matrix;
	glm::mat4 projection_matrix;

//	GLsync fence;

	gui::Compositor* compositor;
	gui::Label* log_window;
	GUIRenderer* gui_renderer;
	::renderer::StandaloneResourceCache resource_cache;
	render2::RenderTarget* render_target;
	render2::Texture* texture;

	platform::Process* process;

	float value;

public:
	EditorKernel()
		: active(true)
		, compositor(nullptr)
		, gui_renderer(nullptr)
		, render_target(nullptr)
		, texture(nullptr)
		, process(nullptr)
		, value(0.0f)
	{
	}

	virtual ~EditorKernel() {}

	virtual bool is_active() const { return active; }
	virtual void set_active(bool isactive) { active = isactive; }

	virtual void event(kernel::SystemEvent& event)
	{
		if (event.subtype == kernel::WindowResized)
		{
			platform::window::Frame frame = platform::window::get_render_frame(main_window);

			assert(device);
			device->backbuffer_resized(frame.width, frame.height);

			compositor->resize(frame.width, frame.height);
		}
	}

	virtual void event(kernel::MouseEvent& event)
	{
		if (compositor)
		{
			static gui::CursorButton::Type input_to_gui[] = {
				gui::CursorButton::None,
				gui::CursorButton::Left,
				gui::CursorButton::Right,
				gui::CursorButton::Middle,
				gui::CursorButton::Mouse4,
				gui::CursorButton::Mouse5
			};

			if (event.subtype == kernel::MouseMoved)
			{
				compositor->cursor_move_absolute(event.mx, event.my);
			}
			else if (event.subtype == kernel::MouseButton)
			{
				if (event.is_down)
				{
					platform::window::set_mouse_tracking(true);
				}
				else
				{
					platform::window::set_mouse_tracking(false);
				}
				compositor->cursor_button(input_to_gui[event.button], event.is_down);
			}
			else if (event.subtype == kernel::MouseWheelMoved)
			{
				compositor->cursor_scroll(event.wheel_direction);
			}
		}
	}

	void test_open_dialog()
	{
		Array<PathString> paths;
		uint32_t flags = platform::OpenFlags::CanChooseDirectories;

		if (platform::show_open_dialog("Choose Game Directory", flags, paths).succeeded())
		{
			fprintf(stdout, "target path is: %s\n", paths[0]());
		}
	}

	void start_watching_assets(const char* project_path)
	{
		std::string asset_root = project_path;
		asset_root.append(PATH_SEPARATOR_STRING);
		asset_root.append("assets");

		// launch the blacksmith tool
		const std::string script_path = "tools/blacksmith/blacksmith.py";
		const std::string config_path = "tools/conf/blacksmith/desktop_monitor.conf";


		// close the process if it's already open
	}

	void stop_watching_assets()
	{
	}


	void timeline_scrubber_changed(size_t current_frame)
	{
		value = (current_frame / 30.0f);
	}

	void render_main_content(render2::RenderTarget* render_target)
	{
		render2::Pass render_pass;
		render_pass.color(0.0f, value, value, 1.0f);
		render_pass.clear_color = true;
		render_pass.clear_depth = true;
		render_pass.depth_test = false;
		render_pass.target = render_target;

		render2::CommandQueue* queue = device->create_queue(render_pass);
		render2::CommandSerializer* serializer = device->create_serializer(queue);

		serializer->pipeline(pipeline);

		const bool test_triangle = 1;
		if (test_triangle)
		{
			serializer->vertex_buffer(vertex_buffer);
			serializer->draw(0, 3);
			device->queue_buffers(queue, 1);
		}
		device->destroy_serializer(serializer);
	}

	virtual kernel::Error startup()
	{
		platform::PathString root_path = platform::get_program_directory();
		platform::PathString content_path = platform::fs_content_directory();

		platform::PathString application_path = platform::get_user_application_directory("arcfusion.net/orion");
		core::startup_filesystem();
		core::filesystem::instance()->root_directory(root_path);
		core::filesystem::instance()->content_directory(content_path);
		core::filesystem::instance()->user_application_directory(application_path);

		core::startup_logging();

		// create a platform window
		{
			platform::window::startup(platform::window::RenderingBackend_Default);

			platform::window::Parameters params;

			bool enable_fullscreen = false;
			if (enable_fullscreen)
			{
				params.enable_fullscreen = enable_fullscreen;
				params.frame = platform::window::screen_frame(0);
			}
			else
			{
				params.frame = platform::window::centered_window_frame(0, 800, 600);
			}

			params.window_title = "orion";
			main_window = platform::window::create(params);
		}

		// old renderer initialize
		{
//			renderer::RenderSettings render_settings;
//			render_settings.gamma_correct = true;

//			renderer::startup(renderer::OpenGL, render_settings);

			// clear errors
//			gl.CheckError("before render startup");

//			fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		}

		platform::window::Frame window_frame;

		// initialize the renderer
		{
			using namespace render2;
			RenderParameters params;

			// set some options
			params["vsync"] = "true";
			params["double_buffer"] = "true";
			params["depth_size"] = "24";
//			params["multisample"] = "4";

			// set opengl specific options
			params["rendering_backend"] = "opengl";
			params["opengl.major"] = "3";
			params["opengl.minor"] = "2";
			params["opengl.profile"] = "core";
			params["opengl.share_context"] = "true";
			
//			for (RenderParameters::Iterator it = params.begin(); it != params.end(); ++it)
//			{
//				const param_string& key = it.key();
//				const param_string& value = it.value();
//				LOGV("'%s' -> '%s'\n", key(), value());
//			}

			device = create_device(params);

			window_frame = platform::window::get_frame(main_window);
			device->init(window_frame.width, window_frame.height);

			// setup shaders
			render2::PipelineDescriptor desc;
			desc.shader = device->create_shader("vertexcolor");
			render2::VertexDescriptor& vertex_format = desc.vertex_description;
			vertex_format.add("in_position", render2::VD_FLOAT, 3);
			vertex_format.add("in_color", render2::VD_FLOAT, 4);
			desc.input_layout = device->create_input_layout(vertex_format, desc.shader);
			pipeline = device->create_pipeline(desc);

			size_t total_bytes = sizeof(MyVertex) * 6;
			vertex_buffer = device->create_vertex_buffer(total_bytes);

#if 1
			MyVertex* vertex = reinterpret_cast<MyVertex*>(device->buffer_lock(vertex_buffer));

//			MyVertex vertex[4];

			vertex[0].set_position(0, window_frame.height, 0);
			vertex[0].set_color(1.0f, 0.0f, 0.0f, 1.0f);

			vertex[1].set_position(window_frame.width, window_frame.height, 0);
			vertex[1].set_color(0.0f, 1.0f, 0.0f, 1.0f);

			vertex[2].set_position(window_frame.width/2.0f, 0, 0);
			vertex[2].set_color(0.0f, 0.0f, 1.0f, 1.0f);

			vertex[3].set_position(0, 0, 0);
			vertex[3].set_color(0.0f, 1.0f, 1.0f, 1.0f);

			device->buffer_unlock(vertex_buffer);
#endif
		}

		// setup editor assets / content paths
		{
//			fs->add_virtual_path("editor/assets");
		}

		font::startup(device);

#if 0
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
#else
		{
			// in lieu of the above working; manually setup the gui...
			gui_renderer = MEMORY_NEW(GUIRenderer, core::memory::global_allocator())(resource_cache);
			gui_renderer->set_device(device);

			compositor = new gui::Compositor(window_frame.width, window_frame.height, &resource_cache, gui_renderer);

#if 0
			gui::Timeline* timeline = new gui::Timeline(compositor);
			timeline->set_bounds(0, 550, 800, 50);
			timeline->set_frame_range(0, 30);
			timeline->on_scrubber_changed.connect(&EditorKernel::timeline_scrubber_changed, this);
#endif


#if 0
			gui::RenderableSurface* surface = new gui::RenderableSurface(compositor);
			surface->set_bounds(0, 0, 512, 512);
			surface->on_render_content.connect(&EditorKernel::render_main_content, this);

			image::Image checker_pattern;
			checker_pattern.create(512, 512, 3);
			checker_pattern.fill(core::Color(0, 25, 25));
			texture = device->create_texture(checker_pattern);

			int handle = resource_cache.track_texture(texture);

			// TODO: sort out this interface!
			render_target = device->create_render_target(texture);
			surface->set_render_target(render_target);
			surface->set_texture_handle(handle);
#endif

			log_window = new gui::Label(compositor);
			log_window->set_origin(0.0f, 0.0f);
			log_window->set_dimensions(1.0f, 0.25f);
			log_window->set_font("fonts/debug.ttf", 16);
			log_window->set_text("log initialized.");

			// install a log handler
		}
#endif
		kernel::parameters().step_interval_seconds = (1.0f/50.0f);

		// test some stuff
		Array<PathString> arguments;
#if 0
		arguments.push_back("--version");
		process = platform::process_create("/usr/bin/clang", arguments);
#elif 0
		// test blacksmith
		arguments.push_back("/Users/apetrone/gemlin/tools/blacksmith/blacksmith.py");
		arguments.push_back("-c");
		arguments.push_back("/Users/apetrone/gemlin/tools/conf/blacksmith/desktop_monitor.conf");
		arguments.push_back("-s");
		arguments.push_back("/Users/apetrone/Documents/games/vrpowergrid/assets");
		process = platform::process_create("/Users/apetrone/gemlin/tools/env/bin/python", arguments);
#endif

		return kernel::NoError;
	}



	virtual void tick()
	{
		platform::window::dispatch_events();

		static float value = 0.0f;
		static float multiplifer = 1.0f;

		value += 0.01f * multiplifer;
		value = glm::clamp(value, 0.0f, 1.0f);
		if (value == 0.0f || value == 1.0f)
			multiplifer *= -1;

		if (compositor)
		{
			compositor->tick(kernel::parameters().step_interval_seconds);
			compositor->process_events();
		}

		platform::window::Frame window_frame = platform::window::get_frame(main_window);

		modelview_matrix = glm::mat4(1.0f);
		projection_matrix = glm::ortho(0.0f, window_frame.width, window_frame.height, 0.0f, -1.0f, 1.0f);
		pipeline->constants().set("modelview_matrix", &modelview_matrix);
		pipeline->constants().set("projection_matrix", &projection_matrix);

		value = 0.15f;

		render2::Pass render_pass;
		render_pass.target = device->default_render_target();
		render_pass.color(value, value, value, 1.0f);
		render_pass.clear_color = true;
		render_pass.clear_depth = true;
		render_pass.depth_test = false;

		render2::CommandQueue* queue = device->create_queue(render_pass);
		render2::CommandSerializer* serializer = device->create_serializer(queue);

		serializer->pipeline(pipeline);
//		serializer->vertex_buffer(vertex_buffer);
//		serializer->draw(0, 3);
		device->queue_buffers(queue, 1);
		device->destroy_serializer(serializer);

		platform::window::activate_context(main_window);

		if (compositor)
		{
			compositor->draw();
		}

		device->submit();



		platform::window::swap_buffers(main_window);

//		glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 2000);
	}


	virtual void shutdown()
	{
		fprintf(stdout, "terminating process...\n");
		platform::process_destroy(process);

		device->destroy_render_target(render_target);
		device->destroy_texture(texture);

		// compositor will cleanup children
		delete compositor;

		// shutdown the gui
		MEMORY_DELETE(gui_renderer, core::memory::global_allocator());

		// explicitly clear the resource cache or else the allocator will
		// detect leaks.
		resource_cache.clear();

		font::shutdown();

		// shutdown the render device
		device->destroy_buffer(vertex_buffer);

		device->destroy_pipeline(pipeline);

		destroy_device(device);

//		glDeleteSync(fence);

//		renderer::shutdown();

		platform::window::destroy(main_window);
		platform::window::shutdown();

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
