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

#include <runtime/runtime.h>
#include <runtime/filesystem.h>
#include <runtime/geometry.h>
#include <runtime/guirenderer.h>
#include <runtime/standaloneresourcecache.h>

#include <platform/platform.h>
#include <platform/window.h>
#include <platform/kernel.h>
#include <platform/input.h>

#include <core/hashset.h>
#include <core/stackstring.h>
#include <core/fixedarray.h>
#include <core/logging.h>
#include <core/profiler.h>
#include <core/mathlib.h>
#include <core/argumentparser.h>

#include <ui/button.h>
#include <ui/compositor.h>
#include <ui/graph.h>
#include <ui/label.h>
#include <ui/layout.h>
#include <ui/menu.h>
#include <ui/timeline.h>
#include <ui/ui.h>

#include <sdk/camera.h>
#include <sdk/game_api.h>
#include <sdk/utils.h>

#include <renderer/debug_draw.h>


#include "project.h"

using namespace platform;
using namespace renderer;
using namespace gemini;


#include <runtime/imocap.h>


#define ENABLE_UI 1
#define DRAW_SENSOR_GRAPHS 0

namespace gui
{



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

		virtual void render(gui::Compositor* /*compositor*/, gui::Renderer* /*renderer*/, gui::render::CommandList& render_commands) override
		{
			if (on_render_content.is_valid())
			{
				on_render_content(target);
			}

			render_commands.add_rectangle(geometry[0], geometry[1], geometry[2], geometry[3], handle, gemini::Color::from_rgba(255, 255, 255, 255));
		}

		// invoked when the handler should render its content to the render
		// target.
		gemini::Delegate<void (render2::RenderTarget*)> on_render_content;

	private:
		render2::RenderTarget* target;
		int handle;
	}; // RenderableSurface
} // namespace gui

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


void log_window_logger_message(core::logging::Handler* handler, const char* message, const char* /*filename*/, const char* /*function*/, int /*line*/, int /*type*/)
{
	gui::Label* logwindow = static_cast<gui::Label*>(handler->userdata);
	logwindow->append_text(message);
	logwindow->scroll_to_bottom();
}

int log_window_logger_open(core::logging::Handler* /*handler*/)
{
	return 1;
}

void log_window_logger_close(core::logging::Handler* /*handler*/)
{
}

// Asset Processing
#if 0
// 1. asset source (currently opened project)
// 2. destination folder: <asset_source>/../builds/<platform>/

// tools for use by asset processor.
// libsox/libogg: audio format conversions
// pvrtexlib: PowerVR texture compression
// nvtt / libsquish: texture compression
// models/animation: custom code
// packaging: custom code
#endif

class AssetProcessingPanel : public gui::Panel
{
public:

	AssetProcessingPanel(gui::Panel* parent)
		: gui::Panel(parent)
	{
		flags |= gui::Panel::Flag_CanMove;
	}
}; // AssetProcessingPanel






size_t total_bytes = sizeof(MyVertex) * 4;


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

	render2::Pipeline* surface_pipeline;
	glm::mat4 surface_modelview;
	glm::mat4 surface_projection;

	glm::mat4 joint_offets[IMOCAP_TOTAL_SENSORS];

	MyVertex vertex_data[4];

//	GLsync fence;

	gui::Compositor* compositor;
	gui::Label* log_window;
	GUIRenderer* gui_renderer;
	gui::Panel* main_panel;
	::renderer::StandaloneResourceCache resource_cache;
	render2::RenderTarget* render_target;
	render2::Texture* texture;
	AssetProcessingPanel* asset_processor;

	gui::Graph* graphs[IMOCAP_TOTAL_SENSORS];

	Array<imocap::mocap_frame_t> mocap_frames;
	size_t current_mocap_frame;
	PathString current_mocap_filename;

	float value;

	core::logging::Handler log_handler;

	Camera camera;

	float yaw;
	float pitch;

	bool left_down;
	bool right_down;
	bool forward_down;
	bool backward_down;

	uint64_t last_time;

	bool should_move_view;

	bool is_recording_frames;
	bool is_playing_frames;
	bool app_in_focus;

	glm::vec3 position_test;
	glm::vec3 velocity_test;

	imocap::MocapDevice* mocap_device;

