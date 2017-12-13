#include <core/logging.h>
#include <core/typedefs.h>

#include <renderer/renderer.h>
#include <renderer/debug_draw.h>

#include <platform/platform.h>
#include <platform/kernel.h>


#include <sdk/camera.h>

#include <runtime/animation.h>
#include <runtime/assets.h>
#include <runtime/debug_event.h>
#include <runtime/runtime.h>
#include <runtime/filesystem.h>
#include <runtime/geometry.h>
#include <runtime/guirenderer.h>
#include <runtime/hotloading.h>
#include <runtime/standaloneresourcecache.h>
#include <runtime/inputstate.h>

#include <runtime/mesh.h>
#include <runtime/transform_graph.h>

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
#include <ui/plot.h>
#include <ui/label.h>
#include <ui/layout.h>
#include <ui/menu.h>
#include <ui/timeline.h>
#include <ui/ui.h>
#include <ui/render_panel.h>
#include <ui/gamepad_panel.h>

#include <rapid/rapid.h>

#include <renderer/debug_draw.h>
#include <renderer/font_library.h>
#include <renderer/renderer.h>
#include <renderer/scene_renderer.h>

#include <sdk/camera.h>
#include <sdk/game_api.h>
#include <sdk/utils.h>

#include <ui/gamepad_panel.h>

using namespace platform;
using namespace renderer;
using namespace gemini;


class ProtoVizKernel : public kernel::IKernel
{
private:
	platform::window::NativeWindow* main_window;

	render2::Device* device;
	render2::Buffer* vertex_buffer;

	RenderScene* render_scene;
	TransformNode* transform_graph;

	gui::Compositor* compositor;
	GUIRenderer* gui_renderer;
	gui::Panel* main_panel;
	gui::GamepadPanel* gamepad_panel;

	::renderer::StandaloneResourceCache* resource_cache;
	render2::RenderTarget* render_target;
	render2::Texture* texture;

	InputEventRelay* event_relay;
	InputState inputstate;

	//gui::Graph* graphs[IMOCAP_TOTAL_SENSORS];
	//gui::Label* status;

	//Array<imocap::mocap_frame_t> mocap_frames;
	//size_t current_mocap_frame;
	//PathString current_mocap_filename;

	//telemetry_viewer tel_viewer;

	//float value;

	//core::logging::Handler log_handler;

	//EditableMesh single_mesh;

	Camera camera;

	//float yaw;
	//float pitch;

	//bool left_down;
	//bool right_down;
	//bool forward_down;
	//bool backward_down;

	//bool should_move_view;

	//bool is_recording_frames;
	//bool is_playing_frames;
	//bool app_in_focus;

	//glm::vec3 position_test;
	//glm::vec3 velocity_test;

	//imocap::MocapDevice* mocap_device;

	//gemini::MonitorHandle monitor_handles[5];

	gemini::Allocator asset_allocator;
	//MonitorDelegate monitor_delegate;
	//gemini::Allocator sensor_allocator;
	gemini::Allocator render_allocator;
	gemini::Allocator debugdraw_allocator;
	gemini::Allocator default_allocator;

	//Array<core::StackString<32>> mesh_animations;
	//uint32_t current_mesh_animation;
	//uint32_t animated_mesh;
	//uint32_t second_mesh;
	//uint32_t attachment_mesh;
	//uint32_t enable_animation;
	//uint32_t mesh_animation_index;


	//NotificationServer notify_server;
	//NotificationClient notify_client;
	//PathDelayHashSet* queued_asset_changes;

	//EntityRenderState entity_render_state;

public:
	ProtoVizKernel()
		: compositor(nullptr)
		, gui_renderer(nullptr)
		, resource_cache(nullptr)
		, render_target(nullptr)
		, event_relay(nullptr)
	{
	}

	virtual ~ProtoVizKernel()
	{
	}

