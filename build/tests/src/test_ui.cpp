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

#include <ui/ui.h>
#include <ui/compositor.h>
#include <ui/graph.h>
#include <ui/button.h>



#include <core/threadsafequeue.h>


#include <assert.h>

using namespace renderer;

#define USE_SERIAL 1

#if USE_SERIAL
platform::Serial* _serial_device = nullptr;
platform::Thread _thread_data;


int poll_data = 1;

struct nunchuck_packet
{
	int8_t joyx;
	int8_t joyy;
	int8_t cbutton;
	int8_t zbutton;
//	int8_t eop[2];

	bool is_null() const
	{
		return (joyx == 0 && joyy == 0 && cbutton == 0 && zbutton == 0);
	}

//	void clamp()
//	{
//		float temp = joyx;
//		joyx = glm::clamp(temp, -127.0f, 127.0f);
//
//		temp = joyy;
//		joyy = glm::clamp(temp, -127.0f, 127.0f);
//	}
};

ThreadSafeQueue<nunchuck_packet> _message_queue;


void data_thread(void* context)
{
	fprintf(stdout, "data_thread enter\n");



	while (poll_data)
	{
		const size_t PACKET_SIZE = 4*128;
		uint8_t buffer[PACKET_SIZE];
		memset(buffer, 0, sizeof(PACKET_SIZE));

		int bytes_read = platform::serial_read(_serial_device, buffer, PACKET_SIZE);
		fprintf(stdout, "bytes_read = %i\n", bytes_read);
		if ((size_t)bytes_read >= sizeof(nunchuck_packet))
		{
			nunchuck_packet* packet = reinterpret_cast<nunchuck_packet*>(buffer);
//			fprintf(stdout, "-> %i %i %i %i\n", packet->joyx, packet->joyy, packet->cbutton, packet->zbutton);
			_message_queue.enqueue(*packet);
		}
	}
	fprintf(stdout, "data_thread exit\n");
}


#endif



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


namespace experimental
{
	struct GUIVertex
	{
		glm::vec2 position;
		float color[4];
		glm::vec2 uv;

		void set_position(float x, float y)
		{
			position.x = x;
			position.y = y;
		}

		void set_color(float red, float green, float blue, float alpha)
		{
			assert(red >= 0.0f && red <= 1.0f);
			assert(green >= 0.0f && green <= 1.0f);
			assert(blue >= 0.0f && blue <= 1.0f);
			assert(alpha >= 0.0f && alpha <= 1.0f);
			color[0] = red;
			color[1] = green;
			color[2] = blue;
			color[3] = alpha;
		}

		void set_uv(float u, float v)
		{
			uv[0] = u;
			uv[1] = v;
		}
	};

	const size_t MAX_VERTICES = 4096;
	class GUIRenderer : public gui::Renderer
	{
		gui::Compositor* compositor;
		float current_depth;

		render2::Device* device;
		render2::Buffer* vertex_buffer;
		render2::Pipeline* pipeline;
		render2::Texture* white_texture;

		glm::mat4 modelview_matrix;
		glm::mat4 projection_matrix;
		unsigned int diffuse_texture;

	public:

		void set_device(render2::Device* device) { this->device = device; }

		virtual void increment_depth();

		virtual void startup(gui::Compositor* c);
		virtual void shutdown(gui::Compositor* c);

		virtual void begin_frame(gui::Compositor* c);
		virtual void end_frame();

		virtual void draw_bounds(const gui::Rect& bounds, const gui::Color& color);
		virtual void draw_textured_bounds(const gui::Rect& bounds, const gui::TextureHandle& handle);
		void draw_line(const gui::Point& start, const gui::Point& end, const gui::Color& color);
		virtual gui::TextureResult texture_create(const char* path, gui::TextureHandle& handle);
		virtual void texture_destroy(const gui::TextureHandle& handle);
		virtual gui::TextureResult texture_info(const gui::TextureHandle& handle, uint32_t& width, uint32_t& height, uint8_t& channels);
		virtual gui::FontResult font_create(const char* path, gui::FontHandle& handle);
		virtual void font_destroy(const gui::FontHandle& handle);
		virtual gui::FontResult font_measure_string(const gui::FontHandle& handle, const char* string, gui::Rect& bounds);
		virtual void font_draw(const gui::FontHandle& handle, const char* string, const gui::Rect& bounds, const gui::Color& color);
		virtual gui::FontResult font_fetch_texture(const gui::FontHandle& handle, gui::TextureHandle& texture);
		virtual void draw_command_lists(gui::render::CommandList** command_lists, Array<gui::render::Vertex>& vertex_buffer, size_t total_lists);

	}; // GUIRenderer

