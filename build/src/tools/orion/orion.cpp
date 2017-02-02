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
#include <runtime/assets.h>
#include <runtime/runtime.h>
#include <runtime/filesystem.h>
#include <runtime/geometry.h>
#include <runtime/guirenderer.h>
#include <runtime/hotloading.h>
#include <runtime/standaloneresourcecache.h>

#include <platform/input.h>
#include <platform/kernel.h>
#include <platform/platform.h>
#include <platform/network.h>
#include <platform/window.h>
#include <platform/directory_monitor.h>

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

#include <rapid/rapid.h>

#include <renderer/debug_draw.h>
#include <renderer/font_library.h>
#include <renderer/renderer.h>
#include <renderer/scene_renderer.h>

#include <sdk/camera.h>
#include <sdk/game_api.h>
#include <sdk/utils.h>




#include "project.h"

using namespace platform;
using namespace renderer;
using namespace gemini;

#include <runtime/imocap.h>

#define ENABLE_UI 1
#define DRAW_SENSOR_GRAPHS 0

#define DRAW_LINES 0
const size_t TOTAL_LINES = 256;

namespace gui
{
	// This uses a render target to present data
	class RenderableSurface : public gui::Panel
	{
	public:
		RenderableSurface(Panel* parent)
			: Panel(parent)
			, render_device(nullptr)
			, target(nullptr)
			, handle(render::WhiteTexture)
		{
			flags |= Flag_CursorEnabled;
			set_name("RenderableSurface");
		}

		void set_device(render2::Device* device) { render_device = device; }
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

		virtual void set_size(const Size& new_size)
		{
			// resize the render target
			LOGV("resize render target %i %i\n", new_size.width, new_size.height);
			gui::Panel::set_size(new_size);
			//https://blog.nelhage.com/2016/12/how-i-test/
			render_device->resize_render_target(target, new_size.width, new_size.height);
		}

	private:
		render2::Device* render_device;
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


// The quiet period required before an asset change is dispatched to subscribers.
const float ASSET_CHANGE_NOTIFICATION_QUIET_SECONDS = 0.250f;
struct ModifiedAssetData
{
	float quiet_time_left;
	MonitorHandle monitor_handle;
};

typedef HashSet<platform::PathString, ModifiedAssetData> PathDelayHashSet;

size_t total_bytes = sizeof(MyVertex) * 4;


struct EditorEnvironment
{
	Project* project;
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

	render2::Pipeline* surface_pipeline;
	glm::mat4 surface_modelview;
	glm::mat4 surface_projection;

	glm::mat4 joint_offsets[IMOCAP_TOTAL_SENSORS];

	MyVertex vertex_data[4];

	EditorEnvironment environment;

//	GLsync fence;

	RenderScene* render_scene;

	gui::Compositor* compositor;
	gui::Label* log_window;
	GUIRenderer* gui_renderer;
	gui::Panel* main_panel;
	::renderer::StandaloneResourceCache* resource_cache;
	render2::RenderTarget* render_target;
	render2::Texture* texture;
	AssetProcessingPanel* asset_processor;

	gui::Graph* graphs[IMOCAP_TOTAL_SENSORS];
	gui::Label* status;

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

	// development interface
	RapidInterface rapid;
	platform::DynamicLibrary* rapid_library;

	gemini::MonitorHandle monitor_zero;
	gemini::MonitorHandle monitor_one;

	gemini::Allocator asset_allocator;
	MonitorDelegate monitor_delegate;
	gemini::Allocator sensor_allocator;
	gemini::Allocator render_allocator;
	gemini::Allocator debugdraw_allocator;

	glm::vec3* lines;
	size_t current_line_index;

	NotificationServer notify_server;
	NotificationClient notify_client;
	PathDelayHashSet* queued_asset_changes;

public:
	EditorKernel()
		: active(true)
		, compositor(nullptr)
		, gui_renderer(nullptr)
		, resource_cache(nullptr)
		, render_target(nullptr)
		, texture(nullptr)
		, value(0.0f)
		, is_recording_frames(false)
		, is_playing_frames(false)
		, app_in_focus(true)
		, mocap_device(nullptr)
		, rapid_library(nullptr)
		, lines(nullptr)
		, current_line_index(0)
		, mocap_frames(sensor_allocator, 0)
	{
		yaw = 0.0f;
		pitch = 0.0f;

		left_down = right_down = forward_down = backward_down = false;

		last_time = 0;

		should_move_view = false;

		asset_processor = nullptr;

		memset(&environment, 0, sizeof(EditorEnvironment));
	}