public:
	EditorKernel()
		: active(true)
		, compositor(nullptr)
		, gui_renderer(nullptr)
		, render_target(nullptr)
		, texture(nullptr)
		, value(0.0f)
		, is_recording_frames(false)
		, is_playing_frames(false)
		, app_in_focus(true)
		, mocap_device(nullptr)
	{
		yaw = 0.0f;
		pitch = 0.0f;

		left_down = right_down = forward_down = backward_down = false;

		last_time = 0;

		should_move_view = false;

		asset_processor = nullptr;
	}

	virtual ~EditorKernel() {}

	virtual bool is_active() const { return active; }
	virtual void set_active(bool isactive) { active = isactive; }

	virtual void event(kernel::KeyboardEvent& event)
	{
		if (event.key == gemini::BUTTON_ESCAPE && event.is_down)
		{
			set_active(false);
		}
		//else
		//{
		//	LOGV("key is_down: '%s', name: '%s', modifiers: %zu\n", event.is_down ? "Yes" : "No", gemini::key_name(event.key), event.modifiers);
		//}

		if (event.key == BUTTON_A)
			left_down = event.is_down;

		if (event.key == BUTTON_D)
			right_down = event.is_down;

		if (event.key == BUTTON_W)
			forward_down = event.is_down;

		if (event.key == BUTTON_S)
			backward_down = event.is_down;

		if (event.key == BUTTON_SPACE)
		{
			LOGV("freezing rotations\n");
			imocap::zero_rotations(mocap_device);

			position_test = velocity_test = glm::vec3(0.0f, 0.0f, 0.0f);

		}
	}


	virtual void event(kernel::SystemEvent& event)
	{
		if (event.subtype == kernel::WindowResized)
		{
			assert(device);
			device->backbuffer_resized(event.render_width, event.render_height);

			assert(compositor);
			compositor->resize(event.window_width, event.window_height);

			main_panel->set_size(event.window_width, event.window_height-24);
		}
		else if (event.subtype == kernel::WindowClosed)
		{
			LOGV("Window was closed!\n");
			set_active(false);
		}

		if (event.window == main_window)
		{
			if (event.subtype == kernel::WindowRestored || event.subtype == kernel::WindowGainFocus)
			{
				app_in_focus = true;
			}
			else if (event.subtype == kernel::WindowMinimized || event.subtype == kernel::WindowLostFocus)
			{
				app_in_focus = false;
			}
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
				bool handled = compositor->cursor_move_absolute(event.mx, event.my);

				if (!handled && should_move_view)
				{
					if (event.dx != 0 || event.dy != 0)
					{
						const float sensitivity = .10f;
						camera.move_view(event.dx, event.dy);
					}
				}
			}
			else if (event.subtype == kernel::MouseButton)
			{
				if (event.is_down)
				{
					if (event.button == MouseButton::MOUSE_LEFT)
					{
						should_move_view = true;
					}

					platform::window::set_mouse_tracking(true);
				}
				else
				{
					if (event.button == MouseButton::MOUSE_LEFT)
					{
						should_move_view = false;
					}
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
		uint32_t flags = platform::OpenDialogFlags::CanChooseDirectories;

		if (platform::show_open_dialog("Choose Game Directory", flags, paths).succeeded())
		{
			LOGV("target path is: %s\n", paths[0]());
		}
	}

	// menu handlers

	void on_file_new(void)
	{
	}

	void on_file_open()
	{
	}

	void on_file_quit(void)
	{
		set_active(false);
	}

	void on_record_start(void)
	{
		if (is_recording_frames == false)
		{
			current_mocap_filename.clear();
			Array<PlatformExtensionDescription> extensions;
			extensions.push_back(PlatformExtensionDescription("Sensor Stream", "sensor"));

			platform::Result save_result = platform::show_save_dialog(
				"Save Sensor Stream",
				0, /* Flags */
				extensions,
				"sensor",
				current_mocap_filename
			);

			if (save_result.succeeded())
			{
				LOGV("Save stream to: %s\n", current_mocap_filename());
				mocap_frames.clear();
				current_mocap_frame = 0;
				is_recording_frames = true;
			}
		}
	}

	void on_record_stop(void)
	{
		if (is_recording_frames)
		{
			is_recording_frames = false;

			platform::File handle = platform::fs_open(current_mocap_filename(), FileMode_Write);
			if (handle.is_open())
			{
				// 1. Total Sensors (poses)
				uint32_t total_sensors = IMOCAP_TOTAL_SENSORS;
				platform::fs_write(handle, &total_sensors, sizeof(uint32_t), 1);

				// 2. Total frames.
				const size_t total_frames = mocap_frames.size();
				platform::fs_write(handle, &total_frames, sizeof(size_t), 1);

				// 3. All frames.
				for (size_t index = 0; index < total_frames; ++index)
				{
					imocap::mocap_frame_t& frame = mocap_frames[index];

					for (size_t pose = 0; pose < IMOCAP_TOTAL_SENSORS; ++pose)
					{
						const glm::quat& rotation = frame.poses[pose];
						platform::fs_write(handle, (const void*)&rotation, sizeof(glm::quat), 1);
					}
				}

				platform::fs_close(handle);
			}

			LOGV("Recorded %i frames\n", mocap_frames.size());
			current_mocap_filename.clear();
			current_mocap_frame = 0;
			mocap_frames.clear();
		}
	}

	void on_playback_start(void)
	{
		Array<PathString> paths;
		platform::Result open_result = platform::show_open_dialog("Choose Sensor Stream", OpenDialogFlags::CanChooseFiles, paths);
		if (open_result.succeeded())
		{
			LOGV("loading sensor stream '%s'\n", paths[0]());
			platform::File handle = platform::fs_open(paths[0](), FileMode_Read);
			if (handle.is_open())
			{
				// 1. Total Sensors (poses)
				size_t total_sensors = 0;
				platform::fs_read(handle, &total_sensors, sizeof(size_t), 1);

				// If you hit this, the file loaded differs in the number of sensors
				// it contains vs the number of sensors this now supports.
				assert(total_sensors == IMOCAP_TOTAL_SENSORS);

				// 2. Read Total Frames
				size_t total_frames = 0;
				platform::fs_read(handle, &total_frames, sizeof(size_t), 1);

				LOGV("found %i total frames...\n", total_frames);
				mocap_frames.resize(total_frames);

				// 3. All frames.
				for (size_t index = 0; index < total_frames; ++index)
				{
					imocap::mocap_frame_t& frame = mocap_frames[index];
					frame.frame_index = index;
					for (size_t pose = 0; pose < total_sensors; ++pose)
					{
						glm::quat& rotation = frame.poses[pose];
						platform::fs_read(handle, (void*)&rotation, sizeof(glm::quat), 1);
					}
				}
			}

			// Set state to playback.
			current_mocap_frame = 0;
			is_playing_frames = true;
			is_recording_frames = false;

			LOGV("start playback of %i frames...\n", mocap_frames.size());
		}
	}

	void on_playback_stop(void)
	{
		is_playing_frames = false;
		LOGV("stopped playback.\n");
	}


	void on_window_toggle_asset_processor(void)
	{
		assert(asset_processor);
		asset_processor->set_visible(!asset_processor->is_visible());
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

		serializer->pipeline(surface_pipeline);

		const bool test_triangle = 1;
		if (test_triangle)
		{
			serializer->vertex_buffer(vertex_buffer);
			serializer->draw(0, 3);
			device->queue_buffers(queue, 1);
		}
		device->destroy_serializer(serializer);

		debugdraw::render(camera.get_modelview(), camera.get_projection(), render_target->width, render_target->height, render_target);
	}

	virtual kernel::Error startup()
	{
		// parse command line values
		std::vector<std::string> arguments;
		core::argparse::ArgumentParser parser;
		core::StackString<MAX_PATH_SIZE> content_path;

		runtime_load_arguments(arguments, parser);

		core::argparse::VariableMap vm;
		const char* docstring = R"(
Usage:
	--assets=<content_path>

Options:
	-h, --help  Show this help screen
	--version  Display the version number
	--assets=<content_path>  The path to load content from
	)";

		if (parser.parse(docstring, arguments, vm, "1.0.0-alpha"))
		{
			std::string path = vm["--assets"];
			content_path = platform::make_absolute_path(path.c_str());
		}
		else
		{
			return kernel::CoreFailed;
		}


		std::function<void(const char*)> custom_path_setup = [&](const char* application_data_path)
		{
			core::filesystem::IFileSystem* filesystem = core::filesystem::instance();
			PathString root_path = platform::get_program_directory();

			// the root path is the current binary path
			filesystem->root_directory(root_path);

			// the content directory is where we'll find our assets
			filesystem->content_directory(content_path);

			// load engine settings (from content path)
			//load_config(config);

			// the application path can be specified in the config (per-game basis)
			//const platform::PathString application_path = platform::get_user_application_directory(config.application_directory.c_str());
			filesystem->user_application_directory(application_data_path);
		};

		gemini::runtime_startup("arcfusion.net/orion", custom_path_setup);

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

			// set perspective on camera
			camera.perspective(60.0f, (int)params.frame.width, (int)params.frame.height, 0.01f, 1024.0f);
			camera.set_position(glm::vec3(0.0f, 5.0f, 10.0f));
			camera.set_type(Camera::FIRST_PERSON);
			camera.update_view();
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
			pipeline->constants().set("modelview_matrix", &modelview_matrix);
			pipeline->constants().set("projection_matrix", &projection_matrix);

			{
				render2::PipelineDescriptor desc;
				desc.shader = device->create_shader("vertexcolor");
				render2::VertexDescriptor& vertex_format = desc.vertex_description;
				vertex_format.add("in_position", render2::VD_FLOAT, 3);
				vertex_format.add("in_color", render2::VD_FLOAT, 4);
				desc.input_layout = device->create_input_layout(vertex_format, desc.shader);
				surface_pipeline = device->create_pipeline(desc);
				surface_pipeline->constants().set("modelview_matrix", &surface_modelview);
				surface_pipeline->constants().set("projection_matrix", &surface_projection);
			}

			vertex_buffer = device->create_vertex_buffer(total_bytes);


			// Buffer Lock and Buffer Unlock are perhaps future features.
			// This does not work as of yet.
#if 0
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
#else
			MyVertex* vertex = vertex_data;
			vertex[0].set_position(0, window_frame.height, 0);
			vertex[0].set_color(1.0f, 0.0f, 0.0f, 1.0f);

			vertex[1].set_position(window_frame.width, window_frame.height, 0);
			vertex[1].set_color(0.0f, 1.0f, 0.0f, 1.0f);

			vertex[2].set_position(window_frame.width / 2.0f, 0, 0);
			vertex[2].set_color(0.0f, 0.0f, 1.0f, 1.0f);

			vertex[3].set_position(0, 0, 0);
			vertex[3].set_color(0.0f, 1.0f, 1.0f, 1.0f);

			device->buffer_upload(vertex_buffer, vertex, total_bytes);
#endif
		}

		// setup editor assets / content paths
		{
//			fs->add_virtual_path("editor/assets");
		}

		font::startup(device);

		// initialize debug draw
		debugdraw::startup(device);

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

			assert(window_frame.width > 0);
			assert(window_frame.height > 0);
			compositor = new gui::Compositor(window_frame.width, window_frame.height, &resource_cache, gui_renderer);
			compositor->set_name("compositor");

#if ENABLE_UI
			main_panel = new gui::Panel(compositor);
			main_panel->set_name("main_panel");
			main_panel->set_origin(0, 24);
			main_panel->set_size(window_frame.width, window_frame.height-24);


			gui::HorizontalLayout* horizontal_layout = new gui::HorizontalLayout();
			gui::VerticalLayout* center_layout = new gui::VerticalLayout();
			main_panel->set_layout(horizontal_layout);
			horizontal_layout->add_layout(center_layout);
#endif


#if DRAW_SENSOR_GRAPHS
			// Create a graph for each sensor
			const char dev_font[] = "fonts/debug.ttf";
			const size_t dev_font_size = 16;

			uint32_t origin = 24;

			for (size_t index = 0; index < IMOCAP_TOTAL_SENSORS; ++index)
			{
				gui::Graph* graph = new gui::Graph(compositor);
				graph->set_origin(window_frame.width - 250, origin);
				graph->set_size(250, 100);
				origin += 108;
				graph->set_maximum_size(gui::Size(250, 100));
				graph->set_font(dev_font, dev_font_size);
				graph->set_background_color(gemini::Color::from_rgba(60, 60, 60, 255));
				graph->set_foreground_color(gemini::Color::from_rgba(255, 255, 255, 255));
				graph->create_samples(100, 1);
				graph->configure_channel(0, gemini::Color::from_rgba(255, 0, 0, 255));
				graph->set_range(-0.5f, 0.5f);
				graph->enable_baseline(true, 0.0f, gemini::Color::from_rgba(64, 64, 64, 255));

				graphs[index] = graph;
			}
#endif


#if 1
			asset_processor = new AssetProcessingPanel(compositor);
			asset_processor->set_origin(0.0f, 25.0f);
			asset_processor->set_size(400, 100);
			asset_processor->set_background_color(gemini::Color(0.25f, 0.25f, 0.25f));
			asset_processor->set_visible(false);
#endif

// add a menu
#if 1
			gui::MenuBar* menubar = new gui::MenuBar(compositor);
			{
				gui::Menu* filemenu = new gui::Menu("File", menubar);

				filemenu->add_item("New Project...", MAKE_MEMBER_DELEGATE(void(), EditorKernel, &EditorKernel::on_file_new, this));
				filemenu->add_item("Open Project...", MAKE_MEMBER_DELEGATE(void(), EditorKernel, &EditorKernel::on_file_open, this));
				filemenu->add_separator();
				filemenu->add_item("Quit", MAKE_MEMBER_DELEGATE(void(), EditorKernel, &EditorKernel::on_file_quit, this));
				menubar->add_menu(filemenu);


				gui::Menu* record = new gui::Menu("Sensor", menubar);
				record->add_item("Save New Stream...", MAKE_MEMBER_DELEGATE(void(), EditorKernel, &EditorKernel::on_record_start, this));
				record->add_item("Stop Recording", MAKE_MEMBER_DELEGATE(void(), EditorKernel, &EditorKernel::on_record_stop, this));
				record->add_item("Open Stream...", MAKE_MEMBER_DELEGATE(void(), EditorKernel, &EditorKernel::on_playback_start, this));
				record->add_item("Stop Playback", MAKE_MEMBER_DELEGATE(void(), EditorKernel, &EditorKernel::on_playback_stop, this));
				menubar->add_menu(record);


				gui::Menu* windowmenu = new gui::Menu("Window", menubar);
				windowmenu->add_item("Show Asset Processor", MAKE_MEMBER_DELEGATE(void(), EditorKernel, &EditorKernel::on_window_toggle_asset_processor, this));
				menubar->add_menu(windowmenu);
			}
#endif

#if ENABLE_UI
			gui::RenderableSurface* surface = new gui::RenderableSurface(main_panel);
			surface->on_render_content.bind<EditorKernel, &EditorKernel::render_main_content>(this);

			image::Image checker_pattern;
			checker_pattern.create(512, 512, 3);
			checker_pattern.fill(gemini::Color::from_rgba(0, 25, 25, 255));
			texture = device->create_texture(checker_pattern);

			int handle = resource_cache.track_texture(texture);

			// TODO: sort out this interface!
			render_target = device->create_render_target(texture);
			surface->set_render_target(render_target);
			surface->set_texture_handle(handle);

			center_layout->add_panel(surface);
#endif

#if ENABLE_UI
			gui::Timeline* timeline = new gui::Timeline(main_panel);
			timeline->set_frame_range(0, 30);
			timeline->on_scrubber_changed.bind<EditorKernel, &EditorKernel::timeline_scrubber_changed>(this);
			timeline->set_frame(0);
			timeline->set_maximum_size(gui::Size(0, 60));

			center_layout->add_panel(timeline);
#endif

#if 0
			// TODO: This needs more work as it can still get swamped and
			// latency increases dramatically.
			log_window = new gui::Label(compositor);
			log_window->set_origin(0.0f, 450);
			log_window->set_size(500, 250);
			log_window->set_font("fonts/debug.ttf", 16);
			log_window->set_name("log_window");
			log_window->set_foreground_color(gemini::Color(0.85f, 0.85f, 0.85f));
			log_window->set_background_color(gemini::Color(0.10f, 0.10f, 0.10f));
			uint32_t current_flags = log_window->get_flags();
			log_window->set_flags(gui::Panel::Flag_CursorEnabled | gui::Panel::Flag_CanMove | current_flags);

			// install a log handler
			log_handler.open = log_window_logger_open;
			log_handler.close = log_window_logger_close;
			log_handler.message = log_window_logger_message;
			log_handler.userdata = (void*)log_window;
			core::logging::instance()->add_handler(&log_handler);

			LOGV("log initialized.\n");
#endif


		}
