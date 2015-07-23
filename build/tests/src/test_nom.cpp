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

#include <nom/nom.hpp>
#include <nom/compositor.hpp>
#include <nom/graph.hpp>
#include <nom/button.hpp>

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

struct ConstantData
{
	glm::mat4 modelview_matrix;
	glm::mat4 projection_matrix;
};

namespace experimental
{
	class GUIRenderer : public gui::Renderer
	{
		struct VertexType
		{
			float position[3];
//			float color[4];
//			glm::vec3 position;
			core::Color color;
//			glm::vec2 uv;

			void set_position(float x, float y, float z)
			{
				position[0] = x;
				position[1] = y;
				position[2] = z;
			}
		};
		
		gui::Compositor* compositor;
		float current_depth;

	//private:
	//	void render_buffer(::renderer::VertexStream& stream, gemini::assets::Shader* shader, gemini::assets::Material* material);

		render2::Device* device;
		render2::Buffer* vertex_buffer;
		render2::Pipeline* pipeline;
		
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
		virtual void draw_command_lists(gui::render::CommandList** command_lists, size_t total_lists);
		
	}; // GUIRenderer
	
	void GUIRenderer::increment_depth()
	{
		current_depth += 1.0f;
	}
	
	void GUIRenderer::startup(gui::Compositor* compositor)
	{
		this->compositor = compositor;
		
		
		this->vertex_buffer = device->create_vertex_buffer(1024*sizeof(VertexType));
		
		
		render2::PipelineDescriptor desc;
		desc.shader = device->create_shader("test");
		desc.vertex_description.add("in_position", render2::VD_FLOAT, 3);
		desc.vertex_description.add("in_color", render2::VD_UNSIGNED_BYTE, 4);
		desc.input_layout = device->create_input_layout(desc.vertex_description, desc.shader);
		desc.enable_blending = true;
		desc.blend_source = render2::BlendOp::SourceAlpha;
		desc.blend_destination = render2::BlendOp::OneMinusSourceAlpha;
		pipeline = device->create_pipeline(desc);
		
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
	
	void GUIRenderer::draw_command_lists(gui::render::CommandList** command_lists, size_t total_lists)
	{
		size_t total_vertices = 0;
		for (size_t index = 0; index < total_lists; index++)
		{
			total_vertices += command_lists[index]->vertex_buffer.size();
		}

		// temp limit
		assert(total_vertices < 1024);
		
		
		ConstantData cd;
		cd.modelview_matrix = glm::mat4(1.0f);
		cd.projection_matrix = glm::ortho(0.0f, (float)this->compositor->width, (float)this->compositor->height, 0.0f, -1.0f, 1.0f);
		pipeline->constants()->assign(&cd, sizeof(ConstantData));
		
		VertexType vertices[1024];
		size_t vertex_index = 0;
		for (size_t index = 0; index < total_lists; ++index)
		{
			gui::render::CommandList* commandlist = command_lists[index];
			
			//			for (size_t command_id = 0; command_id < commandlist->commands.size(); ++command_id)
			//			{
			//				LOGV("command: %i\n", commandlist->commands[command_id].id);
			//			}
			
			// loop through all vertices in this list
			for (size_t v = 0; v < commandlist->vertex_buffer.size(); ++v)
			{
				gui::render::Vertex* gv = &commandlist->vertex_buffer[v];
				
				
				VertexType& vt = vertices[vertex_index];
				vt.set_position(gv->x, gv->y, 0);
				vt.color = core::Color(gv->color.r(), gv->color.g(), gv->color.b(), gv->color.a());
				
				
				++vertex_index;
			}
			
//				gui::render::Vertex* gv = &commandlist->vertex_buffer[v];
//				vertex[v].position.x = gv->x;
//				vertex[v].position.y = gv->y;
//				core::Color c = core::Color(gv->color.r(), gv->color.g(), gv->color.b(), gv->color.a());
//				vertex[v].color = c;
//				
//				vertex[v].uv.x = gv->uv[0];
//				vertex[v].uv.y = gv->uv[1];
//			}

			device->buffer_upload(vertex_buffer, vertices, sizeof(VertexType)*total_vertices);
			
			render2::Pass pass;
			pass.target = device->default_render_target();

			render2::CommandQueue* queue = device->create_queue(pass);
			render2::CommandSerializer* serializer = device->create_serializer(queue);

			serializer->pipeline(pipeline);
			serializer->vertex_buffer(vertex_buffer);
			serializer->draw(0, total_vertices);

			device->queue_buffers(queue, 1);
			
			device->destroy_serializer(serializer);
			//offset += commandlist->vertex_buffer.size();
		}
	}
} // namespace experimental

// ---------------------------------------------------------------------
// TestNom
// ---------------------------------------------------------------------
class TestNom : public kernel::IKernel,
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
				break;
			default:
				break;
		}
	}