	virtual void event(kernel::KeyboardEvent& event)
	{
		event_relay->queue(event, kernel::parameters().current_physics_tick);
#if 0
		if (compositor && compositor->get_focus())
		{
			if (event.is_text)
			{
				compositor->text_event(event.unicode);
			}
			else
			{
				compositor->key_event(event.is_down, event.key, 0);
			}
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
				runtime_unload_rapid();
				LOGV("unloaded rapid interface\n");

			}
			else if (event.key == BUTTON_F3)
			{
				runtime_load_rapid();
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

			if (event.key == BUTTON_F5)
			{
				++current_mesh_animation;
				if (current_mesh_animation >= mesh_animations.size())
				{
					current_mesh_animation = 0;
				}

				LOGV("Playing animation: \"%s\"\n", mesh_animations[current_mesh_animation]());
				mesh_animation_index = render_scene_animation_play(render_scene, animated_mesh, mesh_animations[current_mesh_animation](), 0);
				render_scene_animation_play(render_scene, animated_mesh, "look_right", 1);

				//render_scene_animation_play(render_scene, second_mesh, "idle", 0);

				update_timeline_frames();
			}
			else if (event.key == BUTTON_SPACE)
			{
				enable_animation = !enable_animation;
				LOGV("Animation is now %s\n", enable_animation ? "ON" : "OFF");
			}
			else if (event.key == BUTTON_MINUS)
			{
				kernel::parameters().simulation_time_scale -= 0.1f;
				kernel::parameters().simulation_time_scale = glm::clamp(kernel::parameters().simulation_time_scale, 0.0f, 3.0f);
				LOGV("set timescale to %2.2f\n", kernel::parameters().simulation_time_scale);
			}
			else if (event.key == BUTTON_PLUS)
			{
				kernel::parameters().simulation_time_scale += 0.1f;
				kernel::parameters().simulation_time_scale = glm::clamp(kernel::parameters().simulation_time_scale, 0.0f, 3.0f);
				LOGV("set timescale to %2.2f\n", kernel::parameters().simulation_time_scale);
			}
		}
#endif
	}


	virtual void event(kernel::SystemEvent& event)
	{
		if (event.subtype == kernel::WindowResized)
		{
			assert(device);
			device->backbuffer_resized(event.render_width, event.render_height);

			assert(compositor);
			compositor->resize(event.window_width, event.window_height);

			main_panel->set_size(event.window_width, event.window_height - 24);
			camera.perspective(60.0f, (int)event.window_width, (int)event.window_height, 0.01f, 1024.0f);
		}
		else if (event.subtype == kernel::WindowClosed)
		{
			set_active(false);
		}

		if (event_relay)
		{
			event_relay->queue(event, kernel::parameters().current_physics_tick);
		}
	}

	virtual void event(kernel::MouseEvent& event)
	{
		event_relay->queue(event, kernel::parameters().current_physics_tick);
#if 0
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
				bool handled = compositor->cursor_button(input_to_gui[event.button], event.is_down);
				if (!handled)
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
				}
			}
			else if (event.subtype == kernel::MouseWheelMoved)
			{
				bool handled = false; // compositor->cursor_scroll(event.wheel_direction);
				if (!handled)
				{
					glm::vec3 offset = camera.get_target_offset();
					offset.z += event.wheel_direction * -0.5f;
					offset.z = glm::clamp(offset.z, 0.25f, 10.0f);
					camera.set_target_offset(glm::vec3(offset.x, offset.y, offset.z));
				}
			}
		}
