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
#include <renderer/renderer.h>
#include <renderer/font.h>
#include <renderer/guirenderer.h>
#include <renderer/standaloneresourcecache.h>

#include <runtime/core.h>
#include <runtime/logging.h>

#include <platform/platform.h>
#include <platform/window.h>
#include <platform/kernel.h>
#include <platform/input.h>

#include <core/hashset.h>
#include <core/stackstring.h>
#include <core/fixedarray.h>

#include <runtime/filesystem.h>

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
	class Timeline : public Panel
	{
	public:
		Timeline(Panel* parent)
			: Panel(parent)
			, left_margin(0)
			, frame_width_pixels(0.0f)
			, current_frame(0)
		{
			flags |= Flag_CursorEnabled;
			set_name("Timeline");
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
		}

//		virtual void update(gui::Compositor* compositor, float delta_seconds) override;
		virtual void render(gui::Compositor* compositor, gui::Renderer* renderer, gui::render::CommandList& render_commands) override
		{
			// assuming a horizontal timeline
			if (frame_width_pixels == 0)
			{
				// recompute the distance here
				frame_width_pixels = (bounds.size.width / (float)total_frames);
			}

			assert(frame_width_pixels > 0);

			// draw the background
			render_commands.add_rectangle(geometry[0], geometry[1], geometry[2], geometry[3], gui::render::WhiteTexture, gui::Color(64, 64, 64, 255));

			// add a top rule line to separate this panel
			render_commands.add_line(geometry[0], geometry[3], Color(0, 0, 0, 255), 1.0f);

			Rect frame;
			get_screen_bounds(frame);

			float origin_x = frame.origin.x + left_margin;
			float origin_y = frame.origin.y + 1.0f;

			// center the individual frames
			Rect block;
			block.set(origin_x, origin_y, 1.0f, frame.size.height-2);

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
				render_commands.add_rectangle(points[0], points[1], points[2], points[3], gui::render::WhiteTexture, Color(96, 96, 96, 255));

				block.origin.x += frame_width_pixels;
			}

			// draw the current frame as a highlighted region
			Point region[4];

			float offset = origin_x + (current_frame * frame_width_pixels);

			region[0].x = offset;
			region[0].y = origin_y;

			region[1].x = offset + frame_width_pixels;
			region[1].y = origin_y;

			region[2].x = offset + frame_width_pixels;
			region[2].y = origin_y + block.size.height;

			region[3].x = offset;
			region[3].y = origin_y + block.size.height;

			Color scrubber_highlight(255, 128, 0, 32);
			Color scrubber_outline(255, 128, 0, 192);

			// draw the main highlight fill
			render_commands.add_rectangle(region[0], region[1], region[2], region[3], gui::render::WhiteTexture, scrubber_highlight);

			// draw the outline
			render_commands.add_line(region[0], region[1], scrubber_outline);
			render_commands.add_line(region[1], region[2], scrubber_outline);
			render_commands.add_line(region[2], region[3], scrubber_outline);
			render_commands.add_line(region[3], region[0], scrubber_outline);
		}


		void set_frame_range(int lower_frame_limit, int upper_frame_limit)
		{
			lower_limit = lower_frame_limit;
			upper_limit = upper_frame_limit;

			assert(upper_limit > lower_limit);
			total_frames = (upper_limit - lower_limit);

			// force a recalculate on the next render call
			frame_width_pixels = 0;
		}

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
	};
}