	virtual ~EditorKernel()
	{
		if (environment.project)
		{
			delete environment.project;
		}
	}

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

		if (event.is_down)
		{
			if (event.key == BUTTON_SPACE)
			{
				LOGV("freezing rotations\n");
				imocap::zero_rotations(mocap_device);

				position_test = velocity_test = glm::vec3(0.0f, 0.0f, 0.0f);
			}

			if (event.key == BUTTON_F2)
			{
				unload_rapid_interface();

				LOGV("unloaded rapid interface\n");
			}
			else if (event.key == BUTTON_F3)
			{
				load_rapid_interface();
				LOGV("loading rapid interface\n");
			}

			if (event.key == BUTTON_F4)
			{
				LOGV("take a snapshot\n");
				render2::Image image(render_allocator);
				image.create(render_target->width, render_target->height, 4);
				device->render_target_read_pixels(render_target, image);
				image::save_image_to_file(image, "test.png");
			}
		}
	}

	void on_asset_reload(uint32_t channel, void* data, uint32_t data_size)
	{
		platform::PathString path = (const char*)data;
		path.recompute_size();
		assert(data_size == path.size());

		path = path.basename();
		path = path.remove_extension();
		if (channel == 1)
		{
			LOGV("reloading SHADER asset: %s\n", path());
			shader_load(path(), true);
		}
		else if (channel == 2)
		{
			LOGV("reloading TEXTURE asset: %s\n", path());
		}
	}

	void on_file_updated(MonitorHandle monitor_handle, MonitorAction action, const platform::PathString& path)
	{
		if (action == MonitorAction::Modified)
		{
			ModifiedAssetData mod;
			mod.quiet_time_left = ASSET_CHANGE_NOTIFICATION_QUIET_SECONDS;
			mod.monitor_handle = monitor_handle;
			(*queued_asset_changes)[path] = mod;
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
			camera.perspective(60.0f, (int)event.window_width, (int)event.window_height, 0.01f, 1024.0f);
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
		gemini::Allocator allocator = memory_allocator_default(MEMORY_ZONE_DEFAULT);
		Array<PathString> paths(allocator);
		uint32_t flags = platform::OpenDialogFlags::CanChooseDirectories;

		if (platform::show_open_dialog("Choose Game Directory", flags, paths).succeeded())
		{
			LOGV("target path is: %s\n", paths[0]());
		}
	}

	// menu handlers

	void on_file_new(void)
	{
		uint32_t open_flags = platform::OpenDialogFlags::CanChooseDirectories | platform::OpenDialogFlags::CanCreateDirectories;

		gemini::Allocator temp = memory_allocator_default(MEMORY_ZONE_DEFAULT);

		Array<PathString> paths(temp);
		platform::Result result = platform::show_open_dialog("Choose project root", open_flags, paths);
		if (result.succeeded())
		{
			Project test;
			test.set_name("Just a test");

			platform::PathString project_path = paths[0];
			project_path.append(PATH_SEPARATOR_STRING);
			project_path.append("project.conf");

			test.save_project_as(project_path());

			LOGV("saved path: %s\n", paths[0]());
		}
	}


	uint32_t on_test_click(platform::PlatformDialogEvent& event)
	{
		if (event.type == OpenDialogEventType::OkClicked)
		{
			// does a project file exist at path?
			platform::PathString project_path = event.filename;
			project_path.append(PATH_SEPARATOR_STRING);
			project_path.append("project.conf");
			if (!core::filesystem::instance()->file_exists(project_path(), false))
			{
				// TODO: Alert the user that this directory doesn't contain
				// a project file.
				LOGW("file '%s' does not exist\n", project_path());
				return 1;
			}
		}

		return 0;
	}

	void on_file_open()
	{
		uint32_t open_flags = platform::OpenDialogFlags::CanChooseDirectories;

		gemini::Allocator temp = memory_allocator_default(MEMORY_ZONE_DEFAULT);

		Array<PathString> paths(temp);
		platform::open_dialog_event_handler delegate;
		delegate.bind<EditorKernel, &EditorKernel::on_test_click>(this);
		platform::Result result = platform::show_open_dialog("Choose project", open_flags, paths, delegate);
		if (result.succeeded())
		{
			platform::PathString project_path = paths[0];
			project_path.append(PATH_SEPARATOR_STRING);
			project_path.append("project.conf");

			environment.project = Project::open_project(project_path());
		}
	}

	void on_file_save_project()
	{
	}

	void on_file_save_project_as()
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
			Array<PlatformExtensionDescription> extensions(sensor_allocator);
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
		Array<PathString> paths(sensor_allocator);
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
		value = mathlib::PI * 2.0 * (current_frame / 30.0f);

		render_scene->light_position_world.x = cosf(value);
		render_scene->light_position_world.y = 2.0f;
		render_scene->light_position_world.z = sinf(value);
	}

