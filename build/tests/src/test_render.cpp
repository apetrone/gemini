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

#include <core/argumentparser.h>
#include <core/logging.h>

#include <runtime/assets.h>
#include <runtime/filesystem.h>
#include <runtime/runtime.h>
#include <runtime/asset_library.h>

#include <platform/input.h>
#include <platform/kernel.h>
#include <platform/platform.h>
#include <platform/window.h>

#include <renderer/font_library.h>
#include <renderer/renderer.h>
#include <renderer/vertexbuffer.h>

#include <renderer/color.h>

#include <assert.h>

using namespace gemini;

const float RENDER_TEST_WAIT_SECONDS = 1.0f;

// ---------------------------------------------------------------------
// Color
// ---------------------------------------------------------------------
UNITTEST(Color)
{
	Color red(1.0f, 0, 0, 1.0f);

	Color temp;
	float red_float[4] = {1.0f, 0.0f, 0.0f, 1.0f};
	temp = Color::from_float_pointer(red_float, 4);
	TEST_ASSERT(temp == red, from_float_pointer);

	uint32_t u32_color = red.as_uint32();
	Color int_color = Color::from_int(u32_color);
	TEST_ASSERT(int_color == red, from_int);

	Color ubyte_test = Color::from_rgba(0, 128, 255, 32);
	unsigned char ubyte[] = {0, 128, 255, 32};
	Color ubyte_color = Color::from_ubyte(ubyte);

	TEST_ASSERT(ubyte_color == ubyte_test, from_ubyte);
}

#define TEST_RENDER_GRAPHICS 1

