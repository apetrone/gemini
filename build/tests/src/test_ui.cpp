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

class StandaloneResourceCache : public gui::ResourceCache
{
	typedef HashSet<render2::Texture*, gui::TextureHandle> TextureToHandleSet;
	TextureToHandleSet handle_by_texture;

	Array<render2::Texture*> textures;


	typedef HashSet<const char*, gui::FontHandle> FontByPathSet;
	FontByPathSet font_handle_by_path;

public:
	StandaloneResourceCache();
	void clear();

	virtual gui::FontHandle create_font(const char* filename, size_t pixel_size);
	virtual void destroy_font(const gui::FontHandle& handle);
	virtual gui::TextureHandle texture_for_font(const gui::FontHandle& handle);
	virtual gui::TextureHandle create_texture(const char* filename);
	virtual void destroy_texture(const gui::TextureHandle& handle);


public:
	int track_texture(render2::Texture* texture);
//	void untrack_texture(render2::Texture* texture);

	gui::TextureHandle texture_to_handle(render2::Texture* texture);
	render2::Texture* handle_to_texture(const gui::TextureHandle& handle);
};

StandaloneResourceCache::StandaloneResourceCache() :
	textures(0)
{

}

void StandaloneResourceCache::clear()
{
	for (FontByPathSet::Iterator it = font_handle_by_path.begin(); it != font_handle_by_path.end(); ++it)
	{
		const gui::FontHandle& handle = it.value();
		render2::font::Handle fonthandle(handle);
	}

	textures.clear();

	handle_by_texture.clear();
	font_handle_by_path.clear();
}

gui::FontHandle StandaloneResourceCache::create_font(const char* filename, size_t pixel_size)
{
	Array<unsigned char> fontdata;
	core::filesystem::instance()->virtual_load_file(fontdata, filename);
	render2::font::Handle fonthandle = render2::font::load_from_memory(&fontdata[0], fontdata.size(), pixel_size);
	assert(fonthandle.is_valid());

	gui::FontHandle handle(fonthandle);

	// we need to track the texture for looking during rendering
	render2::Texture* texture = render2::font::get_font_texture(fonthandle);
	track_texture(texture);

	return handle;
}