	void render_main_content(render2::RenderTarget* render_target)
	{
#if 0
		render2::Pass render_pass;
		render_pass.color(0.0f, 0.0f, 0.0, 0.0f);
		render_pass.clear_color = false;
		render_pass.clear_depth = false;
		render_pass.depth_test = false;
		render_pass.target = render_target;

		render2::CommandQueue* queue = device->create_queue(render_pass);
		render2::CommandSerializer* serializer = device->create_serializer(queue);

		serializer->pipeline(surface_pipeline);

		const bool test_triangle = 0;
		if (test_triangle)
		{
			serializer->vertex_buffer(vertex_buffer);
			serializer->draw(0, 3);
		}
		device->queue_buffers(queue, 1);
		device->destroy_serializer(serializer);
#endif
		render_scene->camera_position_world = camera.get_position();
		render_scene->camera_view_direction = camera.get_view();
		render_scene_draw(render_scene, device, camera.get_modelview(), camera.get_projection(), render_target);

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
			//camera.set_position(glm::vec3(0.0f, 5.0f, 10.0f));

			// test for rendering mocap suit
			//camera.set_position(glm::vec3(0.69f, 0.55f, 0.45f));
			//camera.set_yaw(-78.15f);
			//camera.set_pitch(31.65f);

			// test for rendering cubes
			camera.set_position(glm::vec3(-2.10f, 1.24f, 1.10f));
			camera.set_yaw(71.70f);
			camera.set_pitch(12.45f);

			camera.set_type(Camera::FIRST_PERSON);
			camera.update_view();
		}

		platform::window::Frame window_frame;

		asset_allocator = memory_allocator_default(MEMORY_ZONE_ASSETS);
		assert(directory_monitor_startup(asset_allocator) == 0);

		queued_asset_changes = MEMORY2_NEW(asset_allocator, PathDelayHashSet)(asset_allocator);

		monitor_delegate.bind<EditorKernel, &EditorKernel::on_file_updated>(this);

		platform::PathString watch_path = core::filesystem::instance()->content_directory();
		watch_path.append(PATH_SEPARATOR_STRING);
		watch_path.append("shaders");
		watch_path.append(PATH_SEPARATOR_STRING);
		watch_path.append("150");

		monitor_zero = directory_monitor_add(watch_path(), monitor_delegate);

		watch_path = core::filesystem::instance()->content_directory();
		watch_path.append(PATH_SEPARATOR_STRING);
		watch_path.append("textures");
		monitor_one = directory_monitor_add(watch_path(), monitor_delegate);

