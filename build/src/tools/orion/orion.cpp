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

	struct TexturedVertex : public MyVertex
	{
		float uv[2];

		void set_uv(float u, float v)
		{
			uv[0] = u;
			uv[1] = v;
		}
	};

public:
	EditorKernel() :
		active(true)
	{
	}
	
	virtual ~EditorKernel() {}
	
	virtual bool is_active() const { return active; }
	virtual void set_active(bool isactive) { active = isactive; }
	
	virtual void event(kernel::SystemEvent& event)
	{
		if (event.subtype == kernel::WindowGainFocus)
		{
			LOGV("window gained focus\n");
		}
		else if (event.subtype == kernel::WindowLostFocus)
		{
			LOGV("window lost focus\n");
		}
		else if (event.subtype == kernel::WindowResized)
		{
			LOGV("resolution_changed %i %i\n", event.render_width, event.render_height);
		}
	}
	
	virtual void event(kernel::MouseEvent& event)
	{
		if (event.subtype == kernel::MouseWheelMoved)
		{
			LOGV("wheel direction: %i\n", event.wheel_direction);
		}
		else if (event.subtype == kernel::MouseMoved)
		{
			LOGV("mouse moved: %i %i [%i %i]\n", event.mx, event.my, event.dx, event.dy);
		}
		else if (event.subtype == kernel::MouseButton)
		{
			LOGV("mouse button: %s, %i -> %s\n", event.is_down ? "Yes" : "No", event.button, input::mouse_button_name(event.button));
		}
		else
		{
			LOGV("mouse event: %i\n", event.subtype);
		}
	}

	void populate_textured_buffer()
	{
		const size_t width = 256;
		const size_t height = 256;
		glm::vec2 offset(64, 128);

		TexturedVertex* quad = (TexturedVertex*)device->buffer_lock(vertex_buffers[1]);
		quad[0].set_position(offset.x, offset.y+height, 0);
		quad[0].set_color(1, 1, 1, 1);
		quad[0].set_uv(0, 0);

		quad[1].set_position(offset.x+width, offset.y+height, 0);
		quad[1].set_color(1, 1, 1, 1);
		quad[1].set_uv(1, 0);

		quad[2].set_position(offset.x+width, offset.y, 0);
		quad[2].set_color(1, 1, 1, 1);
		quad[2].set_uv(1, 1);

		quad[3] = quad[2];

		quad[4].set_position(offset.x, offset.y, 0);
		quad[4].set_color(1, 1, 1, 1);
		quad[4].set_uv(0, 1);

		quad[5].set_position(offset.x, offset.y+height, 0);
		quad[5].set_color(1, 1, 1, 1);
		quad[5].set_uv(0, 0);

//		device->buffer_upload(vertex_buffers[1], &quad[0], sizeof(TexturedVertex)*6);
		device->buffer_unlock(vertex_buffers[1]);
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

			PLATFORM_LOG(LogMessageType::Info, "total screens: %zu\n", platform::window::screen_count());
			
			for (size_t screen = 0; screen < platform::window::screen_count(); ++screen)
			{
				platform::window::Frame frame = platform::window::screen_frame(screen);
				PLATFORM_LOG(LogMessageType::Info, "screen rect: %zu, origin: %2.2f, %2.2f; resolution: %2.2f x %2.2f\n", screen, frame.x, frame.y, frame.width, frame.height);
			}
			
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

			platform::window::Frame window_frame = platform::window::get_frame(main_window);
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

			size_t total_bytes = sizeof(MyVertex) * 6;
			vertex_buffers[0] = device->create_vertex_buffer(total_bytes);
			vertex_buffers[1] = device->create_vertex_buffer(sizeof(render2::font::FontVertex)*4096);

#if 1
			MyVertex* vertex = reinterpret_cast<MyVertex*>(device->buffer_lock(vertex_buffers[0]));
			
//			MyVertex vertex[4];
			
			vertex[0].set_position(0, window_frame.height, 0);
			vertex[0].set_color(1.0f, 0.0f, 0.0f, 1.0f);
			
			vertex[1].set_position(window_frame.width, window_frame.height, 0);
			vertex[1].set_color(0.0f, 1.0f, 0.0f, 1.0f);
			
			vertex[2].set_position(window_frame.width/2.0f, 0, 0);
			vertex[2].set_color(0.0f, 0.0f, 1.0f, 1.0f);
			
			vertex[3].set_position(0, 0, 0);
			vertex[3].set_color(0.0f, 1.0f, 1.0f, 1.0f);

			device->buffer_unlock(vertex_buffers[0]);
#endif


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
		

		platform::window::Frame window_frame = platform::window::get_frame(main_window);

		modelview_matrix = glm::mat4(1.0f);
		projection_matrix = glm::ortho(0.0f, window_frame.width, window_frame.height, 0.0f, -1.0f, 1.0f);
		pipeline->constants().set("modelview_matrix", &modelview_matrix);
		pipeline->constants().set("projection_matrix", &projection_matrix);

		uint32_t sampler = 0;
		texture_pipeline->constants().set("modelview_matrix", &modelview_matrix);
		texture_pipeline->constants().set("projection_matrix", &projection_matrix);
		texture_pipeline->constants().set("diffuse", &sampler);

		value = 0.25f;

		render2::Pass render_pass;
		render_pass.target = device->default_render_target();
		render_pass.color(value, value, value, 1.0f);
		render_pass.clear_color = true;
		render_pass.clear_depth = true;
		render_pass.depth_test = false;

		render2::CommandQueue* queue = device->create_queue(render_pass);
		render2::CommandSerializer* serializer = device->create_serializer(queue);
		
		serializer->pipeline(pipeline);
		serializer->vertex_buffer(vertex_buffers[0]);
		serializer->draw(0, 3);
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

			render2::CommandQueue* queue = device->create_queue(render_pass);
			render2::CommandSerializer* serializer = device->create_serializer(queue);
			serializer->pipeline(texture_pipeline);
			serializer->vertex_buffer(vertex_buffers[1]);
			serializer->texture(render2::font::get_font_texture(font), 0);


			populate_textured_buffer();
			serializer->draw(0, 6);

			TexturedVertex* v = (TexturedVertex*)device->buffer_lock(vertex_buffers[1]);
			v+=6;

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
			
			size_t index = 0;
			for (auto& vertex : fontvertices)
			{
//				LOGV("[%i] pos [%2.2f, %2.2f]\n", index, vertex.position.x, vertex.position.y);
//				LOGV("[%i] uv [%2.2f, %2.2f]\n", index, vertex.uv.x, vertex.uv.y);
//				vertex.position = transform * vertex.position;
				v->set_position(vertex.position.x + baseline.x, vertex.position.y + baseline.y, 0);
				v->set_color(
							 vertex.color.r/255.0f,
							 vertex.color.g/255.0f,
							 vertex.color.b/255.0f,
							 vertex.color.a/255.0f
							 );
				v->set_uv(vertex.uv.x, vertex.uv.y);
				++v;
				++index;
			}

			serializer->draw(6, fontvertices.size() + 6);

			device->buffer_unlock(vertex_buffers[1]);





			device->queue_buffers(queue, 1);
			device->destroy_serializer(serializer);
		}


		platform::window::activate_context(main_window);

		device->submit();

		platform::window::swap_buffers(main_window);

//		glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 2000);
	}
	
	
	virtual void shutdown()
	{
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