void StandaloneResourceCache::destroy_font(const gui::FontHandle& handle)
{
	assert(0);
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
		render2::Pipeline* gui_pipeline;
		render2::Pipeline* font_pipeline;
		render2::Texture* white_texture;

		glm::mat4 modelview_matrix;
		glm::mat4 projection_matrix;
		unsigned int diffuse_texture;

		StandaloneResourceCache& resource_cache;
	public:

		GUIRenderer(StandaloneResourceCache& cache) :
			resource_cache(cache)
		{}

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
		virtual void font_metrics(const gui::FontHandle& handle, size_t& height, int& ascender, int& descender);
		virtual size_t font_draw(const gui::FontHandle& handle, const char* string, const gui::Rect& bounds, const gui::Color& color, gui::render::Vertex* buffer, size_t buffer_size);
		virtual size_t font_count_vertices(const gui::FontHandle& handle, const char* string);
		virtual gui::TextureHandle font_get_texture(const gui::FontHandle& handle);
		virtual gui::FontResult font_fetch_texture(const gui::FontHandle& handle, gui::TextureHandle& texture);
		virtual void draw_command_lists(gui::render::CommandList** command_lists, size_t total_lists, Array<gui::render::Vertex>& vertex_buffer);

	}; // GUIRenderer

	void GUIRenderer::increment_depth()
	{
		current_depth += 1.0f;
	}

	void GUIRenderer::startup(gui::Compositor* compositor)
	{
		this->compositor = compositor;

		this->vertex_buffer = device->create_vertex_buffer(MAX_VERTICES*sizeof(GUIVertex));

		// standard gui pipeline
		render2::PipelineDescriptor desc;
		desc.shader = device->create_shader("gui");
		desc.vertex_description.add("in_position", render2::VD_FLOAT, 2);
		desc.vertex_description.add("in_color", render2::VD_FLOAT, 4);
		desc.vertex_description.add("in_uv", render2::VD_FLOAT, 2);
		desc.input_layout = device->create_input_layout(desc.vertex_description, desc.shader);
		desc.enable_blending = true;
		desc.blend_source = render2::BlendOp::SourceAlpha;
		desc.blend_destination = render2::BlendOp::OneMinusSourceAlpha;
		gui_pipeline = device->create_pipeline(desc);


		// font pipeline
		render2::PipelineDescriptor fontdesc;
		fontdesc.shader = device->create_shader("font");
		fontdesc.vertex_description.add("in_position", render2::VD_FLOAT, 2);
		fontdesc.vertex_description.add("in_color", render2::VD_FLOAT, 4);
		fontdesc.vertex_description.add("in_uv", render2::VD_FLOAT, 2);
		fontdesc.input_layout = device->create_input_layout(fontdesc.vertex_description, fontdesc.shader);
		fontdesc.enable_blending = true;
		fontdesc.blend_source = render2::BlendOp::SourceAlpha;
		fontdesc.blend_destination = render2::BlendOp::OneMinusSourceAlpha;
		font_pipeline = device->create_pipeline(fontdesc);

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
		device->destroy_pipeline(gui_pipeline);
		device->destroy_pipeline(font_pipeline);
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
		render2::font::Handle fonthandle = render2::font::load_from_memory(&fontdata[0], fontdata.size(), 16);
		assert(fonthandle.is_valid());
		handle = gui::FontHandle(fonthandle);
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
		glm::vec2 bounds_min, bounds_max;
		render2::font::get_string_metrics(render2::font::Handle(handle), string, bounds_min, bounds_max);

		bounds.set(bounds_min.x, bounds_min.y, bounds_max.x, bounds_max.y);
		return gui::FontResult_Success;
	}

	void GUIRenderer::font_metrics(const gui::FontHandle& handle, size_t& height, int& ascender, int& descender)
	{
		render2::font::Metrics metrics;
		render2::font::get_font_metrics(render2::font::Handle(handle), metrics);

		height = metrics.height;
		ascender = metrics.ascender;
		descender = metrics.descender;
	}

	size_t GUIRenderer::font_draw(const gui::FontHandle& handle, const char* string, const gui::Rect& bounds, const gui::Color& color, gui::render::Vertex* buffer, size_t buffer_size)
	{
		Array<render2::font::FontVertex> vertices;
		render2::font::Handle font_handle(handle);
		render2::font::draw_string(font_handle, vertices, string, core::Color(color.r(), color.g(), color.b(), color.a()));

		// todo: this seems counter-intuitive
		// copy back to the buffer
		for (size_t index = 0; index < vertices.size(); ++index)
		{
			render2::font::FontVertex& v = vertices[index];
			gui::render::Vertex& out = buffer[index];
			out.x = v.position.x + bounds.origin.x;
			out.y = v.position.y + bounds.origin.y;
			out.uv[0] = v.uv.x;
			out.uv[1] = v.uv.y;
			out.color = color;
		}

		return vertices.size();
	}

	size_t GUIRenderer::font_count_vertices(const gui::FontHandle& handle, const char* string)
	{
		return core::str::len(string) * 6;
	}

	gui::TextureHandle GUIRenderer::font_get_texture(const gui::FontHandle& handle)
	{
		render2::font::Handle font_handle(handle);
		render2::Texture* texture = render2::font::get_font_texture(font_handle);
		assert(texture);


		gui::TextureHandle th(1);
		return th;
	}

	gui::FontResult GUIRenderer::font_fetch_texture(const gui::FontHandle &handle, gui::TextureHandle &texture)
	{
		return gui::FontResult_Failed;
	}

	void GUIRenderer::draw_command_lists(gui::render::CommandList** command_lists, size_t total_lists, Array<gui::render::Vertex>& vertex_buffer)
	{
		size_t total_vertices = vertex_buffer.size();

		// temp limit
		assert(total_vertices < MAX_VERTICES);
		projection_matrix = glm::ortho(0.0f, (float)this->compositor->width, (float)this->compositor->height, 0.0f, -1.0f, 1.0f);

//		device->buffer_resize(vertex_buffer, sizeof(GUIVertex) * total_vertices);

		diffuse_texture = 0;
		gui_pipeline->constants().set("projection_matrix", &projection_matrix);
		gui_pipeline->constants().set("diffuse", &diffuse_texture);

		font_pipeline->constants().set("projection_matrix", &projection_matrix);
		font_pipeline->constants().set("diffuse", &diffuse_texture);

		assert(total_lists > 0);

		GUIVertex vertices[MAX_VERTICES];
		memset(vertices, 0, sizeof(GUIVertex)*MAX_VERTICES);

		// loop through all vertices in the source vertex_buffer
		// and convert them to our buffer
		for (size_t index = 0; index < total_vertices; ++index)
		{
			gui::render::Vertex* gv = &vertex_buffer[index];
			GUIVertex& vt = vertices[index];
			vt.set_position(gv->x, gv->y);
			vt.set_color(gv->color.r()/255.0f, gv->color.g()/255.0f, gv->color.b()/255.0f, gv->color.a()/255.0f);
			vt.set_uv(gv->uv[0], gv->uv[1]);
		}

		device->buffer_upload(this->vertex_buffer, vertices, sizeof(GUIVertex)*total_vertices);

		size_t command_index = 0;
		for (size_t index = 0; index < total_lists; ++index)
		{
			gui::render::CommandList* commandlist = command_lists[index];


			// setup the pass and queue the draw
			render2::Pass pass;
			pass.target = device->default_render_target();
			pass.clear_color = false;
			pass.clear_depth = false;

			render2::CommandQueue* queue = device->create_queue(pass);
			render2::CommandSerializer* serializer = device->create_serializer(queue);

			serializer->vertex_buffer(this->vertex_buffer);

			for (gui::render::Command& command : commandlist->commands)
			{
				render2::Pipeline* pipeline;
				render2::Texture* texture_pointer = white_texture;

				if (command.type == gui::render::CommandType_Generic)
				{
					pipeline = gui_pipeline;
				}
				else if (command.type == gui::render::CommandType_Font)
				{
					pipeline = font_pipeline;
				}
				else
				{
					// Unable to render this command type
					assert(0);
				}

				serializer->pipeline(pipeline);

				if (!command.texture.is_valid())
				{
					// no valid texture; use default white
					serializer->texture(white_texture, 0);
				}
				else
				{
					// valid texture; look it up
					texture_pointer = resource_cache.handle_to_texture(command.texture);
					serializer->texture(texture_pointer, 0);
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
	gui::Label* label;
	experimental::GUIRenderer renderer;
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
//		gui::FontHandle dev_font = resource_cache.create_font("fonts/Arial Unicode.ttf", 16);
//		gui::FontHandle dev_font = resource_cache.create_font("fonts/Cantarell-Regular.ttf", 16);
		gui::FontHandle dev_font = resource_cache.create_font("fonts/7x5.ttf", 8);



		assert(dev_font.is_valid());

		// setup the framerate graph
		graph = new gui::Graph(root);
		graph->set_bounds(width-250, 0, 250, 100);
		graph->set_font(dev_font);
		graph->set_background_color(gui::Color(60, 60, 60, 255));
		graph->set_foreground_color(gui::Color(255, 255, 255, 255));
		graph->create_samples(100, 1);
		graph->configure_channel(0, gui::Color(0, 255, 0, 255));
		graph->set_range(0.0f, 33.3f);
		graph->enable_baseline(true, 16.6f, gui::Color(255, 0, 255, 255));

		label = new gui::Label(root);
		label->set_background_color(gui::Color(32, 32, 32));
		label->set_foreground_color(gui::Color(0, 255, 0));
		label->set_bounds(50, 75, 100, 100);
		label->set_font(dev_font);
		label->set_text("AB");

		{
			gui::Label* label = new gui::Label(root);
			label->set_background_color(gui::Color(0, 32, 0));
			label->set_foreground_color(gui::Color(255, 0, 0));
			label->set_bounds(150, 175, 100, 100);
			label->set_font(dev_font);
			label->set_text("B");
		}
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
			newgame->set_font(dev_font);
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