		// initialize the renderer
		{
			render_allocator = memory_allocator_default(MEMORY_ZONE_RENDERER);


			hotloading::startup(render_allocator);

			debugdraw_allocator = memory_allocator_default(MEMORY_ZONE_DEBUGDRAW);

			using namespace render2;
			RenderParameters params(render_allocator);

			// set some options
			params["vsync"] = "true";
			params["double_buffer"] = "true";
			params["gamma_correct"] = "true";
			params["depth_size"] = "24";
//			params["multisample"] = "4";

			// set opengl specific options
			//params["rendering_backend"] = "opengl";


			device = create_device(render_allocator, params);
			assert(device != nullptr);

			assets::startup(device, true);

			window_frame = platform::window::get_frame(main_window);
			device->init(window_frame.width, window_frame.height);

			// setup shaders
			render2::PipelineDescriptor desc;
			desc.shader = shader_load("vertexcolor");
			render2::VertexDescriptor& vertex_format = desc.vertex_description;
			vertex_format.add("in_position", render2::VD_FLOAT, 3);
			vertex_format.add("in_color", render2::VD_FLOAT, 4);
			desc.input_layout = device->create_input_layout(vertex_format, desc.shader);
			pipeline = device->create_pipeline(desc);
			pipeline->constants().set("modelview_matrix", &modelview_matrix);
			pipeline->constants().set("projection_matrix", &projection_matrix);

			{
				render2::PipelineDescriptor desc;
				desc.shader = shader_load("vertexcolor");
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


		render_scene = render_scene_create(render_allocator, device);

		AssetHandle test_mesh = mesh_load("models/cube");

		glm::mat4 transform(1.0f);

		for (size_t index = 0; index < 4; ++index)
		{
			render_scene_add_static_mesh(render_scene, test_mesh, 0, transform);
			transform = glm::translate(transform, glm::vec3(1.5f, 0.0f, 0.0f));
		}

		// initialize debug draw
		debugdraw::startup(debugdraw_allocator, device);

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
			resource_cache = MEMORY2_NEW(render_allocator, StandaloneResourceCache)(render_allocator);

			gui_renderer = MEMORY2_NEW(render_allocator, GUIRenderer)(render_allocator, *resource_cache);
			gui_renderer->set_device(device);

			gui::set_allocator(render_allocator);

			assert(window_frame.width > 0);
			assert(window_frame.height > 0);
			compositor = new gui::Compositor(window_frame.width, window_frame.height, resource_cache, gui_renderer);
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


			const char dev_font[] = "debug";
			const size_t dev_font_size = 16;
#if DRAW_SENSOR_GRAPHS
			// Create a graph for each sensor


			uint32_t origin = 24;

			for (size_t index = 0; index < IMOCAP_TOTAL_SENSORS; ++index)
			{
				gui::Graph* graph = new gui::Graph(compositor);
				graph->set_origin(window_frame.width - 250, origin);
				graph->set_size(250, 100);
				origin += 108;
				graph->set_maximum_size(gui::Sizes(250, 100));
				graph->set_font(dev_font, dev_font_size);
				graph->set_background_color(gemini::Color::from_rgba(60, 60, 60, 255));
				graph->set_foreground_color(gemini::Color::from_rgba(255, 255, 255, 255));
				graph->create_samples(100, 3);
				graph->configure_channel(0, gemini::Color::from_rgba(255, 0, 0, 255));
				graph->configure_channel(1, gemini::Color::from_rgba(0, 255, 0, 255));
				graph->configure_channel(2, gemini::Color::from_rgba(0, 0, 255, 255));
				graph->set_range(-0.5f, 0.5f);
				graph->enable_baseline(true, 0.0f, gemini::Color::from_rgba(64, 64, 64, 255));

				graphs[index] = graph;
			}
#endif

			status = new gui::Label(compositor);
			status->set_origin(10, 100);
			status->set_size(150, 75);
			status->set_font(dev_font, dev_font_size);
			status->set_text("");
			status->set_foreground_color(gemini::Color(1.0f, 1.0f, 1.0f));
			status->set_background_color(gemini::Color(0.0f, 0.0f, 0.0f, 0.0f));

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
				filemenu->add_item("Save Project", MAKE_MEMBER_DELEGATE(void(), EditorKernel, &EditorKernel::on_file_save_project, this));
				filemenu->add_item("Save Project As...", MAKE_MEMBER_DELEGATE(void(), EditorKernel, &EditorKernel::on_file_save_project_as, this));
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

			image::Image checker_pattern(render_allocator);
			checker_pattern.create(512, 512, 3);
			checker_pattern.fill(gemini::Color::from_rgba(0, 25, 25, 255));
			texture = device->create_texture(checker_pattern);

			int handle = resource_cache->track_texture(texture);

			// TODO: sort out this interface!
			render_target = device->create_render_target(texture);
			surface->set_device(device);
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
			log_window->set_font("debug", 16);
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

		sensor_allocator = memory_allocator_default(MEMORY_ZONE_DEFAULT);
		imocap::startup(sensor_allocator);

		mocap_device = imocap::device_create();

		load_rapid_interface();


		notify_server_create(&notify_server);
		notify_client_create(&notify_client);

		NotifyMessageDelegate channel_delegate;
		channel_delegate.bind<EditorKernel, &EditorKernel::on_asset_reload>(this);
		notify_client_subscribe(&notify_client, 1, channel_delegate);
		notify_client_subscribe(&notify_client, 2, channel_delegate);


		lines = new glm::vec3[TOTAL_LINES];

		return kernel::NoError;
	}

	void load_rapid_interface()
	{
		if (rapid_library)
		{
			unload_rapid_interface();
		}

		PathString program_directory = platform::get_program_directory();
		PathString lib_directory = program_directory;
		core::str::directory_up(&lib_directory[0]);
		core::str::directory_up(&lib_directory[0]);
		lib_directory.append(PATH_SEPARATOR_STRING);
		lib_directory.append("lib");
		lib_directory.append(PATH_SEPARATOR_STRING);
		lib_directory.append("debug_x86_64");
		lib_directory.append(PATH_SEPARATOR_STRING);
#if defined(PLATFORM_LINUX)
		lib_directory.append("lib");
#endif
		lib_directory.append("rapid");
		lib_directory.append(platform::dylib_extension());

		LOGV("rapid lib = %s\n", lib_directory());
		rapid_library = platform::dylib_open(lib_directory());
		assert(rapid_library);

		populate_interface_fn pif = reinterpret_cast<populate_interface_fn>(platform::dylib_find(rapid_library, "populate_interface"));
		assert(pif);

		pif(rapid);
		status->set_text("PLUGIN LOADED");
	}

	void unload_rapid_interface()
	{
		if (rapid_library)
		{
			memset(&rapid, 0, sizeof(RapidInterface));
			platform::dylib_close(rapid_library);
			rapid_library = nullptr;
			status->set_text("PLUGIN NOT LOADED");
		}
	}

	void tick_queued_asset_changes(PathDelayHashSet& hashset, float tick_seconds)
	{
		uint32_t handle_to_channel[] = {
			1,
			2
		};
		PathDelayHashSet::Iterator it = hashset.begin();
		for (; it != hashset.end(); ++it)
		{
			ModifiedAssetData& data = it.value();
			assert(data.monitor_handle > 0 && data.monitor_handle < 3);

			data.quiet_time_left -= tick_seconds;
			if (data.quiet_time_left <= 0.0f)
			{
				NotifyMessage message;
				platform::PathString asset_path = it.key();
				notify_message_string(message, asset_path);
				message.channel = handle_to_channel[data.monitor_handle - 1];
				notify_server_publish(&notify_server, &message);

				it = hashset.remove(it);
				continue;
			}
		}
	} // tick_queued_asset_changes

	virtual void tick()
	{
		uint64_t current_time = platform::microseconds();
		platform::update(kernel::parameters().framedelta_milliseconds);

		// while i debug network stuff; don't do this...
		//if (!app_in_focus)
		//{
		//	return;
		//}

		hotloading::tick();
		directory_monitor_update();
		notify_server_tick(&notify_server);
		notify_client_tick(&notify_client);
		tick_queued_asset_changes(*queued_asset_changes, kernel::parameters().framedelta_seconds);

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
		debugdraw::text(20, yoffset+48, core::str::format("Camera: %2.2f, %2.2f, %2.2f [%2.2f, %2.2f]", camera.get_position().x, camera.get_position().y, camera.get_position().z, camera.get_yaw(), camera.get_pitch()), gemini::Color(1.0f, 1.0f, 1.0f));

		glm::quat local_rotations[IMOCAP_TOTAL_SENSORS];

		// We need to adjust the coordinate frame from the sensor to the engine.
		local_rotations[0] = imocap::device_sensor_local_orientation(mocap_device, 0);
		local_rotations[1] = imocap::device_sensor_local_orientation(mocap_device, 1);
		local_rotations[2] = imocap::device_sensor_local_orientation(mocap_device, 2);

		// It is incorrect to cancel out the parent rotation entirely for children.
		// This results in the child not inheriting the parent's coordinate space.

		// For child rotations, we need to cancel out the offset parent rotation
		// from the local child rotation.

		// temp: setup joint offsets
		joint_offsets[0] = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
		joint_offsets[1] = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.254f));
		joint_offsets[2] = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.254f));


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
			glm::mat4& world_pose = world_poses[index];
			if (rapid_library && rapid.compute_pose)
			{
				rapid.compute_pose(world_pose, world_poses, local_rotations, joint_offsets, index);
			}

			debugdraw::axes(world_pose, 0.1f);

			glm::vec3 origin = glm::vec3(glm::column(world_pose, 3));
			if (index > 0)
			{
				debugdraw::line(last_origin, origin, Color::from_rgba(255, 128, 0, 255));
			}
			last_origin = origin;

			debugdraw::sphere(origin, Color::from_rgba(255, 0, 0, 255), 0.025f);

			const glm::vec3 acceleration = imocap::device_sensor_local_acceleration(mocap_device, index);