	void GUIRenderer::increment_depth()
	{
		current_depth += 1.0f;
	}

	void GUIRenderer::startup(gui::Compositor* compositor)
	{
		this->compositor = compositor;

		this->vertex_buffer = device->create_vertex_buffer(MAX_VERTICES*sizeof(GUIVertex));

		render2::PipelineDescriptor desc;
		desc.shader = device->create_shader("gui");
		desc.vertex_description.add("in_position", render2::VD_FLOAT, 2);
		desc.vertex_description.add("in_color", render2::VD_FLOAT, 4);
		desc.vertex_description.add("in_uv", render2::VD_FLOAT, 2);
		desc.input_layout = device->create_input_layout(desc.vertex_description, desc.shader);
		desc.enable_blending = true;
		desc.blend_source = render2::BlendOp::SourceAlpha;
		desc.blend_destination = render2::BlendOp::OneMinusSourceAlpha;
		pipeline = device->create_pipeline(desc);


		render2::Image white_image;
		white_image.create(4, 4, 3);
		white_image.filter = image::FILTER_NONE;
		white_image.flags = image::F_CLAMP_BORDER;
		white_image.fill(core::Color(255, 255, 255));
		white_texture = device->create_texture(white_image);


//		// generate the white texture we'll use for solid colors
//		white_texture = assets::textures()->allocate_asset();
//		white_texture->image.create(4, 4, 3);
//		white_texture->image.fill(Color(255, 255, 255));
//		//	image::generate_checker_pattern(white_texture->image, Color(255, 0, 0), Color(0, 255, 0));
//		white_texture->image.flags = image::F_WRAP | image::F_RGB | image::F_CLAMP_BORDER;
//		white_texture->texture = ::renderer::driver()->texture_create(white_texture->image);
//		assets::textures()->take_ownership("gui/white_texture", white_texture);
//
//
//
//
//		stream.desc.add(::renderer::VD_FLOAT3);
//		stream.desc.add(::renderer::VD_UNSIGNED_BYTE4);
//		stream.desc.add(::renderer::VD_FLOAT2);
//		stream.create(64, 64, ::renderer::DRAW_TRIANGLES);
//
//		// load shader
//		shader = assets::shaders()->load_from_path("shaders/gui");
//
//		texture_map = assets::materials()->allocate_asset();
//		if (texture_map)
//		{
//			::renderer::MaterialParameter parameter;
//			parameter.type = ::renderer::MP_SAMPLER_2D;
//			parameter.name = "diffusemap";
//			parameter.texture_unit = assets::texture_unit_for_map("diffusemap");
//			parameter.texture = white_texture->texture;
//			//		parameter.texture = assets::textures()->get_default()->texture;
//
//
//			texture_map->add_parameter(parameter);
//			assets::materials()->take_ownership("gui/texture_map", texture_map);
//
//		}
	}

	void GUIRenderer::shutdown(gui::Compositor* c)
	{
		device->destroy_texture(white_texture);
		device->destroy_buffer(vertex_buffer);
		device->destroy_pipeline(pipeline);
	}

	void GUIRenderer::begin_frame(gui::Compositor* c)
	{
		current_depth = 0.0f;
//		::renderer::RenderStream rs;
//
//		rs.add_state(::renderer::STATE_BLEND, 1 );
//		rs.add_blendfunc(::renderer::BLEND_SRC_ALPHA, ::renderer::BLEND_ONE_MINUS_SRC_ALPHA );
//		rs.add_state(::renderer::STATE_DEPTH_TEST, 0);
//		rs.add_state(::renderer::STATE_DEPTH_WRITE, 0);
//
//		rs.run_commands();
	}

	void GUIRenderer::end_frame()
	{
//		::renderer::RenderStream rs;
//
//		rs.add_state(::renderer::STATE_BLEND, 0 );
//		rs.add_state(::renderer::STATE_DEPTH_TEST, 1);
//		rs.add_state(::renderer::STATE_DEPTH_WRITE, 1);
//		rs.run_commands();
	}


	void GUIRenderer::draw_bounds(const gui::Rect& bounds, const gui::Color& color) {}
	void GUIRenderer::draw_textured_bounds(const gui::Rect& bounds, const gui::TextureHandle& handle) {}
	void GUIRenderer::draw_line(const gui::Point& start, const gui::Point& end, const gui::Color& color) {}

	gui::TextureResult GUIRenderer::texture_create(const char* path, gui::TextureHandle& handle)
	{
//		assets::Texture * tex = assets::textures()->load_from_path((char*)path);
//		if ( !tex )
//		{
//			return gui::TextureResult_Failed;
//		}
//
//		handle = tex->Id();

		return gui::TextureResult_Success;
	}