class EditorKernel : public kernel::IKernel,
public kernel::IEventListener<kernel::KeyboardEvent>,
public kernel::IEventListener<kernel::MouseEvent>,
public kernel::IEventListener<kernel::SystemEvent>
{
private:
	bool active;
	platform::window::NativeWindow* main_window;
	
	render2::Device* device;
	render2::Buffer* vertex_buffers[2];

	render2::Pipeline* pipeline;
	render2::Pipeline* texture_pipeline;

	glm::mat4 modelview_matrix;
	glm::mat4 projection_matrix;

	render2::font::Handle font;
//	GLsync fence;

	gui::Compositor* compositor;
	GUIRenderer* gui_renderer;
	::renderer::StandaloneResourceCache resource_cache;

public:
	EditorKernel()
		: active(true)
		, compositor(nullptr)
		, gui_renderer(nullptr)
	{
	}
	
	virtual ~EditorKernel() {}
	
	virtual bool is_active() const { return active; }
	virtual void set_active(bool isactive) { active = isactive; }
	
	virtual void event(kernel::SystemEvent& event)
	{
//		if (event.subtype == kernel::WindowGainFocus)
//		{
//			LOGV("window gained focus\n");
//		}
//		else if (event.subtype == kernel::WindowLostFocus)
//		{
//			LOGV("window lost focus\n");
//		}
//		else if (event.subtype == kernel::WindowResized)
//		{
//			LOGV("resolution_changed %i %i\n", event.render_width, event.render_height);
//		}

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
				compositor->cursor_button(input_to_gui[event.button], event.is_down);
			}
			else if (event.subtype == kernel::MouseWheelMoved)
			{
				LOGV("wheel direction: %i\n", event.wheel_direction);
			}
		}
//		if (event.subtype == kernel::MouseWheelMoved)
//		{
//			LOGV("wheel direction: %i\n", event.wheel_direction);
//		}
//		else if (event.subtype == kernel::MouseMoved)
//		{
//			LOGV("mouse moved: %i %i [%i %i]\n", event.mx, event.my, event.dx, event.dy);
//		}
//		else if (event.subtype == kernel::MouseButton)
//		{
//			LOGV("mouse button: %s, %i -> %s\n", event.is_down ? "Yes" : "No", event.button, input::mouse_button_name(event.button));
//		}
//		else
//		{
//			LOGV("mouse event: %i\n", event.subtype);
//		}
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
			params["multisample"] = "4";

			// set opengl specific options
			params["rendering_backend"] = "opengl";
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

			// create the texture pipeline
			texture_pipeline = nullptr;
			if (1)
			{
				render2::PipelineDescriptor desc;
				desc.shader = device->create_shader("vertexcolortexture");
				desc.vertex_description.add("in_position", render2::VD_FLOAT, 3);
				desc.vertex_description.add("in_color", render2::VD_FLOAT, 4);
				desc.vertex_description.add("in_uv", render2::VD_FLOAT, 2);
				desc.input_layout = device->create_input_layout(desc.vertex_description, desc.shader);
				desc.enable_blending = true;
				desc.blend_source = render2::BlendOp::SourceAlpha;
				desc.blend_destination = render2::BlendOp::OneMinusSourceAlpha;
				texture_pipeline = device->create_pipeline(desc);
			}


//			populate_textured_buffer();

		}
		
		// setup editor assets / content paths
		{
//			fs->add_virtual_path("editor/assets");
		}

		render2::font::startup(device);

		Array<unsigned char> data;
		core::filesystem::instance()->virtual_load_file(data, "fonts/debug.ttf");
		font = render2::font::load_from_memory(&data[0], data.size(), 16);

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


			gui::Panel* root = new gui::Panel(compositor);
			root->set_bounds(100, 100, 300, 400);
			root->set_background_color(gui::Color(255, 0, 0, 255));

			gui::Timeline* timeline = new gui::Timeline(compositor);
			timeline->set_bounds(0, 550, 800, 50);

			timeline->set_frame_range(0, 55);
		}
#endif
		kernel::parameters().step_interval_seconds = (1.0f/50.0f);

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

		uint32_t sampler = 0;
		texture_pipeline->constants().set("modelview_matrix", &modelview_matrix);
		texture_pipeline->constants().set("projection_matrix", &projection_matrix);
		texture_pipeline->constants().set("diffuse", &sampler);

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
//		serializer->vertex_buffer(vertex_buffers[0]);
//		serializer->draw(0, 3);
		device->queue_buffers(queue, 1);
		device->destroy_serializer(serializer);


		// draw a test quad with the font
#if 0
		{
			render2::Pass render_pass;
			render_pass.target = device->default_render_target();
			render_pass.clear_color = false;

			render2::CommandQueue* queue = device->create_queue(render_pass);
			render2::CommandSerializer* serializer = device->create_serializer(queue);
			serializer->pipeline(texture_pipeline);
			serializer->vertex_buffer(vertex_buffers[1]);
			serializer->texture(render2::font::get_font_texture(font), 0);
			serializer->draw(0, 6);
			device->queue_buffers(queue, 1);
			device->destroy_serializer(serializer);
		}