#if DRAW_SENSOR_GRAPHS
			graphs[index]->record_value(acceleration.x, 0);
			graphs[index]->record_value(acceleration.y, 1);
			graphs[index]->record_value(acceleration.z, 2);
#endif

			if (index == 2)
			{
				velocity_test += acceleration * (float)kernel::parameters().step_interval_seconds;
				position_test += velocity_test;

				assert(current_line_index < TOTAL_LINES);
				glm::vec3* line0 = &lines[current_line_index++];
				*line0 = origin;

				current_line_index = current_line_index % TOTAL_LINES;
			}

			//debugdraw::basis(origin, acceleration, 1.0f, 0.025f);
		}

#if DRAW_LINES
		// draw all lines
		glm::vec3 last_line = lines[0];
		for (size_t index = 0; index < TOTAL_LINES / 2; index += 2)
		{
			debugdraw::line(last_line, lines[index * 2 + 1], gemini::Color(1.0f, 1.0f, 1.0f));

			assert((index * 2 + 1) < TOTAL_LINES);
			last_line = lines[index * 2 + 1];
		}
#endif

#if DRAW_SENSOR_GRAPHS
		debugdraw::box(glm::vec3(-0.5f, -0.5f, -0.5f) + position_test, glm::vec3(0.5f, 0.5f, 0.5f) + position_test, gemini::Color(0.0f, 1.0f, 1.0f));