public:

	TestNom()
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
		graph->set_font(compositor, "fonts/debug");
		graph->set_background_color(gui::Color(60, 60, 60, 255));
		graph->set_foreground_color(gui::Color(255, 255, 255, 255));
		graph->create_samples(100, 1);
		graph->configure_channel(0, gui::Color(0, 255, 0, 255));
		graph->set_range(0.0f, 33.3f);

		graph->enable_baseline(true, 16.6f, gui::Color(255, 0, 255, 255));
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
			PLATFORM_LOG(platform::LogMessageType::Info, "display %lu rect = %u, %u, %u x %u\n", (unsigned long)i, frame.x, frame.y, frame.width, frame.height);
		}



		// create a test window
		platform::window::Parameters params;
		params.frame = platform::window::centered_window_frame(0, 1280, 720);
		params.window_title = "test_render";
		
		native_window = platform::window::create(params);
		assert(native_window != nullptr);
		platform::window::Frame window_frame = platform::window::get_frame(native_window);
		PLATFORM_LOG(platform::LogMessageType::Info, "window dimensions: %i %i\n", window_frame.width, window_frame.height);
		
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
		desc.shader = device->create_shader("test");
		desc.vertex_description.add("in_position", render2::VD_FLOAT, 3); // position
		desc.vertex_description.add("in_color", render2::VD_FLOAT, 4); // color
		desc.input_layout = device->create_input_layout(desc.vertex_description, desc.shader);
		pipeline = device->create_pipeline(desc);
		
		// create a vertex buffer and populate it with data
		float width = (float)window_frame.width;
		float height = (float)window_frame.height;
		
		device->init(window_frame.width, window_frame.height);
		
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

		kernel::parameters().step_interval_seconds = (1.0f/50.0f);
		setup_gui(window_frame.width, window_frame.height);

		return kernel::NoError;
	}
	
	void update()
	{
		static uint64_t last_time = 0;
		static float accumulator = 0;
		
		uint64_t current_time = platform::microseconds();
		kernel::Parameters& params = kernel::parameters();
		
		// calculate delta ticks in miliseconds
		params.framedelta_raw_msec = (current_time - last_time)*0.001f;
		// cache the value in seconds
		params.framedelta_filtered_seconds = params.framedelta_raw_msec*0.001f;
		last_time = current_time;
		
		// update accumulator
		accumulator += params.framedelta_filtered_seconds;
		
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
		
		// update the gui
		compositor->update(kernel::parameters().framedelta_filtered_seconds);
	
		// sanity check
		assert(device);
		assert(pipeline);
		assert(vertex_buffer);
		
		static float rot = 0.0f;
		
		rot += 10.f*kernel::parameters().framedelta_filtered_seconds;
		
		graph->set_rotation(mathlib::degrees_to_radians(rot));

		if (rot > 360)
			rot -= 360.0f;
		
		// submit the queues to the GPU
		platform::window::activate_context(native_window);
		
		if (graph)
		{
			graph->record_value(kernel::parameters().framedelta_raw_msec, 0);
		}


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

		device->submit();

		platform::window::swap_buffers(native_window);
	}


	virtual void shutdown()
	{
		delete compositor;
	
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
	render2::Device* device;
	render2::Pipeline* pipeline;
	render2::Buffer* vertex_buffer;
};

PLATFORM_MAIN
{
	PLATFORM_IMPLEMENT_PARAMETERS();
	PLATFORM_RETURN(platform::run_application(new TestNom()));
}