#endif

		Array<render2::font::FontVertex> fontvertices;
		if (font.is_valid())
		{

			glm::mat2 transform(1.0f);
			const float radians = mathlib::degrees_to_radians(-45);
			transform = glm::mat2(
							 cos(radians), -sin(radians),
							 sin(radians), cos(radians)
							 );

			const char buffer[] = "The quick brown fox jumps over the lazy dog";


			render2::font::draw_string(font, fontvertices, buffer, core::Color(0, 255, 255, 255));


			glm::vec2 minres, maxres;
			render2::font::get_string_metrics(font, buffer, minres, maxres);


			render2::Pass render_pass;
			render_pass.target = device->default_render_target();
			render_pass.color(0, 0, 0, 1.0f);
			render_pass.clear_color = false;

//			render2::CommandQueue* queue = device->create_queue(render_pass);
//			render2::CommandSerializer* serializer = device->create_serializer(queue);
//			serializer->pipeline(texture_pipeline);
//			serializer->vertex_buffer(vertex_buffers[1]);
//			serializer->texture(render2::font::get_font_texture(font), 0);
//
//
//			populate_textured_buffer();
//			serializer->draw(0, 6);
//
//			TexturedVertex* v = (TexturedVertex*)device->buffer_lock(vertex_buffers[1]);
//			v+=6;

//			glm::vec2 baseline(64.0f, 120.0f);
			glm::vec2 baseline(window_frame.width/2.0f - maxres.x/2.0f, 120.0f);

			// draw background highlight
#if 0
			baseline.y -= maxres.y;

			v->set_position(baseline.x, baseline.y+maxres.y, 0);
			v->set_color(0, 0, 0, 1);
			v->set_uv(0, 0);
			++v;

			v->set_position(baseline.x+maxres.x, baseline.y+maxres.y, 0);
			v->set_color(0, 0, 0, 1);
			v->set_uv(1, 0);
			++v;

			v->set_position(baseline.x+maxres.x, baseline.y, 0);
			v->set_color(0, 0, 0, 1);
			v->set_uv(1, 1);
			++v;

			v->set_position(baseline.x+maxres.x, baseline.y, 0);
			v->set_color(0, 0, 0, 1);
			v->set_uv(1, 1);
			++v;

			v->set_position(baseline.x, baseline.y, 0);
			v->set_color(0, 0, 0, 1);
			v->set_uv(0, 1);
			++v;

			v->set_position(baseline.x, baseline.y+maxres.y, 0);
			v->set_color(0, 0, 0, 1);
			v->set_uv(0, 0);
			++v;

			baseline.y += maxres.y;
#endif
			
//			size_t index = 0;
//			for (auto& vertex : fontvertices)
//			{
////				LOGV("[%i] pos [%2.2f, %2.2f]\n", index, vertex.position.x, vertex.position.y);
////				LOGV("[%i] uv [%2.2f, %2.2f]\n", index, vertex.uv.x, vertex.uv.y);
////				vertex.position = transform * vertex.position;
//				v->set_position(vertex.position.x + baseline.x, vertex.position.y + baseline.y, 0);
//				v->set_color(
//							 vertex.color.r/255.0f,
//							 vertex.color.g/255.0f,
//							 vertex.color.b/255.0f,
//							 vertex.color.a/255.0f
//							 );
//				v->set_uv(vertex.uv.x, vertex.uv.y);
//				++v;
//				++index;
//			}

//			serializer->draw(6, fontvertices.size() + 6);

//			device->buffer_unlock(vertex_buffers[1]);


			device->queue_buffers(queue, 1);
			device->destroy_serializer(serializer);
		}


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
		// compositor will cleanup children
		delete compositor;

		// shutdown the gui
		MEMORY_DELETE(gui_renderer, core::memory::global_allocator());

		// explicitly clear the resource cache or else the allocator will
		// detect leaks.
		resource_cache.clear();

		render2::font::shutdown();

		// shutdown the render device
		device->destroy_buffer(vertex_buffers[0]);

		if (vertex_buffers[1])
			device->destroy_buffer(vertex_buffers[1]);

		device->destroy_pipeline(pipeline);

		if (texture_pipeline)
			device->destroy_pipeline(texture_pipeline);
		
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
