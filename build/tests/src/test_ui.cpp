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
#include <renderer/font.h>
#include <renderer/guirenderer.h>

#include <ui/ui.h>
#include <ui/compositor.h>
#include <ui/graph.h>
#include <ui/button.h>



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



class StandaloneResourceCache : public CommonResourceCache
{
	typedef HashSet<render2::Texture*, gui::TextureHandle> TextureToHandleSet;
	TextureToHandleSet handle_by_texture;

	Array<render2::Texture*> textures;

	typedef Array<gui::FontHandle> FontHandleArray;

	typedef HashSet<const char*, FontHandleArray> FontByPathSet;
	FontByPathSet font_handle_by_path;

public:
	StandaloneResourceCache();

	virtual void clear() override;

	virtual gui::FontHandle create_font(const char* filename, size_t pixel_size) override;
	virtual void destroy_font(const gui::FontHandle& handle) override;
	virtual gui::TextureHandle texture_for_font(const gui::FontHandle& handle) override;
	virtual gui::TextureHandle create_texture(const char* filename) override;
	virtual void destroy_texture(const gui::TextureHandle& handle) override;

public:
	virtual int track_texture(render2::Texture* texture) override;
	virtual gui::TextureHandle texture_to_handle(render2::Texture* texture) override;
	virtual render2::Texture* handle_to_texture(const gui::TextureHandle& handle) override;
};

StandaloneResourceCache::StandaloneResourceCache() :
	textures(0)
{

}

void StandaloneResourceCache::clear()
{
	for (FontByPathSet::Iterator it = font_handle_by_path.begin(); it != font_handle_by_path.end(); ++it)
	{
		Array<gui::FontHandle>& items = it.value();
		items.clear();
	}

	textures.clear();

	handle_by_texture.clear();
	font_handle_by_path.clear();
}

gui::FontHandle StandaloneResourceCache::create_font(const char* filename, size_t pixel_size)
{
	// This has to intelligently check to see if we've
	// already loaded a font by matching
	// both the filename and the requested pixel_size.
	if (font_handle_by_path.has_key(filename))
	{
		Array<gui::FontHandle>& items = font_handle_by_path[filename];

		for (auto& handle : items)
		{
			render2::font::Handle font_handle(handle);
			if (render2::font::get_pixel_size(font_handle) == pixel_size)
			{
				return handle;
			}
		}
	}

	Array<unsigned char> fontdata;
	core::filesystem::instance()->virtual_load_file(fontdata, filename);
	render2::font::Handle fonthandle = render2::font::load_from_memory(&fontdata[0], fontdata.size(), pixel_size);
	assert(fonthandle.is_valid());

	gui::FontHandle handle(fonthandle);

	// we need to track the texture for looking during rendering
	render2::Texture* texture = render2::font::get_font_texture(fonthandle);
	track_texture(texture);

	// insert the new handle into the array
	Array<gui::FontHandle>& items = font_handle_by_path[filename];
	items.push_back(handle);

	return handle;
}

void StandaloneResourceCache::destroy_font(const gui::FontHandle& handle)
{
	// ignore these for now; we take care of font tracking
//	render2::font::Handle fonthandle(handle);
//	render2::font::destroy_font(fonthandle);
}

gui::TextureHandle StandaloneResourceCache::texture_for_font(const gui::FontHandle& handle)
{
	render2::Texture* texture = render2::font::get_font_texture(render2::font::Handle(handle));
	if (handle_by_texture.has_key(texture))
	{
		return handle_by_texture[texture];
	}

	return gui::TextureHandle();
}

gui::TextureHandle StandaloneResourceCache::create_texture(const char* filename)
{
	gui::TextureHandle handle;
	assert(0);
	return handle;
}

int StandaloneResourceCache::track_texture(render2::Texture *texture)
{
	assert(!handle_by_texture.has_key(texture));


	int ref = static_cast<int>(textures.size());
	textures.push_back(texture);
	handle_by_texture[texture] = ref;

	return ref;
}

void StandaloneResourceCache::destroy_texture(const gui::TextureHandle& handle)
{
	assert(0);
}

gui::TextureHandle StandaloneResourceCache::texture_to_handle(render2::Texture* texture)
{
	assert(handle_by_texture.has_key(texture));
	gui::TextureHandle handle = handle_by_texture[texture];
	return handle;
}

render2::Texture* StandaloneResourceCache::handle_to_texture(const gui::TextureHandle& handle)
{
	assert(handle.is_valid());
	int index = handle;
	return textures[index];
}


class ControllerTestPanel : public gui::Panel
{
	glm::vec2 mins;
	glm::vec2 maxs;

	glm::vec2 last;

	int flags;
public:
	ControllerTestPanel(gui::Panel* root) : gui::Panel(root)
	{
		mins = glm::vec2(-127, -127);
		maxs = glm::vec2(127, 127);

		last.x = 0;
		last.y = 0;

		flags = 0;
	}


	void set_x(float value)
	{
		last.x = value;
	}