// ---------------------------------------------------------------------
// TestRender
// ---------------------------------------------------------------------
class TestRender : public kernel::IKernel,
	public kernel::IEventListener<kernel::KeyboardEvent>,
	public kernel::IEventListener<kernel::MouseEvent>,
	public kernel::IEventListener<kernel::SystemEvent>
{
	struct ConstantData
	{
		glm::mat4 modelview_matrix;
		glm::mat4 projection_matrix;
		unsigned int texture_unit;
	};

	struct TestRenderState
	{
		glm::mat4 modelview_matrix;
		glm::mat4 projection_matrix;
		unsigned int diffuse;
		unsigned int mask_texture;

		AssetHandle font_handle;
		platform::window::NativeWindow* native_window;
		platform::window::NativeWindow* other_window;

		render2::Device* device;

		render2::Pipeline* pipeline; 			// pipeline for untextured geometry
		render2::Pipeline* texture_pipeline; 	// pipeline for textured geometry
		render2::Pipeline* font_pipeline; 		// pipeline for font rendering
		render2::Pipeline* line_pipeline; 		// pipeline for rendering lines
		render2::Pipeline* multi_pipeline;		// pipeline for multi-texturing

		render2::Buffer* vertex_buffer;
		render2::Buffer* textured_buffer;
		render2::Buffer* font_buffer;
		render2::Buffer* line_buffer;

		render2::Texture* checker;
		render2::Texture* notexture;

		size_t total_font_vertices;

		TestRenderState() :
			native_window(nullptr),
			other_window(nullptr),
			device(nullptr),
			pipeline(nullptr),
			texture_pipeline(nullptr),
			font_pipeline(nullptr),
			line_pipeline(nullptr),
			multi_pipeline(nullptr),
			vertex_buffer(nullptr),
			textured_buffer(nullptr),
			font_buffer(nullptr),
			line_buffer(nullptr),
			checker(nullptr),
			notexture(nullptr),
			total_font_vertices(0)
		{
		}
	};

	typedef void (*test_state_callback)(TestRenderState& state);
	struct RenderTest
	{
		core::StackString<128> name;
		test_state_callback render_callback;
	};

public:
	virtual void event(kernel::KeyboardEvent& event)
	{
		if (event.is_down)
		{
			if (event.key == gemini::BUTTON_ESCAPE)
			{
				kernel::instance()->set_active(false);
			}
			else if (event.key == gemini::BUTTON_SPACE)
			{
				platform::window::set_cursor(center.x, center.y);
			}
		}
	}

	virtual void event(kernel::MouseEvent& event)
	{
//		LOGV("mouse move: %i %i %i\n", event.subtype, event.mx, event.my);
	}

	virtual void event(kernel::SystemEvent& event)
	{
		switch(event.subtype)
		{
			case kernel::WindowResized:
				LOGV("window resized: %i x %i\n", event.render_width, event.render_height);
				if (state.device)
				{
					state.device->backbuffer_resized(event.render_width, event.render_height);
				}
				break;
			default:
				break;
		}
	}
public:

	static void render_stage1(TestRenderState& state)
	{
		// sanity check
		assert(state.device);
		assert(state.pipeline);
		assert(state.vertex_buffer);

		render2::Pass render_pass;
		render_pass.target = state.device->default_render_target();
		render_pass.color(0.0f, 0.0f, 0.0f, 1.0f);
		render_pass.clear_color = true;
		render_pass.clear_depth = true;
		render_pass.depth_test = false;

		// create a command queue
		render2::CommandQueue* queue = state.device->create_queue(render_pass);

		// create a command serializer for the queue
		render2::CommandSerializer* serializer = state.device->create_serializer(queue);
		assert(serializer);

		// color-based triangle
		serializer->pipeline(state.pipeline);
		serializer->vertex_buffer(state.vertex_buffer);
//		serializer->draw_indexed_primitives(index_buffer, 3);
		serializer->draw(0, 3);

		// procedurally-generated textured triangle
		serializer->pipeline(state.texture_pipeline);
		serializer->vertex_buffer(state.textured_buffer);
		serializer->texture(state.checker, 0);
		serializer->draw(0, 3);

		// quad; texture loaded from disk (notexture.png)
		serializer->texture(state.notexture, 0);
		serializer->draw(3, 6);

		// quad; font texture
//		serializer->texture(font_texture(state.font_handle), 0);
//		serializer->draw(9, 6);

		serializer->pipeline(state.font_pipeline);
		serializer->vertex_buffer(state.font_buffer);
		serializer->texture(font_texture(state.font_handle), 0);
		serializer->draw(0, state.total_font_vertices);

		// horizontal line test
		serializer->pipeline(state.line_pipeline);
		serializer->vertex_buffer(state.line_buffer);
		serializer->draw(0, 4);

		// queue the buffer with our device
		state.device->destroy_serializer(serializer);
		state.device->queue_buffers(queue, 1);

		// submit the queues to the GPU
		platform::window::activate_context(state.native_window);
		state.device->submit();
		platform::window::swap_buffers(state.native_window);

		if (state.other_window)
		{
			platform::window::activate_context(state.other_window);
			platform::window::swap_buffers(state.other_window);
		}
	}

	static void render_stage2(TestRenderState& state)
	{
		// sanity check
		assert(state.device);
		assert(state.pipeline);
		assert(state.vertex_buffer);

		render2::Pass render_pass;
		render_pass.target = state.device->default_render_target();
		render_pass.color(0.0f, 0.0f, 0.0f, 1.0f);
		render_pass.clear_color = true;
		render_pass.clear_depth = true;
		render_pass.depth_test = false;

		// create a command queue
		render2::CommandQueue* queue = state.device->create_queue(render_pass);

		// create a command serializer for the queue
		render2::CommandSerializer* serializer = state.device->create_serializer(queue);
		assert(serializer);

		// procedurally-generated textured triangle
		serializer->pipeline(state.multi_pipeline);
		serializer->vertex_buffer(state.textured_buffer);
		serializer->texture(state.checker, 0);
		serializer->texture(state.notexture, 1);
		serializer->draw(0, 3);

		// quad; texture loaded from disk (notexture.png)

		//serializer->draw(3, 6);

		// queue the buffer with our device
		state.device->destroy_serializer(serializer);
		state.device->queue_buffers(queue, 1);

		// submit the queues to the GPU
		platform::window::activate_context(state.native_window);
		state.device->submit();
		platform::window::swap_buffers(state.native_window);
	} // render_stage2


	TestRender()
		: render_callbacks(render_allocator, 0)
	{
		state.native_window = nullptr;
		active = true;
	}

	virtual bool is_active() const { return active; }
	virtual void set_active(bool isactive) { active = isactive; }

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
			platform::PathString root_path = platform::get_program_directory();

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

		gemini::runtime_startup("arcfusion.net/gemini/test_render", custom_path_setup);
//		platform::PathString temp_path = platform::get_user_temp_directory(); // adding this line breaks Android. Yes, you read that correctly.
//		LOGV("temp_path: %s\n", temp_path());

		unittest::UnitTest::execute();

		platform::window::startup(platform::window::RenderingBackend_Default);



		size_t total_displays = platform::window::screen_count();
		LOGV("-> total screens: %lu\n", total_displays);

		for (size_t i = 0; i < total_displays; ++i)
		{
			platform::window::Frame frame = platform::window::screen_frame(i);
			LOGV("display %lu rect = %2.2f, %2.2f, %2.2f x %2.2f\n", (unsigned long)i, frame.x, frame.y, frame.width, frame.height);
		}

		// automatically shutdown after 3 seconds
		countdown = RENDER_TEST_WAIT_SECONDS;

		// create a test window
		platform::window::Parameters params;
		params.frame = platform::window::centered_window_frame(0, 512, 512);
		params.window_title = "test_render";

		state.native_window = platform::window::create(params);
		assert(state.native_window != nullptr);
		platform::window::Frame window_frame = platform::window::get_frame(state.native_window);
		LOGV("window dimensions: %2.2f %2.2f\n", window_frame.width, window_frame.height);

		platform::window::focus(state.native_window);

		if (0 && platform::window::screen_count() > 1)
		{
			params.frame = platform::window::centered_window_frame(1, 1024, 768);
			params.window_title = "other_window";
			state.other_window = platform::window::create(params);
			assert(state.other_window != nullptr);
			window_frame = platform::window::get_frame(state.other_window);
			LOGV("other window dimensions: %i %i\n", window_frame.width, window_frame.height);

			platform::window::Frame wf = platform::window::get_frame(state.other_window);

			LOGV("other_window frame: %2.2f, %2.2f, %2.2f x %2.2f\n", wf.x, wf.y, wf.width, wf.height);

			// try to center the mouse cursor in the window
			center.x = (wf.width/2.0f + wf.x);
			center.y = (wf.height/2.0f + wf.y);

			LOGV("other_window center: %2.2f, %2.2f\n", center.x, center.y);
			platform::window::set_cursor(center.x, center.y);
		}

#if TEST_RENDER_GRAPHICS
		render_allocator = memory_allocator_default(MEMORY_ZONE_RENDERER);

		// initialize render device
		render2::RenderParameters render_parameters(render_allocator);
		render_parameters["rendering_backend"] = "default";
		render_parameters["gamma_correct"] = "false";
		render_parameters["vsync"] = "true";

		state.device = render2::create_device(render_allocator, render_parameters);
		assert(state.device != nullptr);

		assets::startup(state.device, false);

		window_frame = platform::window::get_frame(state.native_window);

		// setup the pipeline
		render2::PipelineDescriptor desc;
		desc.shader = shader_load("vertexcolor");
		desc.vertex_description.add("in_position", render2::VD_FLOAT, 3); // position
		desc.vertex_description.add("in_color", render2::VD_FLOAT, 4); // color
		desc.input_layout = state.device->create_input_layout(desc.vertex_description, desc.shader);
		desc.primitive_type = render2::PrimitiveType::Triangles;
		state.pipeline = state.device->create_pipeline(desc);

		// create a vertex buffer and populate it with data
		float width = (float)window_frame.width;
		float height = (float)window_frame.height;

		state.device->init(window_frame.width, window_frame.height);

		// Draw a triangle on screen with the wide part of the base at the bottom
		// of the screen.
		size_t total_bytes = sizeof(MyVertex) * 3;
		state.vertex_buffer = state.device->create_vertex_buffer(total_bytes);
		assert(state.vertex_buffer != nullptr);
		MyVertex vertices[3];

		generate_triangle(0, vertices, glm::vec2(width, height), glm::vec2(0, 0));
		state.device->buffer_upload(state.vertex_buffer, vertices, total_bytes);

		// setup texture pipeline
		render2::PipelineDescriptor td;
		td.shader = shader_load("vertexcolortexture");
		td.vertex_description.add("in_position", render2::VD_FLOAT, 3);
		td.vertex_description.add("in_color", render2::VD_FLOAT, 4);
		td.vertex_description.add("in_uv", render2::VD_FLOAT, 2);
		td.input_layout = state.device->create_input_layout(td.vertex_description, td.shader);
		state.texture_pipeline = state.device->create_pipeline(td);

		// setup multi-texture pipeline
		render2::PipelineDescriptor md;
		md.shader = shader_load("multitexture");
		md.vertex_description.add("in_position", render2::VD_FLOAT, 3);
		md.vertex_description.add("in_color", render2::VD_FLOAT, 4);
		md.vertex_description.add("in_uv", render2::VD_FLOAT, 2);
		md.input_layout = state.device->create_input_layout(md.vertex_description, md.shader);
		state.multi_pipeline = state.device->create_pipeline(md);

		// setup font pipeline
		render2::PipelineDescriptor fd;
		fd.shader = shader_load("font");
		fd.vertex_description.add("in_position", render2::VD_FLOAT, 2);
		fd.vertex_description.add("in_color", render2::VD_FLOAT, 4);
		fd.vertex_description.add("in_uv", render2::VD_FLOAT, 2);
		fd.enable_blending = true;
		fd.blend_source = render2::BlendOp::SourceAlpha;
		fd.blend_destination = render2::BlendOp::OneMinusSourceAlpha;
		fd.input_layout = state.device->create_input_layout(td.vertex_description, fd.shader);
		state.font_pipeline = state.device->create_pipeline(fd);

		// setup line pipeline
		render2::PipelineDescriptor ld;
		ld.shader = shader_load("lines");
		ld.vertex_description.add("in_position", render2::VD_FLOAT, 3);
		ld.vertex_description.add("in_color", render2::VD_FLOAT, 4);
		ld.input_layout = state.device->create_input_layout(ld.vertex_description, ld.shader);
		ld.primitive_type = render2::PrimitiveType::Lines;
		state.line_pipeline = state.device->create_pipeline(ld);

		// setup texture vertex buffer
		const size_t TOTAL_TEXTURED_VERTICES = 15;
		total_bytes = sizeof(TexturedVertex) * TOTAL_TEXTURED_VERTICES;
		state.textured_buffer = state.device->create_vertex_buffer(total_bytes);
		TexturedVertex texverts[ TOTAL_TEXTURED_VERTICES ];
		generate_textured_triangle(0, texverts, glm::vec2(width, height), glm::vec2(width/2, 0));
		generate_textured_quad(3, texverts, glm::vec2(width, height), glm::vec2(0, height/2));
		generate_textured_quad(9, texverts, glm::vec2(width, height), glm::vec2(width/2, height/2));
		state.device->buffer_upload(state.textured_buffer, texverts, total_bytes);

		// setup constant buffer
		state.modelview_matrix = glm::mat4(1.0f);
		state.projection_matrix = glm::ortho(0.0f, width, height, 0.0f, -1.0f, 1.0f);
		state.diffuse = 0;
		state.mask_texture = 1;
		state.pipeline->constants().set("modelview_matrix", &state.modelview_matrix);
		state.pipeline->constants().set("projection_matrix", &state.projection_matrix);

		state.texture_pipeline->constants().set("modelview_matrix", &state.modelview_matrix);
		state.texture_pipeline->constants().set("projection_matrix", &state.projection_matrix);
		state.texture_pipeline->constants().set("diffuse", &state.diffuse);

		state.font_pipeline->constants().set("modelview_matrix", &state.modelview_matrix);
		state.font_pipeline->constants().set("projection_matrix", &state.projection_matrix);
		state.font_pipeline->constants().set("diffuse", &state.diffuse);

		state.line_pipeline->constants().set("modelview_matrix", &state.modelview_matrix);
		state.line_pipeline->constants().set("projection_matrix", &state.projection_matrix);

		state.multi_pipeline->constants().set("modelview_matrix", &state.modelview_matrix);
		state.multi_pipeline->constants().set("projection_matrix", &state.projection_matrix);
		state.multi_pipeline->constants().set("diffuse", &state.diffuse);
		state.multi_pipeline->constants().set("mask_texture", &state.mask_texture);

//		platform::window::show_cursor(true);
		const size_t TOTAL_FONT_VERTICES = 1024;
		state.font_buffer = state.device->create_vertex_buffer(sizeof(TexturedVertex) * TOTAL_FONT_VERTICES);


		const size_t TOTAL_LINE_VERTICES = 4;
		state.line_buffer = state.device->create_vertex_buffer(sizeof(MyVertex) * TOTAL_LINE_VERTICES);
#endif




		// ---------------------------------------------------------------------
		// texture creation
		// ---------------------------------------------------------------------
		LOGV("generating textures...\n");
		{
			// generate a texture
			image::Image checker_pattern(render_allocator);
			checker_pattern.width = 32;
			checker_pattern.height = 32;
			checker_pattern.channels = 3;
			image::generate_checker_pattern(checker_pattern, gemini::Color(1.0f, 0.0f, 1.0f), gemini::Color(0.0f, 1.0f, 0.0f));
			state.checker = state.device->create_texture(checker_pattern);
			assert(state.checker);
			LOGV("created checker_pattern texture procedurally\n");
		}


		// load a texture from file
		{
			gemini::Allocator allocator = gemini::memory_allocator_default(gemini::MEMORY_ZONE_RENDERER);

			Array<unsigned char> buffer(render_allocator);
			core::filesystem::instance()->virtual_load_file(buffer, "textures/notexture.png");
			assert(!buffer.empty());
			image::Image image = image::load_from_memory(allocator, &buffer[0], buffer.size());
			LOGV("loaded image: %i x %i, @ %i\n", image.width, image.height, image.channels);
			image.filter = image::FILTER_NONE;
			state.notexture = state.device->create_texture(image);
			assert(state.notexture);
			LOGV("loaded notexture.png successfully\n");
		}

		// load a compressed texture?

//		image::Image test_pattern;
//		test_pattern.create(32, 32, 1);
//		test_pattern.fill(gemini::Color(255, 0, 0));
//		checker = device->create_texture(test_pattern);
//		assert(checker);

		// ---------------------------------------------------------------------
		// font
		// ---------------------------------------------------------------------
		FontCreateParameters font_params;
		font_params.size_pixels = 16;
		state.font_handle = font_load("debug", false, &font_params);

		const char* text = "The quick brown fox jumps over the lazy dog.";
		Array<FontVertex> temp_vertices(render_allocator);
		const size_t text_size = core::str::len(text);

		temp_vertices.resize(font_count_vertices(text_size));
		font_draw_string(state.font_handle, &temp_vertices[0], text, text_size, gemini::Color(1.0f, 1.0f, 1.0f));

		FontMetrics metrics;
		font_metrics(state.font_handle, metrics);

		float offset[2] = {window_frame.width/2, window_frame.height/2 + metrics.max_height};

		state.total_font_vertices = temp_vertices.size();
		TexturedVertex tvf[TOTAL_FONT_VERTICES];
		for (size_t index = 0; index < temp_vertices.size(); ++index)
		{
			TexturedVertex* tv = &tvf[index];
			FontVertex* fv = &temp_vertices[index];
			tv->position[0] = fv->position.x + offset[0];
			tv->position[1] = fv->position.y + offset[1];
			tv->position[2] = 0;
			tv->color = fv->color;
			tv->uv[0] = fv->uv.x;
			tv->uv[1] = fv->uv.y;
		}
		state.device->buffer_upload(state.font_buffer, &tvf, sizeof(TexturedVertex)*TOTAL_FONT_VERTICES);

		// hit this assert if we couldn't load the font
		assert(state.font_handle != InvalidAssetHandle);

		// ---------------------------------------------------------------------
		// line buffer
		// ---------------------------------------------------------------------
		const float h_width = (width/2.0f);
		const float h_height = (height/2.0f);
		MyVertex lines[TOTAL_LINE_VERTICES];

		// horizontal red line
		lines[0].color = gemini::Color(1.0f, 0.0f, 0.0f, 1.0f);
		lines[0].set_position(h_width + 8, h_height + 32, 0);
		lines[1].color = gemini::Color(1.0f, 0.0f, 0.0f, 1.0f);
		lines[1].set_position(width - 8, h_height + 32, 0.0f);

		// vertical green line
		lines[2].color = gemini::Color(0.0f, 1.0f, 0.0f, 1.0f);
		lines[2].set_position(h_width + 8, h_height + 32, 0.0f);
		lines[3].color = gemini::Color(0.0f, 1.0f, 0.0f, 1.0f);
		lines[3].set_position(h_width + 8, height - 8, 0.0f);

		state.device->buffer_upload(state.line_buffer, &lines, sizeof(MyVertex) * TOTAL_LINE_VERTICES);

		// additional setup
		kernel::parameters().step_interval_seconds = (1.0f/50.0f);

		render_callbacks.push_back(RenderTest({ "stage1", render_stage1 }));
		render_callbacks.push_back(RenderTest({ "stage2", render_stage2 }));

		test_state = 0;
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
			params.current_frame++;
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
		countdown -= kernel::parameters().framedelta_milliseconds;

		platform::update(kernel::parameters().framedelta_milliseconds);

		RenderTest* render_test = &render_callbacks[test_state];
		render_test->render_callback(state);

		if (countdown <= 0)
		{
			// snap a screenshot
			render2::Image image(render_allocator);
			render2::RenderTarget* render_target = state.device->default_render_target();
			image.create(render_target->width, render_target->height, 4);
			state.device->render_target_read_pixels(render_target, image);
			image::flip_image_vertically(render_target->width, render_target->height, 4, &image.pixels[0]);
			image::save_image_to_file(image, core::str::format("%s.png", render_test->name()));

			++test_state;
			countdown = RENDER_TEST_WAIT_SECONDS;
			if (test_state >= render_callbacks.size())
			{
				kernel::instance()->set_active(false);
			}
		}
	}


	virtual void shutdown()
	{
		assets::shutdown();

#if TEST_RENDER_GRAPHICS
		if (state.checker)
		{
			state.device->destroy_texture(state.checker);
		}

		state.device->destroy_texture(state.notexture);

		state.device->destroy_pipeline(state.pipeline);
		state.device->destroy_pipeline(state.texture_pipeline);
		state.device->destroy_pipeline(state.font_pipeline);
		state.device->destroy_pipeline(state.line_pipeline);
		state.device->destroy_pipeline(state.multi_pipeline);

		state.device->destroy_buffer(state.vertex_buffer);
		state.device->destroy_buffer(state.textured_buffer);
		state.device->destroy_buffer(state.font_buffer);
		state.device->destroy_buffer(state.line_buffer);

		render2::destroy_device(render_allocator, state.device);
#endif

		render_callbacks.clear();

		platform::window::destroy(state.native_window);
		if (state.other_window)
		{
			platform::window::destroy(state.other_window);
		}
		platform::window::shutdown();

		gemini::runtime_shutdown();
	}