#endif
	}

	virtual void event(kernel::GameControllerEvent& event)
	{
		event_relay->queue(event, kernel::parameters().current_physics_tick);
	}

	int32_t handle_input_message(const InputMessage& message)
	{
		//if (!handled && should_move_view)
		//{
		//	if (event.dx != 0 || event.dy != 0)
		//	{
		//		const float sensitivity = .10f;
		//		camera.move_view(event.dx, event.dy);
		//	}
		//}
		input_message_to_inputstate(message, inputstate);

		return 0;
	}

	void on_view_reset(void)
	{
		camera.set_yaw(52.35f);
		camera.set_pitch(48.90f);
		camera.set_position(glm::vec3(0.0f, 0.0f, 0.0f));
		//camera.set_type(Camera::FIRST_PERSON);
		camera.set_type(Camera::THIRD_PERSON);
		camera.set_target_offset(glm::vec3(0.0f, 1.0f, 5.0f));
		camera.update_view();
	}


	void render_main_content(render2::RenderTarget* render_target)
	{
		render_scene->camera_position_world = camera.get_position();
		render_scene->camera_view_direction = camera.get_view();
		render_scene_draw(render_scene, device, camera.get_modelview(), camera.get_projection(), render_target);

#if 0
		// We need to put the grid in its own scene node.
		// draw the grid
		render2::Pass render_pass;
		render_pass.color(0.0f, 0.0f, 0.0, 0.0f);
		render_pass.clear_color = false;
		render_pass.clear_depth = false;
		render_pass.depth_test = true;
		render_pass.target = render_target;

		render2::CommandQueue* queue = device->create_queue(render_pass);
		render2::CommandSerializer* serializer = device->create_serializer(queue);

		serializer->pipeline(line_pipeline);

		serializer->vertex_buffer(vertex_buffer);
		serializer->draw(0, total_grid_lines);
		device->queue_buffers(queue, 1);
		device->destroy_serializer(serializer);
#endif
		debugdraw::render(camera.get_modelview(), camera.get_projection(), render_target->width, render_target->height, render_target);
	}

	virtual kernel::Error startup()
	{
		gemini::Allocator allocator = gemini::memory_allocator_default(gemini::MEMORY_ZONE_DEFAULT);
		ArgumentParser parser;
		argparse_create(allocator, &parser);

		gemini::string asset_path;
		argparse_arg(&parser, "The path to load content from", "--assets", nullptr, &asset_path);

		Array<gemini::string> arguments(allocator);
		runtime_load_arguments(allocator, arguments);
		if (argparse_parse(&parser, arguments) != 0)
		{
			argparse_destroy(allocator, &parser);
			runtime_destroy_arguments(allocator, arguments);
			return kernel::CoreFailed;
		}

		default_allocator = memory_allocator_default(MEMORY_ZONE_DEFAULT);

		event_relay = MEMORY2_NEW(default_allocator, InputEventRelay)(default_allocator);
		event_relay->add_handler(MAKE_MEMBER_DELEGATE(int32_t(const InputMessage&), ProtoVizKernel, &ProtoVizKernel::handle_input_message, this));

		gemini::runtime_startup("arcfusion.net/protoviz", platform::make_absolute_path(asset_path.c_str()));

		argparse_destroy(allocator, &parser);
		runtime_destroy_arguments(allocator, arguments);


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
			camera.set_position(glm::vec3(0.0f, 0.0f, 0.0f));
			//camera.set_position(glm::vec3(0.0f, 5.0f, 10.0f));

			// test for rendering mocap suit
			//camera.set_position(glm::vec3(0.69f, 0.55f, 0.45f));
			//camera.set_yaw(-78.15f);
			//camera.set_pitch(31.65f);

			// test for rendering cubes
			//camera.set_position(glm::vec3(-2.10f, 1.24f, 1.10f));
			//camera.set_yaw(71.70f);
			//camera.set_pitch(12.45f);

			// test for rendering cube_rig2
			//camera.set_position(glm::vec3(-1.51, 3.67f, 1.21f));
			camera.set_yaw(52.35f);
			camera.set_pitch(48.90f);

			//camera.set_type(Camera::FIRST_PERSON);

			camera.set_type(Camera::THIRD_PERSON);
			camera.set_target_offset(glm::vec3(0.0f, 1.0f, 5.0f));

			camera.update_view();
		}

		platform::window::Frame window_frame;

		asset_allocator = memory_allocator_default(MEMORY_ZONE_ASSETS);
		//assert(directory_monitor_startup(asset_allocator) == 0);

		int32_t startup_result = net_startup();
		assert(startup_result == 0);

		// initialize the renderer
		{
			render_allocator = memory_allocator_default(MEMORY_ZONE_RENDERER);
			debugdraw_allocator = memory_allocator_default(MEMORY_ZONE_DEBUGDRAW);

			using namespace render2;
			RenderParameters params(render_allocator);

			// set some options
			params["vsync"] = "true";
			params["double_buffer"] = "true";
			params["gamma_correct"] = "true";
			params["depth_size"] = "24";

			device = create_device(render_allocator, params);
			assert(device != nullptr);

			assets::startup(device, true);
			render_scene_startup(device, render_allocator);

			window_frame = platform::window::get_frame(main_window);
			device->init(window_frame.width, window_frame.height);
		}

		// setup editor assets / content paths
		{
			//			fs->add_virtual_path("editor/assets");
		}


		render_scene = render_scene_create(render_allocator, device);
		transform_graph = transform_graph_create_node(render_allocator, "root");

		animation::startup(asset_allocator);

		// initialize debug draw
		debugdraw::startup(debugdraw_allocator, device);

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

			const float MENU_BAR_SIZE = 0;
			main_panel = new gui::Panel(compositor);
			main_panel->set_name("main_panel");
			main_panel->set_origin(0, MENU_BAR_SIZE);
			main_panel->set_size(window_frame.width, window_frame.height - MENU_BAR_SIZE);


			gui::HorizontalLayout* horizontal_layout = new gui::HorizontalLayout();
			gui::VerticalLayout* center_layout = new gui::VerticalLayout();
			main_panel->set_layout(horizontal_layout);
			horizontal_layout->add_layout(center_layout);

			const char dev_font[] = "debug";
			const size_t dev_font_size = 16;

			gui::RenderableSurface* surface = new gui::RenderableSurface(main_panel);
			surface->on_render_content.bind<ProtoVizKernel, &ProtoVizKernel::render_main_content>(this);

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

			gamepad_panel = new gui::GamepadPanel(compositor);
			gamepad_panel->set_size(600, 350);
			gamepad_panel->set_origin(0, 0);
			gamepad_panel->set_name("gamepad panel");
			gamepad_panel->set_background_color(gemini::Color(0.3f, 0.3f, 0.3f));
		}



		kernel::parameters().step_interval_seconds = (1.0f / 50.0f);

		return kernel::NoError;
	}

	virtual void fixed_update(float step_seconds)
	{
		event_relay->dispatch(kernel::parameters().current_physics_tick);

		if (inputstate.keyboard().get_key(BUTTON_ESCAPE).is_down())
		{
			set_active(false);
		}

		if (inputstate.keyboard().get_key(BUTTON_SPACE).was_released())
		{
			LOGV("press space\n");
		}
		else if (inputstate.mouse().get_button(MOUSE_LEFT).was_pressed())
		{
			LOGV("press left\n");
		}
		else if (inputstate.mouse().get_button(MOUSE_LEFT).is_down())
		{
			LOGV("left is down\n");
		}
		else if (inputstate.mouse().get_button(MOUSE_LEFT).was_released())
		{
			LOGV("left released\n");
		}

		gamepad_panel->set_from_joystick(inputstate.joystick_by_index(0));

		inputstate.mouse().reset_delta();
	}

	virtual void tick(bool performed_fixed_update)
	{
		uint64_t current_time = platform::microseconds();

		// while i debug network stuff; don't do this...
		//if (!app_in_focus)
		//{
		//	return;
		//}

		//if (enable_animation && animated_mesh != 0)
		//{
		//	// TODO: update the timeline
		//	uint32_t current_frame = render_scene_animation_current_frame(render_scene, animated_mesh, 0);
		//	if (timeline)
		//	{
		//		timeline->set_frame(current_frame);
		//	}

		//	animation::update(kernel::parameters().simulation_delta_seconds);
		//}


		{
			// Copy frame state for each entity into their respective transform nodes.
			//transform_graph_copy_frame_state(transform_graph, &frame_state);

			//transform_graph_print(transform_graph);


			// Update transform nodes with current animation poses
			animation_update_transform_nodes();

			// get the latest world matrices from the transform graph
			glm::mat4 world_matrices[256];
			transform_graph_transform(transform_graph, world_matrices);

			// copy transforms from nodes to animated components
			animation_update_components();

			// update world matrices for scene rendering
			render_scene_update(render_scene, world_matrices);
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

			//if (left_down)
			//	camera.move_left(movement_factor * params.framedelta_seconds);
			//if (right_down)
			//	camera.move_right(movement_factor * params.framedelta_seconds);
			//if (forward_down)
			//	camera.move_forward(movement_factor * params.framedelta_seconds);
			//if (backward_down)
			//	camera.move_backward(movement_factor * params.framedelta_seconds);

			camera.update_view();
		}


		int32_t yoffset = 130;
		debugdraw::text(20, yoffset, "Left Click + Drag: Rotate Camera", gemini::Color(1.0f, 1.0f, 1.0f));
		debugdraw::text(20, yoffset + 16, "WASD: Move Camera", gemini::Color(1.0f, 1.0f, 1.0f));
		debugdraw::text(20, yoffset + 32, "Space: Calibrate / Freeze Rotations", gemini::Color(1.0f, 1.0f, 1.0f));
		debugdraw::text(20, yoffset + 48, core::str::format("Camera: %2.2f, %2.2f, %2.2f [%2.2f, %2.2f]", camera.get_position().x, camera.get_position().y, camera.get_position().z, camera.get_yaw(), camera.get_pitch()), gemini::Color(1.0f, 1.0f, 1.0f));
		//debugdraw::text(20, yoffset + 64, core::str::format("debug bytes/sec: %i\n", tel_viewer.bytes_per_second), gemini::Color(1.0f, 1.0f, 1.0f));

		platform::window::Frame window_frame = platform::window::get_frame(main_window);

		static float the_time = 0.0f;
		render_scene->light_position_world.x = cosf(the_time);
		render_scene->light_position_world.y = 2.0f;
		render_scene->light_position_world.z = sinf(the_time);
		the_time += 0.01f;

		// draw the position of the light
		debugdraw::sphere(render_scene->light_position_world, Color(1.0f, 1.0f, 1.0f), 0.5f, 0.0f);

		debugdraw::update(kernel::parameters().framedelta_seconds);

		debugdraw::axes(glm::mat4(1.0f), 1.0f);

		if (compositor)
		{
			compositor->tick(static_cast<float>(kernel::parameters().step_interval_seconds));
		}

		//surface_modelview = glm::mat4(1.0f);
		//surface_projection = glm::ortho(0.0f, window_frame.width, window_frame.height, 0.0f, -1.0f, 1.0f);

		//modelview_matrix = camera.get_modelview();
		//projection_matrix = camera.get_projection();

		platform::window::activate_context(main_window);

		if (compositor)
		{
			compositor->draw();
		}

		device->submit();
		platform::window::swap_buffers(main_window);

		//		glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 2000);

		kernel::Parameters& params = kernel::parameters();
		params.current_frame++;

		// calculate delta ticks in milliseconds
		params.framedelta_milliseconds = (current_time - platform::microseconds()) * MillisecondsPerMicrosecond;

		params.simulation_delta_seconds = (params.framedelta_milliseconds * params.simulation_time_scale) * SecondsPerMillisecond;

		// cache the value in seconds
		params.framedelta_seconds = params.framedelta_milliseconds * SecondsPerMillisecond;

		if (performed_fixed_update)
		{
			inputstate.update();
		}
	}


	virtual void shutdown()
	{
		// must be shut down before the animations; as they're referenced.
		transform_graph_destroy_node(render_allocator, transform_graph);
		render_scene_destroy(render_scene, device);
		render_scene_shutdown();

		animation::shutdown();


		net_shutdown();
		debugdraw::shutdown();

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

		MEMORY2_DELETE(default_allocator, event_relay);
		event_relay = nullptr;

		assets::shutdown();

		destroy_device(render_allocator, device);

		platform::window::destroy(main_window);
		platform::window::shutdown();

		gemini::runtime_shutdown();
	}

}; // ProtoVizKernel



PLATFORM_MAIN
{
	PLATFORM_IMPLEMENT_PARAMETERS();
	PLATFORM_RETURN(platform::run_application(new ProtoVizKernel()));
}
