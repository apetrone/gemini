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

#include <core/logging.h>

#include <runtime/filesystem.h>
#include <runtime/runtime.h>
#include <runtime/guirenderer.h>
#include <runtime/standaloneresourcecache.h>

#include <platform/platform.h>
#include <platform/window.h>
#include <platform/kernel.h>
#include <platform/input.h>

#include <renderer/renderer.h>
#include <renderer/vertexbuffer.h>
#include <renderer/vertexstream.h>
#include <renderer/font.h>

#include <ui/ui.h>
#include <ui/compositor.h>
#include <ui/graph.h>
#include <ui/button.h>
#include <ui/slider.h>
#include <ui/label.h>
#include <ui/tabcontrol.h>

#include <core/threadsafequeue.h>


#include <assert.h>

using namespace renderer;

// ---------------------------------------------------------------------
// gui
// ---------------------------------------------------------------------
static void* gui_malloc_callback(size_t size)
{
	return core::memory::global_allocator().allocate(size, sizeof(void*), __FILE__, __LINE__);
}

static void gui_free_callback(void* pointer)
{
	core::memory::global_allocator().deallocate(pointer);
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





// ---------------------------------------------------------------------
// TestUi
// ---------------------------------------------------------------------
class TestUi : public kernel::IKernel,
	public kernel::IEventListener<kernel::KeyboardEvent>,
	public kernel::IEventListener<kernel::MouseEvent>,
	public kernel::IEventListener<kernel::SystemEvent>
{
	bool active;
	platform::window::NativeWindow* native_window;
	gui::Compositor* compositor;
	gui::Graph* graph;
	gui::Label* label;
	gui::Slider* slider;
	gui::Label* slider_label;
	GUIRenderer renderer;
	StandaloneResourceCache resource_cache;

	glm::mat4 modelview_matrix;
	glm::mat4 projection_matrix;

public:
	virtual void event(kernel::KeyboardEvent& event)
	{
		if (event.is_down)
		{
			if (event.key == input::BUTTON_ESCAPE)
			{
				kernel::instance()->set_active(false);
			}
		}
	}

	virtual void event(kernel::MouseEvent& event)
	{
		if (event.subtype == kernel::MouseMoved)
		{
			compositor->cursor_move_absolute(event.mx, event.my);
		}
		else if (event.subtype == kernel::MouseButton)
		{
			gui::CursorButton::Type input_to_gui[] = {
				gui::CursorButton::None,
				gui::CursorButton::Left,
				gui::CursorButton::Right,
				gui::CursorButton::Middle,
				gui::CursorButton::Mouse4,
				gui::CursorButton::Mouse5
			};

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
	}

	virtual void event(kernel::SystemEvent& event)
	{
		switch(event.subtype)
		{
			case kernel::WindowResized:
			{
				LOGV("window resized: %i x %i\n", event.render_width, event.render_height);
				if (device)
				{
					device->backbuffer_resized(event.render_width, event.render_height);
				}

				compositor->resize(event.render_width, event.render_height);
				break;
			}
			case kernel::WindowClosed:
			{
				LOGV("Window was closed!\n");
				set_active(false);
				break;
			}

			default:
				break;
		}
	}

public:

	TestUi() :
		renderer(resource_cache)
	{
		native_window = nullptr;
		active = true;
		graph = nullptr;
	}

	virtual bool is_active() const { return active; }
	virtual void set_active(bool isactive) { active = isactive; }

	void slider_value_changed(float new_value)
	{
		if (slider_label)
		{
			slider_label->set_text(
				core::str::format("%2.2f", new_value)
			);
		}
	}

	void test_button_clicked(gui::EventArgs& args)
	{
		fprintf(stdout, "test_button_clicked\n");
	}

	void setup_gui(int width, int height)
	{
		renderer.set_device(device);

		gui::set_allocator(gui_malloc_callback, gui_free_callback);
		compositor = new gui::Compositor(width, height, &resource_cache, &renderer);
		compositor->set_name("main_compositor");

		platform::window::Frame frame = platform::window::get_render_frame(native_window);

		// Window frame is invalid!
		assert(frame.width > 0);
		assert(frame.height > 0);

		const char dev_font[] = "fonts/debug.ttf";
		const char menu_font[] = "fonts/debug.ttf";

		const size_t dev_font_size = 16;
		const size_t menu_font_size = 48;

		// setup the framerate graph
#if 1
		graph = new gui::Graph(compositor);
		graph->set_origin(width - 250, 0);
		graph->set_dimensions(graph->dimensions_from_pixels(gui::Point(250, 100)));
		graph->set_font(dev_font, dev_font_size);
		graph->set_background_color(gemini::Color::from_rgba(60, 60, 60, 255));
		graph->set_foreground_color(gemini::Color::from_rgba(255, 255, 255, 255));
		graph->create_samples(100, 1);
		graph->configure_channel(0, gemini::Color::from_rgba(0, 255, 0, 255));
		graph->set_range(0.0f, 33.3f);
		graph->enable_baseline(true, 16.6f, gemini::Color::from_rgba(255, 0, 255, 255));
#endif

		// test tab panel

		gui::TabControl* tab = new gui::TabControl(compositor);
		tab->set_origin(10, 10);
		tab->set_dimensions(tab->dimensions_from_pixels(gui::Point(250, 250)));
		tab->set_name("tab_panel");

		label = new gui::Label(tab);
		label->set_origin(50, 115);
		label->set_dimensions(label->dimensions_from_pixels(gui::Point(110, 40)));
		label->set_background_color(gemini::Color::from_rgba(32, 32, 32, 255));
		label->set_foreground_color(gemini::Color::from_rgba(0, 255, 0, 255));
		label->set_font(dev_font, dev_font_size);
		label->set_text("ABCDEFGHIJKLMNOPQRSTUVWXYZ");

		tab->add_tab(0, "test", label);

		{
			label = new gui::Label(tab);
			label->set_origin(50, 115);
			label->set_dimensions(label->dimensions_from_pixels(gui::Point(110, 40)));
			label->set_background_color(gemini::Color::from_rgba(32, 32, 32, 255));
			label->set_foreground_color(gemini::Color::from_rgba(255, 0, 0, 255));
			label->set_font(dev_font, dev_font_size);
			label->set_text("adam 0123456789");
			tab->add_tab(1, "test2", label);
		}

		// test buttons
#if 1
		gemini::Color button_background = gemini::Color::from_rgba(128, 128, 128, 255);
		gemini::Color button_hover = gemini::Color::from_rgba(255, 255, 128, 255);

		uint32_t button_width = 320;
		uint32_t button_height = 50;
		uint32_t button_spacing = 10;
		const size_t total_buttons = 4;
//		uint32_t vertical_offset = 0;
		uint32_t origin_x = (compositor->get_size().width/2.0f) - (button_width/2.0f);
		uint32_t origin_y = (compositor->get_size().height/2.0f) - ((button_height*total_buttons)/2.0f);

		const char* captions[total_buttons] = {
			"New Game",
			"Test Button",
			"Test Two",
			"Test Three"
		};

		gui::Button* buttons[total_buttons] = {nullptr};

		for (size_t index = 0; index < total_buttons; ++index)
		{
			gui::Button* button = new gui::Button(compositor);
			button->set_origin(origin_x, origin_y);
			button->set_dimensions(button->dimensions_from_pixels(gui::Point(button_width, button_height)));
			button->set_font(menu_font, menu_font_size);
			button->set_text(captions[index]);
			button->set_background_color(button_background);
			button->set_hover_color(button_hover);
			button->on_click.connect(&TestUi::test_button_clicked, this);

			origin_y += button_height + button_spacing;
			buttons[index] = button;
		}


		buttons[0]->set_visible(false);
		LOGV("button %p is not visible\n", buttons[0]);
#endif

		// test slider

		// slider label to check value
		slider_label = new gui::Label(compositor);
		slider_label->set_origin(230, 300);
		slider_label->set_dimensions(slider_label->dimensions_from_pixels(gui::Point(40, 30)));
		slider_label->set_background_color(gemini::Color::from_rgba(0, 0, 0, 0));
		slider_label->set_foreground_color(gemini::Color::from_rgba(255, 255, 255, 255));
		slider_label->set_text("empty");
		slider_label->set_font("fonts/debug.ttf", 16);

		slider = new gui::Slider(compositor);
		slider->set_origin(20, 300);
		slider->set_dimensions(slider->dimensions_from_pixels(gui::Point(200, 40)));
		slider->set_background_color(gemini::Color::from_rgba(60, 60, 60, 255));
		slider->set_foreground_color(gemini::Color::from_rgba(255, 255, 255, 255));
		slider->on_value_changed.connect(&TestUi::slider_value_changed, this);
		slider->set_value(0.5f);

	}

	virtual kernel::Error startup()
	{
		gemini::runtime_startup("arcfusion.net/gemini/test_render");

		input::startup();

		platform::window::startup(platform::window::RenderingBackend_Default);

		size_t total_displays = platform::window::screen_count();
		LOGV("-> total screens: %lu\n", total_displays);

		for (size_t i = 0; i < total_displays; ++i)
		{
			platform::window::Frame frame = platform::window::screen_frame(i);
			LOGV("display %lu rect = %2.2f, %2.2f, %2.2f x %2.2f\n", (unsigned long)i, frame.x, frame.y, frame.width, frame.height);
		}



		// create a test window
		platform::window::Parameters params;
		params.frame = platform::window::centered_window_frame(0, 1280, 720);
		params.window_title = "test_ui";

		native_window = platform::window::create(params);
		assert(native_window != nullptr);
		platform::window::Frame window_frame = platform::window::get_frame(native_window);
		LOGV("window dimensions: %2.2f %2.2f\n", window_frame.width, window_frame.height);

		platform::window::focus(native_window);

		// initialize render device
		render2::RenderParameters render_parameters;
		render_parameters["rendering_backend"] = "default";
		render_parameters["gamma_correct"] = "true";

		device = render2::create_device(render_parameters);
		assert(device != nullptr);

		window_frame = platform::window::get_frame(native_window);

		device->init(static_cast<int>(window_frame.width), static_cast<int>(window_frame.height));

		// setup the pipeline
		render2::PipelineDescriptor desc;
		desc.shader = device->create_shader("vertexcolor");
		desc.vertex_description.add("in_position", render2::VD_FLOAT, 3); // position
		desc.vertex_description.add("in_color", render2::VD_FLOAT, 4); // color
		desc.input_layout = device->create_input_layout(desc.vertex_description, desc.shader);
		pipeline = device->create_pipeline(desc);

		// create a vertex buffer and populate it with data
		float width = (float)window_frame.width;
		float height = (float)window_frame.height;

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
		modelview_matrix = glm::mat4(1.0f);
		projection_matrix = glm::ortho(0.0f, width, height, 0.0f, -1.0f, 1.0f);

		pipeline->constants().set("modelview_matrix", &modelview_matrix);
		pipeline->constants().set("projection_matrix", &projection_matrix);

		kernel::parameters().step_interval_seconds = (1.0f/50.0f);


		// initialize fonts
		font::startup(device);

		// initialize gui
		setup_gui(window_frame.width, window_frame.height);

		return kernel::NoError;
	}

	void update()
	{
		assert(kernel::parameters().step_interval_seconds != 0.0f);
		uint64_t current_time = platform::microseconds();
		kernel::Parameters& params = kernel::parameters();

		// need to initialize this to current_time otherwise the initial
		// delta could be very large; which leads to an infinite loop
		// when processing the accumulator.
		static uint64_t last_time = current_time;
		static float accumulator = 0;

		// calculate delta ticks in milliseconds
		params.framedelta_milliseconds = (current_time - last_time)*0.001f;

		// cache the value in seconds
		params.framedelta_seconds = params.framedelta_milliseconds*0.001f;

		last_time = current_time;

		// update accumulator
		accumulator += params.framedelta_seconds;

		while(accumulator >= params.step_interval_seconds)
		{
			// subtract the interval from the accumulator
			accumulator -= params.step_interval_seconds;

			// increment tick counter
			params.current_tick++;
		}

		params.step_alpha = accumulator / params.step_interval_seconds;
		if ( params.step_alpha >= 1.0f )
		{
			params.step_alpha -= 1.0f;
		}
	}

	virtual void tick()
	{
		update();

		// update our input
		input::update();

		// dispatch all window events
		platform::window::dispatch_events();

		if (graph)
		{
			graph->record_value(kernel::parameters().framedelta_milliseconds, 0);
		}

		// sanity check
		assert(device);
		assert(pipeline);
		assert(vertex_buffer);

		static float rot = 0.0f;

		rot += 10.f*kernel::parameters().framedelta_seconds;

		if (label)
		{
			core::memory::Zone* zone = core::memory::global_allocator().memory_zone;
			label->set_text(core::str::format("total_bytes: %i, total_allocations: %i", zone->get_total_bytes(), zone->get_total_allocations()));
		}


//		graph->set_rotation(mathlib::degrees_to_radians(rot));

		if (rot > 360)
			rot -= 360.0f;

		// update the gui
		compositor->tick(kernel::parameters().framedelta_seconds);

#if 1
		render2::Pass render_pass;
		render_pass.target = device->default_render_target();
		render_pass.color(0.0f, 0.0f, 0.0f, 1.0f);
		render_pass.clear_color = true;
		render_pass.clear_depth = true;
		render_pass.depth_test = false;

		// create a command queue
		render2::CommandQueue* queue = device->create_queue(render_pass);

		// create a command serializer for the queue
		render2::CommandSerializer* serializer = device->create_serializer(queue);
		assert(serializer);

		// add commands to the queue
		serializer->pipeline(pipeline);
//		serializer->viewport(0, 0, native_window->dimensions.width, native_window->dimensions.height);
		serializer->vertex_buffer(vertex_buffer);
//		serializer->draw_indexed_primitives(index_buffer, 3);
		serializer->draw(0, 3);
		device->destroy_serializer(serializer);

		// queue the buffer with our device
		device->queue_buffers(queue, 1);


		compositor->draw();
#endif

		platform::window::activate_context(native_window);
		device->submit();
		platform::window::swap_buffers(native_window);
	}


	virtual void shutdown()
	{
		// shutdown/destroy the gui
		delete compositor;

		resource_cache.clear();

		// shutdown the fonts
		font::shutdown();

		device->destroy_buffer(vertex_buffer);
		device->destroy_pipeline(pipeline);
		render2::destroy_device(device);

		platform::window::destroy(native_window);
		platform::window::shutdown();

		input::shutdown();

		gemini::runtime_shutdown();
	}


private:
	render2::Device* device;
	render2::Pipeline* pipeline;
	render2::Buffer* vertex_buffer;
};

PLATFORM_MAIN
{
	PLATFORM_IMPLEMENT_PARAMETERS();
	PLATFORM_RETURN(platform::run_application(new TestUi()));
}