#endif



		kernel::parameters().step_interval_seconds = (1.0f/50.0f);
		int32_t startup_result = net_startup();
		assert(startup_result == 0);

		mocap_device = imocap::device_create();

		return kernel::NoError;
	}



	virtual void tick()
	{
		uint64_t current_time = platform::microseconds();
		platform::update(kernel::parameters().framedelta_milliseconds);

		if (!app_in_focus)
		{
			return;
		}

		static float value = 0.0f;
		static float multiplifer = 1.0f;

		value += 0.01f * multiplifer;
		value = glm::clamp(value, 0.0f, 1.0f);
		if (value == 0.0f || value == 1.0f)
			multiplifer *= -1;


		{
			kernel::Parameters& params = kernel::parameters();
			const float movement_factor = 30.0f;

			if (left_down)
				camera.move_left(movement_factor * params.framedelta_seconds);
			if (right_down)
				camera.move_right(movement_factor * params.framedelta_seconds);
			if (forward_down)
				camera.move_forward(movement_factor * params.framedelta_seconds);
			if (backward_down)
				camera.move_backward(movement_factor * params.framedelta_seconds);

			camera.update_view();
		}


		int32_t yoffset = 130;
		debugdraw::text(20, yoffset, "Left Click + Drag: Rotate Camera", gemini::Color(1.0f, 1.0f, 1.0f));
		debugdraw::text(20, yoffset+16, "WASD: Move Camera", gemini::Color(1.0f, 1.0f, 1.0f));
		debugdraw::text(20, yoffset+32, "Space: Calibrate / Freeze Rotations", gemini::Color(1.0f, 1.0f, 1.0f));

		glm::quat local_rotations[IMOCAP_TOTAL_SENSORS];

		// We need to adjust the coordinate frame from the sensor to the engine.
		local_rotations[0] = imocap::device_sensor_local_orientation(mocap_device, 0);
		local_rotations[1] = glm::inverse(local_rotations[0]) * imocap::device_sensor_local_orientation(mocap_device, 1);
		local_rotations[2] = glm::inverse(local_rotations[1]) * imocap::device_sensor_local_orientation(mocap_device, 2);

		// temp: setup joint offsets
		joint_offets[0] = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
		joint_offets[1] = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.254f));
		joint_offets[2] = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.254f));


		glm::mat4 world_poses[IMOCAP_TOTAL_SENSORS];

		glm::mat4 to_bind_pose[IMOCAP_TOTAL_SENSORS];
		glm::mat4 inverse_bind_pose[IMOCAP_TOTAL_SENSORS];

		glm::vec3 last_origin;

		glm::mat4 parent_pose;

		imocap::mocap_frame_t mocap_frame;

		if (is_playing_frames)
		{
			memcpy(local_rotations, mocap_frames[current_mocap_frame].poses, sizeof(glm::quat) * IMOCAP_TOTAL_SENSORS);
			current_mocap_frame++;

			if (current_mocap_frame > (mocap_frames.size() - 1))
			{
				is_playing_frames = false;
				LOGV("reached end of playback buffer at %i frames.\n", current_mocap_frame);
			}
		}


		for (size_t index = 0; index < IMOCAP_TOTAL_SENSORS; ++index)
		{
			glm::mat4 m = glm::toMat4(local_rotations[index]);
			glm::mat4 local_pose = (joint_offets[index] * m);
			glm::mat4 parent_pose;
			if (index > 0)
			{
				parent_pose = world_poses[(index-1)];
			}

			glm::mat4& world_pose = world_poses[index];

			// convert to
			local_pose = to_bind_pose[index] * local_pose;

			glm::mat4 model_pose = parent_pose * local_pose;


			glm::mat4 final_pose = inverse_bind_pose[index] * model_pose;


			debugdraw::axes(world_pose, 0.1f);

			glm::vec3 origin = glm::vec3(glm::column(world_pose, 3));
			if (index > 0)
			{
				debugdraw::line(last_origin, origin, Color::from_rgba(255, 128, 0, 255));
			}
			last_origin = origin;

			debugdraw::sphere(origin, Color::from_rgba(255, 0, 0, 255), 0.025f);
			//mocap_frame.poses[index] = local_rotations[index];

			const glm::vec3 acceleration = imocap::device_sensor_linear_acceleration(mocap_device, index);
			const glm::vec3 gravity = imocap::device_sensor_gravity(mocap_device, index);


#if DRAW_SENSOR_GRAPHS
			graphs[index]->record_value(acceleration.x, 0);
#endif

			if (index == 2)
			{
				velocity_test += (acceleration /*+ gravity*/);
				position_test += velocity_test;
			}

			debugdraw::basis(origin, acceleration, 1.0f, 0.025f);
		}

		debugdraw::box(glm::vec3(-0.5f, -0.5f, -0.5f) + position_test, glm::vec3(0.5f, 0.5f, 0.5f) + position_test, gemini::Color(0.0f, 1.0f, 1.0f));

		if (is_recording_frames)
		{
			memcpy(mocap_frame.poses, local_rotations, sizeof(glm::quat) * IMOCAP_TOTAL_SENSORS);
			mocap_frame.frame_index = current_mocap_frame;
			current_mocap_frame++;

			mocap_frames.push_back(mocap_frame);
		}
		//debugdraw::axes(glm::mat4(1.0f), 1.0f);

		platform::window::Frame window_frame = platform::window::get_frame(main_window);

		glm::vec3 vertices[] = {
			glm::vec3(-2.0f, 3.0f, 1.0f),
			glm::vec3(-2.5f, 1.25f, 1.0f),
			glm::vec3(2.0f, 3.0f, -1.0f),
			glm::vec3(2.0f, 1.0f, -1.0f)
		};

		OrientedBoundingBox box;
		compute_oriented_bounding_box_by_points(box, vertices, 4);

		//glm::mat3 tr = glm::toMat3(glm::angleAxis(glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
		//debugdraw::oriented_box(tr, glm::vec3(-2.0f, 0.0f, 0.0f), glm::vec3(2.0f, 1.0f, 1.0f), gemini::Color(1.0f, 0.0f, 0.0f));

		debugdraw::oriented_box(box.rotation, box.center, box.positive_extents, gemini::Color(1.0f, 0.0f, 0.0f));
		debugdraw::axes(glm::mat4(box.rotation), 1.0f, 0.0f);

		debugdraw::update(kernel::parameters().framedelta_seconds);

		if (compositor)
		{
			compositor->tick(static_cast<float>(kernel::parameters().step_interval_seconds));
		}

		surface_modelview = glm::mat4(1.0f);
		surface_projection = glm::ortho(0.0f, window_frame.width, window_frame.height, 0.0f, -1.0f, 1.0f);
		//const glm::vec3& p = camera.get_position();
		//LOGV("p: %2.2f, %2.2f, %2.2f\n", p.x, p.y, p.z);

		modelview_matrix = camera.get_modelview();
		projection_matrix = camera.get_projection();