private:
	struct MyVertex
	{
		float position[3];
		gemini::Color color;

		void set_position(float x, float y, float z)
		{
			position[0] = x;
			position[1] = y;
			position[2] = z;
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


	// counter clock wise
	void generate_triangle(size_t index, MyVertex* source, const glm::vec2& dimensions, const glm::vec2& offset)
	{
		MyVertex* vertices = (source+index);
		vertices[0].set_position(offset.x, (dimensions.y/2)+offset.y, 0);
		vertices[0].color = gemini::Color(1.0f, 0.0f, 0.0f, 1.0f);

		vertices[1].set_position((dimensions.x/2)+offset.x, (dimensions.y/2)+offset.y, 0);
		vertices[1].color = gemini::Color(0.0f, 1.0f, 0.0f, 1.0f);

		vertices[2].set_position((dimensions.x/4)+offset.x, offset.y, 0);
		vertices[2].color = gemini::Color(0.0f, 0.0f, 1.0f, 1.0f);
	}

	void generate_textured_triangle(size_t index, TexturedVertex* source, const glm::vec2& dimensions, const glm::vec2& offset)
	{
		TexturedVertex* vertices = (source+index);
		vertices[0].set_position(offset.x, (dimensions.y/2)+offset.y, 0);
		vertices[0].color = gemini::Color(1.0f, 1.0f, 1.0f, 1.0f);
		vertices[0].set_uv(0.0f, 1.0f);

		vertices[1].set_position((dimensions.x/2)+offset.x, (dimensions.y/2)+offset.y, 0);
		vertices[1].color = gemini::Color(1.0f, 1.0f, 1.0f, 1.0f);
		vertices[1].set_uv(1.0f, 1.0f);

		vertices[2].set_position((dimensions.x/4)+offset.x, offset.y, 0);
		vertices[2].color = gemini::Color(1.0f, 1.0f, 1.0f, 1.0f);
		vertices[2].set_uv(0.5f, 0.0f);
	}

	void generate_textured_quad(size_t index, TexturedVertex* source, const glm::vec2& dimensions, const glm::vec2& offset)
	{
		TexturedVertex* vertices = (source+index);

		// lower left
		vertices[0].set_position(offset.x, (dimensions.y/2)+offset.y, 0);
		vertices[0].color = gemini::Color(1.0f, 1.0f, 1.0f, 1.0f);
		vertices[0].set_uv(0.0f, 1.0f);

		// lower right
		vertices[1].set_position((dimensions.x/2)+offset.x, (dimensions.y/2)+offset.y, 0);
		vertices[1].color = gemini::Color(1.0f, 1.0f, 1.0f, 1.0f);
		vertices[1].set_uv(1.0f, 1.0f);

		// upper right
		vertices[2].set_position((dimensions.x/2)+offset.x, offset.y, 0);
		vertices[2].color = gemini::Color(1.0f, 1.0f, 1.0f, 1.0f);
		vertices[2].set_uv(1.0f, 0.0f);

		// (and upper right again on the second triangle)
		vertices[3].set_position((dimensions.x/2)+offset.x, offset.y, 0);
		vertices[3].color = gemini::Color(1.0f, 1.0f, 1.0f, 1.0f);
		vertices[3].set_uv(1.0f, 0.0f);

		// upper left
		vertices[4].set_position(offset.x, offset.y, 0);
		vertices[4].color = gemini::Color(1.0f, 1.0f, 1.0f, 1.0f);
		vertices[4].set_uv(0.0f, 0.0f);

		// lower left
		vertices[5].set_position(offset.x, (dimensions.y/2)+offset.y, 0);
		vertices[5].color = gemini::Color(1.0f, 1.0f, 1.0f, 1.0f);
		vertices[5].set_uv(0.0f, 1.0f);
	}


	size_t test_state;
	float countdown;
	glm::vec2 center;

	gemini::Allocator render_allocator;
	Array<RenderTest> render_callbacks;
	TestRenderState state;
	bool active;
};

PLATFORM_MAIN
{
	PLATFORM_IMPLEMENT_PARAMETERS();

	PLATFORM_RETURN(platform::run_application(new TestRender()));
}