	void set_y(float value)
	{
		last.y = value;
	}

	virtual bool can_move() const { return true; }

	virtual void update(gui::Compositor* compositor, const gui::TimeState& timestate)
	{
		gui::Panel::update(compositor, timestate);
	}

	virtual void render(gui::Compositor* compositor, gui::Renderer* renderer, gui::render::CommandList& render_commands) override
	{
		render_commands.reset();
		render_commands.add_rectangle(
			geometry[0],
			geometry[1],
			geometry[2],
			geometry[3],
			gui::render::WhiteTexture,
			background_color);

		gui::Rect frame = bounds;
		glm::vec2 framesize(frame.width(), frame.height());
		glm::vec2 center = geometry[0] + (framesize / 2.0f);
		glm::vec2 start = center;
		glm::vec2 end = center + last;

		// draw the joystick vector
		render_commands.add_line(start, end, gui::Color(255, 0, 0), 3.0f);

		// render the outline
		gui::Color color(0, 0, 0);


		gui::Color c_down(0, 255, 255);
		gui::Color z_down(255, 128, 0);

		if (flags & 2)
			color = c_down;
		else if (flags & 1)
			color = z_down;

		render_commands.add_line(geometry[0], gui::Point(geometry[0].x+frame.width(), geometry[0].y), color, 4.0f);
		render_commands.add_line(gui::Point(geometry[0].x+frame.width(), geometry[0].y), geometry[0] + framesize, color, 4.0f);

//
//		if (this->background != 0)
//		{
////			renderer->draw_textured_bounds(frame, this->background);
//			render_commands.add_rectangle(
//				geometry[0],
//				geometry[1],
//				geometry[2],
//				geometry[3],
//				this->background,
//				gui::Color(255, 255, 255, 255)
//			);
//		}
//
//
//		for(PanelVector::iterator it = children.begin(); it != children.end(); ++it)
//		{
//			Panel* child = (*it);
//			if (child->is_visible())
//			{
//				child->render(compositor, renderer, style);
//			}
//		}

//		gui::Panel::render(frame, compositor, renderer, style);
	} // render
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
	gui::Panel* root;
	gui::Graph* graph;
	gui::Label* label;
	GUIRenderer renderer;
	StandaloneResourceCache resource_cache;
	ControllerTestPanel* ctp;