#if 0
		value = 0.35f;
		render2::Pass render_pass;
		render_pass.target = device->default_render_target();
		render_pass.color(value, value, value, 1.0f);
		render_pass.clear_color = true;
		render_pass.clear_depth = true;
		render_pass.depth_test = false;

		render2::CommandQueue* queue = device->create_queue(render_pass);
		render2::CommandSerializer* serializer = device->create_serializer(queue);

		serializer->pipeline(pipeline);
		serializer->vertex_buffer(vertex_buffer);
		serializer->draw(0, 3);
		device->queue_buffers(queue, 1);
		device->destroy_serializer(serializer);
#endif
		platform::window::activate_context(main_window);

		if (compositor)
		{
			compositor->draw();
		}

		/*debugdraw::render(modelview_matrix, projection_matrix, window_frame.width, window_frame.height);*/

		device->submit();

		platform::window::swap_buffers(main_window);

#if defined(GEMINI_ENABLE_PROFILER)
		gemini::profiler::report();
		gemini::profiler::reset();
#endif

		//		glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 2000);

		kernel::Parameters& params = kernel::parameters();
		params.current_frame++;

		// calculate delta ticks in milliseconds
		params.framedelta_milliseconds = (current_time - last_time) * SecondsPerMillisecond;

		// cache the value in seconds
		params.framedelta_seconds = params.framedelta_milliseconds * SecondsPerMillisecond;
		last_time = current_time;
	}


	virtual void shutdown()
	{
		imocap::device_destroy(mocap_device);
		mocap_device = nullptr;

		net_shutdown();
		debugdraw::shutdown();

		// remove the log handler
		core::logging::instance()->remove_handler(&log_handler);

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
		device->destroy_pipeline(surface_pipeline);

		destroy_device(device);

//		glDeleteSync(fence);

		platform::window::destroy(main_window);
		platform::window::shutdown();

		gemini::runtime_shutdown();
	}

};

PLATFORM_MAIN
{
	PLATFORM_IMPLEMENT_PARAMETERS();
	PLATFORM_RETURN(platform::run_application(new EditorKernel()));
}