	void GUIRenderer::texture_destroy(const gui::TextureHandle& handle)
	{
		// nothing really to do in our system
	}

	gui::TextureResult GUIRenderer::texture_info(const gui::TextureHandle& handle, uint32_t& width, uint32_t& height, uint8_t& channels)
	{
//		assets::Texture * tex = assets::textures()->find_with_id( handle );
//		if ( !tex )
//		{
//			return gui::TextureResult_Failed;
//		}
//
		return gui::TextureResult_Success;
	}

	gui::FontResult GUIRenderer::font_create(const char* path, gui::FontHandle& handle)
	{
		Array<unsigned char> fontdata;
		core::filesystem::instance()->virtual_load_file(fontdata, path);
		render2::font::Handle fonthandle = render2::font::load_from_memory(&fontdata[0], fontdata.size(), 8);
		assert(fonthandle.is_valid());
		handle = gui::FontHandle(fonthandle);

//		assets::Font* font = assets::fonts()->load_from_path((char*)path);
//		if (font == 0)
//		{
//			return gui::FontResult_Failed;
//		}
//
//		handle = font->Id();

		return gui::FontResult_Success;
	}

	void GUIRenderer::font_destroy(const gui::FontHandle& handle)
	{
		// nothing really to do in our system
		render2::font::Handle fonthandle(handle);
		// TODO: implement this
//		render2::font::destroy_font(fonthandle);
	}

	gui::FontResult GUIRenderer::font_measure_string(const gui::FontHandle& handle, const char* string, gui::Rect& bounds)
	{
//		assets::Font* font = assets::fonts()->find_with_id(handle);
//		if (font)
//		{
//			unsigned int width = font::measure_width(font->handle, string);
//			unsigned int height = font::measure_height(font->handle, string);
//			bounds.set(0, 0, width, height);
//			return gui::FontResult_Success;
//		}

		// TODO: implement this for fonts
		bounds.set(0, 0, 12, 12);
		return gui::FontResult_Success;

		return gui::FontResult_Failed;
	}

	void GUIRenderer::font_draw(const gui::FontHandle& handle, const char* string, const gui::Rect& bounds, const gui::Color& color)
	{
//		assets::Font* font = assets::fonts()->find_with_id(handle);
//		if (font)
//		{
//			font::draw_string(font->handle, bounds.origin.x, bounds.origin.y, string, Color(color.r(), color.g(), color.b(), color.a()));
//		}
	}

	gui::FontResult GUIRenderer::font_fetch_texture(const gui::FontHandle &handle, gui::TextureHandle &texture)
	{
		return gui::FontResult_Failed;
	}