#endif

		if (is_recording_frames)
		{
			memcpy(mocap_frame.poses, local_rotations, sizeof(glm::quat) * IMOCAP_TOTAL_SENSORS);
			mocap_frame.frame_index = current_mocap_frame;
			current_mocap_frame++;

			mocap_frames.push_back(mocap_frame);
		}
		//debugdraw::axes(glm::mat4(1.0f), 1.0f);

		platform::window::Frame window_frame = platform::window::get_frame(main_window);

#if 0 // draw obb
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
#endif
		//static float the_time = 0.0f;
		//render_scene->light_position_world.x = cosf(the_time);
		//render_scene->light_position_world.y = 2.0f;
		//render_scene->light_position_world.z = sinf(the_time);

		//the_time += 0.01f;

		// draw the position of the light
		debugdraw::sphere(render_scene->light_position_world, Color(1.0f, 1.0f, 1.0f), 0.5f, 0.0f);

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
		render_pass.depth_write = true;

		render2::CommandQueue* queue = device->create_queue(render_pass);
		render2::CommandSerializer* serializer = device->create_serializer(queue);

		serializer->pipeline(surface_pipeline);
		serializer->vertex_buffer(vertex_buffer);
		serializer->draw(0, 3);
		device->queue_buffers(queue, 1);
		device->destroy_serializer(serializer);
#endif
		platform::window::activate_context(main_window);

		//render_scene_draw(render_scene, device, modelview_matrix, projection_matrix);

		//debugdraw::render(camera.get_modelview(), camera.get_projection(), render_target->width, render_target->height, device->default_render_target());

		if (compositor)
		{
			compositor->draw();
		}

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

		//memory_leak_report(false);
	}


	virtual void shutdown()
	{
		delete [] lines;
		lines = nullptr;
		unload_rapid_interface();

		imocap::device_destroy(mocap_device);
		mocap_device = nullptr;
		imocap::shutdown();

		directory_monitor_shutdown();

		notify_client_destroy(&notify_client);
		notify_server_destroy(&notify_server);

		MEMORY2_DELETE(asset_allocator, queued_asset_changes);
		queued_asset_changes = nullptr;

		net_shutdown();
		debugdraw::shutdown();

		// remove the log handler
		core::logging::instance()->remove_handler(&log_handler);

		device->destroy_render_target(render_target);
		device->destroy_texture(texture);

		// compositor will cleanup children
		delete compositor;

		// shutdown the gui
		MEMORY2_DELETE(render_allocator, gui_renderer);

		// explicitly clear the resource cache or else the allocator will
		// detect leaks.
		resource_cache->clear();
		MEMORY2_DELETE(render_allocator, resource_cache);

		assets::shutdown();

		render_scene_destroy(render_scene, device);

		// shutdown the render device
		device->destroy_buffer(vertex_buffer);

		device->destroy_pipeline(pipeline);
		device->destroy_pipeline(surface_pipeline);

		destroy_device(render_allocator, device);

//		glDeleteSync(fence);

		hotloading::shutdown();

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
