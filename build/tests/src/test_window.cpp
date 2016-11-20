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
#include <renderer/font.h>
#include <renderer/renderer.h>

#include <runtime/filesystem.h>
#include <runtime/runtime.h>

#include <platform/platform.h>
#include <platform/window.h>
#include <platform/kernel.h>
#include <platform/input.h>

#include <core/argumentparser.h>
#include <core/logging.h>
#include <core/hashset.h>
#include <core/stackstring.h>
#include <core/fixedarray.h>

#define VK_USE_PLATFORM_WIN32_KHR 1

#include <vulkan/vulkan.h>

using namespace platform;
using namespace renderer;

static VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT object_type,
	uint64_t obj,
	size_t location,
	int32_t code,
	const char* layer_prefix,
	const char* message,
	void* user_data)
{
	LOGV("validation layer: %s\n", message);
	return VK_FALSE;
}

class TestWindow : public kernel::IKernel,
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

	font::Handle font;

	gemini::Allocator render_allocator;
	gemini::Allocator font_allocator;

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
	TestWindow() :
		active(true)
	{
	}

	virtual ~TestWindow() {}

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
		else if (event.subtype == kernel::WindowClosed)
		{
			LOGV("Window was closed!\n");
			set_active(false);
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
			LOGV("mouse button: %s, %i -> %s\n", event.is_down ? "Yes" : "No", event.button, gemini::mouse_button_name(event.button));
		}
		else
		{
			LOGV("mouse event: %i\n", event.subtype);
		}
	}

	virtual kernel::Error startup()
	{
		// parse command line values
		std::vector<std::string> arguments;
		core::argparse::ArgumentParser parser;
		core::StackString<MAX_PATH_SIZE> content_path;

		gemini::runtime_load_arguments(arguments, parser);

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




		gemini::runtime_startup("arcfusion.net/test_window", custom_path_setup);

		VkApplicationInfo app;
		app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app.pApplicationName = nullptr;
		app.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		app.pEngineName = "No Engine";
		app.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		app.apiVersion = VK_API_VERSION_1_0;


		const char* win32_extensions[] = { "VK_KHR_surface", "VK_KHR_win32_surface", VK_EXT_DEBUG_REPORT_EXTENSION_NAME };

		VkInstanceCreateInfo info;
		memset(&info, 0, sizeof(VkInstanceCreateInfo));
		info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		info.pApplicationInfo = &app;
		info.enabledExtensionCount = 3;
		info.ppEnabledExtensionNames = win32_extensions;

		// Query available extensions
		uint32_t total_extensions = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &total_extensions, nullptr);
		LOGV("total extensions: %i\n", total_extensions);

		Array<VkExtensionProperties> props;
		props.resize(total_extensions);

		const char* validation_layers[] = { "VK_LAYER_LUNARG_standard_validation" };
		uint32_t total_layers = 0;
		vkEnumerateInstanceLayerProperties(&total_layers, nullptr);
		Array<VkLayerProperties> layers;
		layers.resize(total_layers);
		vkEnumerateInstanceLayerProperties(&total_layers, &layers[0]);
		LOGV("Listing Vulkan Layers (%i)...\n", total_layers);
		for (const VkLayerProperties& prop : layers)
		{
			LOGV("-> %s, %s\n", prop.layerName, prop.description);
		}
		info.enabledLayerCount = 1;
		info.ppEnabledLayerNames = validation_layers;

		vkEnumerateInstanceExtensionProperties(nullptr, &total_extensions, &props[0]);

		LOGV("Listing Vulkan Extensions (%i)...\n", total_extensions);
		for (const VkExtensionProperties& prop : props)
		{
			LOGV("-> %s\n", prop.extensionName);
		}


		//VkAllocationCallbacks callbacks;
		//memset(&callbacks, 0, sizeof(VkAllocationCallbacks));
		VkInstance instance;
		VkResult result = vkCreateInstance(&info, nullptr, &instance);

		VkDebugReportCallbackEXT callback;
		VkDebugReportCallbackCreateInfoEXT callback_info;
		callback_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		callback_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT;
		callback_info.pfnCallback = vulkan_debug_callback;

		PFN_vkCreateDebugReportCallbackEXT create_report_callback = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
		assert(create_report_callback);


		VkResult report_result = create_report_callback(instance, &callback_info, nullptr, &callback);
		assert(report_result == VK_SUCCESS);



		uint32_t total_devices = 0;
		vkEnumeratePhysicalDevices(instance, &total_devices, nullptr);

		Array<VkPhysicalDevice> devices;
		devices.resize(total_devices);
		vkEnumeratePhysicalDevices(instance, &total_devices, &devices[0]);


		VkPhysicalDeviceFeatures device_features;
		vkGetPhysicalDeviceFeatures(devices[0], &device_features);

		VkPhysicalDeviceProperties device_properties;
		vkGetPhysicalDeviceProperties(devices[0], &device_properties);


		// Only suitable with discrete GPUs.
		LOGV("Found device %s\n", device_properties.deviceName);
		assert(device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);

		uint32_t queue_family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(devices[0], &queue_family_count, nullptr);

		Array<VkQueueFamilyProperties> queue_families;
		queue_families.resize(queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(devices[0], &queue_family_count, &queue_families[0]);


		// Determine if a device is suitable based on our application's
		// requirements.
		size_t queue_graphics_index = 0;
		for (size_t index = 0; index < queue_families.size(); ++index)
		{
			VkQueueFamilyProperties& qfamily = queue_families[index];
			if (qfamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				queue_graphics_index = index;
			}
		}

		VkDeviceQueueCreateInfo queue_create_info;
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.queueFamilyIndex = queue_graphics_index;
		queue_create_info.queueCount = 1;

		float queue_priority = 1.0f;
		queue_create_info.pQueuePriorities = &queue_priority;



		VkPhysicalDeviceFeatures requested_features;
		VkDeviceCreateInfo device_create_info;
		device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		device_create_info.pQueueCreateInfos = &queue_create_info;
		device_create_info.queueCreateInfoCount = 1;
		device_create_info.pEnabledFeatures = &requested_features;
		device_create_info.enabledLayerCount = 1;
		device_create_info.ppEnabledLayerNames = validation_layers;



		VkDevice dev;
		if (vkCreateDevice(devices[0], &device_create_info, nullptr, &dev) != VK_SUCCESS)
		{
			LOGV("Unable to create new device\n");
		}


		VkQueue graphics_queue;
		vkGetDeviceQueue(dev, queue_graphics_index, 0, &graphics_queue);
		assert(graphics_queue != VK_NULL_HANDLE);

		// https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Window_surface

		{
			VkWin32SurfaceCreateInfoKHR surface_info;
			surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
			surface_info.hwnd = nullptr;
			surface_info.hinstance = GetModuleHandleA(nullptr);
		}



		PFN_vkDestroyDebugReportCallbackEXT destroy_report_callback = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
		assert(destroy_report_callback);
		destroy_report_callback(instance, callback, nullptr);

		vkDestroyInstance(instance, nullptr);

		// create a platform window
		{
			platform::window::startup(platform::window::RenderingBackend_Default);

			LOGV("total screens: %zu\n", platform::window::screen_count());

			for (size_t screen = 0; screen < platform::window::screen_count(); ++screen)
			{
				platform::window::Frame frame = platform::window::screen_frame(screen);
				LOGV("screen rect: %zu, origin: %2.2f, %2.2f; resolution: %2.2f x %2.2f\n", screen, frame.x, frame.y, frame.width, frame.height);
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

			params.window_title = "test_window";
			main_window = platform::window::create(params);
		}

#if 0
		Array<PlatformExtensionDescription> extensions;
		extensions.push_back(PlatformExtensionDescription("Animation Session", "session"));
		extensions.push_back(PlatformExtensionDescription("Animation", "animation"));
		extensions.push_back(PlatformExtensionDescription("Any File", "*"));

		PathString output;
		platform::show_save_dialog("Save something", 0, extensions, "session", output);
		LOGV("output is %s\n", output());
#endif

#if 0
		Array<PathString> paths;
		platform::show_open_dialog("Open Something", 0, paths);
		LOGV("path selected: %s\n", paths[0]());
#endif

		// initialize the renderer
		{
			using namespace render2;
			render_allocator = gemini::memory_allocator_default(gemini::MEMORY_ZONE_RENDERER);
			RenderParameters params(render_allocator);

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

			LOGV("creating render device...\n");
			device = create_device(render_allocator, params);
			assert(device);

			platform::window::Frame window_frame = platform::window::get_frame(main_window);
			LOGV("initializing render device...\n");
			device->init(static_cast<int>(window_frame.width), static_cast<int>(window_frame.height));

			LOGV("setting up shaders...\n");

			// setup shaders
			render2::PipelineDescriptor desc;
			desc.shader = device->create_shader("vertexcolor");
			render2::VertexDescriptor& vertex_format = desc.vertex_description;
			vertex_format.add("in_position", render2::VD_FLOAT, 3);
			vertex_format.add("in_color", render2::VD_FLOAT, 4);

			LOGV("vertexcolor - create_input_layout\n");
			desc.input_layout = device->create_input_layout(vertex_format, desc.shader);
			LOGV("vertexcolor - create_pipeline\n");
			pipeline = device->create_pipeline(desc);
			LOGV("vertexcolor shader loaded OK\n");

			size_t total_bytes = sizeof(MyVertex) * 6;
			LOGV("creating vertex buffer (%i bytes)\n", total_bytes);
			vertex_buffer = device->create_vertex_buffer(total_bytes);
			assert(vertex_buffer);


			// This section is for testing platforms where the vertex buffer
			// can be locked and unlocked. Not available on GLES2!
#if 0
			LOGV("lock and populate vertex buffer\n");
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
			MyVertex vertex[6];
			vertex[0].set_position(0, window_frame.height, 0);
			vertex[0].set_color(1.0f, 0.0f, 0.0f, 1.0f);

			vertex[1].set_position(window_frame.width, window_frame.height, 0);
			vertex[1].set_color(0.0f, 1.0f, 0.0f, 1.0f);

			vertex[2].set_position(window_frame.width/2.0f, 0, 0);
			vertex[2].set_color(0.0f, 0.0f, 1.0f, 1.0f);

			vertex[3].set_position(0, 0, 0);
			vertex[3].set_color(0.0f, 1.0f, 1.0f, 1.0f);

			device->buffer_upload(vertex_buffer, vertex, total_bytes);
#endif
		}

		LOGV("font startup...\n");
		font_allocator = gemini::memory_allocator_default(gemini::MEMORY_ZONE_DEFAULT);
		font::startup(font_allocator, device);

		Array<unsigned char> data(font_allocator);
		LOGV("load fonts/debug.ttf\n");
		core::filesystem::instance()->virtual_load_file(data, "fonts/debug.ttf");
		font = font::load_from_memory(&data[0], data.size(), 16);

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
		serializer->vertex_buffer(vertex_buffer);
		serializer->draw(0, 3);
		device->queue_buffers(queue, 1);
		device->destroy_serializer(serializer);

		platform::window::activate_context(main_window);

		device->submit();

		platform::window::swap_buffers(main_window);
	}


	virtual void shutdown()
	{
		font::shutdown();

		// shutdown the render device
		device->destroy_buffer(vertex_buffer);

		device->destroy_pipeline(pipeline);

		destroy_device(render_allocator, device);

//		glDeleteSync(fence);

//		renderer::shutdown();

		platform::window::destroy(main_window);
		platform::window::shutdown();

		gemini::runtime_shutdown();
	}


	virtual void event(kernel::KeyboardEvent& event)
	{
		if (event.key == gemini::BUTTON_ESCAPE && event.is_down)
		{
			set_active(false);
		}
		else
		{
			LOGV("key is_down: '%s', name: '%s', modifiers: %zu\n", event.is_down ? "Yes" : "No", gemini::key_name(static_cast<uint32_t>(event.key)), event.modifiers);
		}
	}

};

PLATFORM_MAIN
{
	PLATFORM_IMPLEMENT_PARAMETERS();
	PLATFORM_RETURN(platform::run_application(new TestWindow()));
}