	glm::mat4 modelview_matrix;
	glm::mat4 projection_matrix;

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
			compositor->cursor_button(input_to_gui[event.button], event.is_down);
		}
	}

	virtual void event(kernel::SystemEvent& event)
	{
		switch(event.subtype)
		{
			case kernel::WindowResized:
				LOGV("window resized: %i x %i\n", event.render_width, event.render_height);
				if (device)
				{
					device->backbuffer_resized(event.render_width, event.render_height);
				}

				compositor->resize(event.render_width, event.render_height);

				break;
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
		root = nullptr;
		graph = nullptr;
	}

	virtual bool is_active() const { return active; }
	virtual void set_active(bool isactive) { active = isactive; }

	void setup_gui(int width, int height)
	{
		renderer.set_device(device);

		gui::set_allocator(gui_malloc_callback, gui_free_callback);
		compositor = new gui::Compositor(width, height, &resource_cache, &renderer);

		root = new gui::Panel(compositor);
		platform::window::Frame frame = platform::window::get_render_frame(native_window);



		root->set_bounds(0, 0, frame.width, frame.height);
		root->set_background_color(gui::Color(255, 255, 255, 0));

//		gui::FontHandle dev_font = resource_cache.create_font("fonts/nokiafc22.ttf", 8);
//		gui::FontHandle dev_font = resource_cache.create_font("fonts/04B_08.ttf", 8);
//		gui::FontHandle menu_font = resource_cache.create_font("fonts/Arial Unicode.ttf", 24);
//		gui::FontHandle dev_font = resource_cache.create_font("fonts/Cantarell-Regular.ttf", 16);
//		gui::FontHandle dev_font = resource_cache.create_font("fonts/7x5.ttf", 8);

//		const char dev_font[] = "fonts/04B_08.ttf";
		const char dev_font[] = "fonts/nokiafc22.ttf";
		const char menu_font[] = "fonts/Arial Unicode.ttf";


		// setup the framerate graph
#if 1
		graph = new gui::Graph(root);
		graph->set_bounds(width-250, 0, 250, 100);
		graph->set_font(dev_font, 8);
		graph->set_background_color(gui::Color(60, 60, 60, 255));
		graph->set_foreground_color(gui::Color(255, 255, 255, 255));
		graph->create_samples(100, 1);
		graph->configure_channel(0, gui::Color(0, 255, 0, 255));
		graph->set_range(0.0f, 33.3f);
		graph->enable_baseline(true, 16.6f, gui::Color(255, 0, 255, 255));
#endif

#if 1
		label = new gui::Label(root);
		label->set_background_color(gui::Color(32, 32, 32));
		label->set_foreground_color(gui::Color(0, 255, 0));
		label->set_bounds(50, 75, 110, 40);
		label->set_font(dev_font, 8);
		label->set_text("This is a label");

		{
			gui::Label* label = new gui::Label(root);
			label->set_background_color(gui::Color(32, 32, 32));
			label->set_foreground_color(gui::Color(255, 0, 0));
			label->set_bounds(50, 115, 110, 40);
			label->set_font(dev_font, 8);
			label->set_text("This is another label");
		}

#endif
//		ctp = new ControllerTestPanel(root);
//		ctp->set_bounds(0, 0, 300, 300);
//		ctp->set_background_color(gui::Color(80, 80, 80));

//		gui::Panel* panel = new gui::Panel(root);
//		panel->set_bounds(width-250, 0, 250, 100);
//		panel->set_background_color(gui::Color(60, 60, 60, 255));


#if 1
		gui::Color button_background(128, 128, 128, 255);
		gui::Color button_hover(255, 255, 128, 255);

		uint32_t button_width = 320;
		uint32_t button_height = 50;
		uint32_t button_spacing = 10;
		const size_t total_buttons = 4;
//		uint32_t vertical_offset = 0;
		uint32_t origin_x = (compositor->width/2.0f) - (button_width/2.0f);
		uint32_t origin_y = (compositor->height/2.0f) - ((button_height*total_buttons)/2.0f);

		const char* captions[total_buttons] = {
			"New Game",
			"Test Button",
			"Test Two",
			"Test Three"
		};

		for (size_t index = 0; index < total_buttons; ++index)
		{
			gui::Button* newgame = new gui::Button(root);
			newgame->set_bounds(origin_x, origin_y, button_width, button_height);
			newgame->set_font(menu_font, 24);
			newgame->set_text(captions[index]);
			newgame->set_background_color(button_background);
			newgame->set_hover_color(button_hover);
			newgame->set_userdata((void*)2);
			origin_y += button_height + button_spacing;
		}
#endif
	}

	virtual kernel::Error startup()
	{
		platform::PathString root_path = platform::get_program_directory();
		platform::PathString content_path = platform::fs_content_directory();
		platform::PathString user_application_path = platform::get_user_application_directory("arcfusion.net/gemini/test_render");
//		platform::PathString temp_path = platform::get_user_temp_directory(); // adding this line breaks Android. Yes, you read that correctly.
		core::startup_filesystem();
		core::filesystem::instance()->root_directory(root_path);
		core::filesystem::instance()->content_directory(content_path);
		core::filesystem::instance()->user_application_directory(user_application_path);

		core::startup_logging();

		LOGV("root_path: %s\n", root_path());
		LOGV("content_path: %s\n", content_path());
		LOGV("user_application_path: %s\n", user_application_path());
//		LOGV("temp_path: %s\n", temp_path());

		input::startup();

		platform::window::startup(platform::window::RenderingBackend_Default);

		size_t total_displays = platform::window::screen_count();
		PLATFORM_LOG(platform::LogMessageType::Info, "-> total screens: %lu\n", total_displays);

		for (size_t i = 0; i < total_displays; ++i)
		{
			platform::window::Frame frame = platform::window::screen_frame(i);
			PLATFORM_LOG(platform::LogMessageType::Info, "display %lu rect = %2.2f, %2.2f, %2.2f x %2.2f\n", (unsigned long)i, frame.x, frame.y, frame.width, frame.height);
		}



		// create a test window
		platform::window::Parameters params;
		params.frame = platform::window::centered_window_frame(0, 1280, 720);
		params.window_title = "test_ui";

		native_window = platform::window::create(params);
		assert(native_window != nullptr);
		platform::window::Frame window_frame = platform::window::get_frame(native_window);
		PLATFORM_LOG(platform::LogMessageType::Info, "window dimensions: %2.2f %2.2f\n", window_frame.width, window_frame.height);

		platform::window::focus(native_window);

		// initialize render device
		render2::RenderParameters render_parameters;
		render_parameters["rendering_backend"] = "default";
		render_parameters["gamma_correct"] = "true";

		device = render2::create_device(render_parameters);
		assert(device != nullptr);

		window_frame = platform::window::get_frame(native_window);

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

		device->init(static_cast<int>(window_frame.width), static_cast<int>(window_frame.height));

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
		render2::font::startup(device);

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

		// calculate delta ticks in miliseconds
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
		compositor->update(kernel::parameters().framedelta_seconds);

#if 1
		render2::Pass render_pass;
		render_pass.target = device->default_render_target();
		render_pass.color(0.0f, 0.0f, 0.0f, 1.0f);
		render_pass.clear_color = true;
		render_pass.clear_depth = true;

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


		compositor->render();
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
		render2::font::shutdown();

		device->destroy_buffer(vertex_buffer);
		device->destroy_pipeline(pipeline);
		render2::destroy_device(device);

		platform::window::destroy(native_window);
		platform::window::shutdown();

		input::shutdown();

		core::shutdown();
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