	void GUIRenderer::draw_command_lists(gui::render::CommandList** command_lists, Array<gui::render::Vertex>& vertex_buffer, size_t total_lists)
	{
		size_t total_vertices = vertex_buffer.size();

		// temp limit
		assert(total_vertices < MAX_VERTICES);
		projection_matrix = glm::ortho(0.0f, (float)this->compositor->width, (float)this->compositor->height, 0.0f, -1.0f, 1.0f);

//		device->buffer_resize(vertex_buffer, sizeof(GUIVertex) * total_vertices);

		diffuse_texture = 0;
		pipeline->constants().set("projection_matrix", &projection_matrix);
		pipeline->constants().set("diffuse", &diffuse_texture);

		assert(total_lists > 0);

		GUIVertex vertices[MAX_VERTICES];
		size_t vertex_index = 0;
		// loop through all vertices in the source vertex_buffer
		// and convert them to our buffer
		for (size_t index = 0; index < total_vertices; ++index)
		{
			gui::render::Vertex* gv = &vertex_buffer[index];

			GUIVertex& vt = vertices[vertex_index];
			vt.set_position(gv->x, gv->y);
			vt.set_color(gv->color.r()/255.0f, gv->color.g()/255.0f, gv->color.b()/255.0f, gv->color.a()/255.0f);
			vt.set_uv(gv->uv[0], gv->uv[1]);

			++vertex_index;
		}

		device->buffer_upload(this->vertex_buffer, vertices, sizeof(GUIVertex)*total_vertices);

		size_t command_index = 0;
		for (size_t index = 0; index < total_lists; ++index)
		{
			gui::render::CommandList* commandlist = command_lists[index];

			// setup the pass and queue the draw
			render2::Pass pass;
			pass.target = device->default_render_target();
			pass.color(1.0f, 0.0f, 0.0f, 1.0f);
			pass.clear_color = false;
			pass.clear_depth = false;

			render2::CommandQueue* queue = device->create_queue(pass);
			render2::CommandSerializer* serializer = device->create_serializer(queue);
			serializer->pipeline(pipeline);
			serializer->vertex_buffer(this->vertex_buffer);

			for (gui::render::Command& command : commandlist->commands)
			{
				if (!command.texture.is_valid())
				{
					serializer->texture(white_texture, 0);
				}
				serializer->draw(command.vertex_offset, command.vertex_count);
				++command_index;
			}
			device->queue_buffers(queue, 1);

			device->destroy_serializer(serializer);

		}


	}
} // namespace experimental



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


	void set_cbutton(int value)
	{
		if (value)
		{
			flags |= 2;
		}
		else
		{
			flags &= ~2;
		}
	}

	void set_zbutton(int value)
	{
		if (value)
		{
			flags |= 1;
		}
		else
		{
			flags &= ~1;
		}
	}

	virtual bool can_move() const { return true; }

	virtual void update(gui::Compositor* compositor, const gui::TimeState& timestate)
	{




		gui::Panel::update(compositor, timestate);
	}


	virtual void render(gui::Rect& frame, gui::Compositor* compositor, gui::Renderer* renderer, gui::Style* style)
	{
		render_commands.reset();
		render_commands.add_rectangle(
			geometry[0],
			geometry[1],
			geometry[2],
			geometry[3],
			gui::render::WhiteTexture,
			background_color);


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
		compositor->queue_commandlist(&render_commands);
//
//		for(PanelVector::iterator it = children.begin(); it != children.end(); ++it)
//		{
//			Panel* child = (*it);
//			if (child->is_visible())
//			{
//				child->render(child->bounds, compositor, renderer, style);
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
	experimental::GUIRenderer renderer;
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

	TestUi()
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
		gui::set_allocator(gui_malloc_callback, gui_free_callback);
		compositor = new gui::Compositor(width, height);

		root = new gui::Panel(compositor);
		platform::window::Frame frame = platform::window::get_render_frame(native_window);

		renderer.set_device(device);
		compositor->set_renderer(&renderer);

		root->set_bounds(0, 0, frame.width, frame.height);
		root->set_background_color(gui::Color(255, 0, 255, 0));

		// setup the framerate graph
		graph = new gui::Graph(root);
		graph->set_bounds(width-250, 0, 250, 100);
		graph->set_font(compositor, "fonts/nokiafc22.ttf");
		graph->set_background_color(gui::Color(60, 60, 60, 255));
		graph->set_foreground_color(gui::Color(255, 255, 255, 255));
		graph->create_samples(100, 1);
		graph->configure_channel(0, gui::Color(0, 255, 0, 255));
		graph->set_range(0.0f, 33.3f);
		graph->enable_baseline(true, 16.6f, gui::Color(255, 0, 255, 255));

		if (_serial_device)
		{
			ctp = new ControllerTestPanel(root);
			ctp->set_bounds(0, 0, 300, 300);
			ctp->set_background_color(gui::Color(80, 80, 80));
		}

//		gui::Panel* panel = new gui::Panel(root);
//		panel->set_bounds(width-250, 0, 250, 100);
//		panel->set_background_color(gui::Color(60, 60, 60, 255));



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
			newgame->set_font(compositor, "fonts/nokiafc22.ttf");
			newgame->set_text(captions[index]);
			newgame->set_background_color(button_background);
			newgame->set_hover_color(button_hover);
			newgame->set_userdata((void*)2);
			origin_y += button_height + button_spacing;
		}
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

#if USE_SERIAL
		const char* serial_device = "/dev/cu.usbmodem1d151311";
		const size_t baud = 57600;
		_serial_device = platform::serial_open(serial_device, baud);
		if (!_serial_device)
		{
			LOGW("unable to open serial device\n");
		}
		else
		{
			platform::thread_create(_thread_data, data_thread, 0);
		}

#endif

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

//		graph->set_rotation(mathlib::degrees_to_radians(rot));

		if (rot > 360)
			rot -= 360.0f;


#if USE_SERIAL
		if (ctp)
		{
			// process the queue
			while(_message_queue.size() > 0)
			{
				nunchuck_packet packet = _message_queue.dequeue();
//				fprintf(stdout, "<- %i %i %i %i\n", packet.joyx, packet.joyy, packet.cbutton, packet.zbutton);
				ctp->set_x(packet.joyx);
				ctp->set_y(-packet.joyy);
				ctp->set_cbutton(packet.cbutton);
				ctp->set_zbutton(packet.zbutton);
			}
		}
#endif


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
		poll_data = 0;
		platform::thread_join(_thread_data);

		if (_serial_device)
		{
			platform::serial_close(_serial_device);
		}



		// shutdown/destroy the gui
		delete compositor;

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