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

#include <platform/platform.h>
#include <platform/window.h>
#include <platform/kernel.h>
#include <fixedsizequeue.h>

#include <core/str.h>
#include <core/logging.h>
#include <core/argumentparser.h>
#include <core/mathlib.h>
#include <core/profiler.h>

#include <runtime/assets.h>
#include <runtime/audio_mixer.h>
#include <runtime/filesystem.h>
#include <runtime/runtime.h>
#include <runtime/standaloneresourcecache.h>

#include <renderer/renderer.h>
#include <renderer/constantbuffer.h>
#include <renderer/debug_draw.h>
#include <renderer/scene_renderer.h>

#include <runtime/keyframechannel.h>
#include <runtime/mesh.h>
#include <runtime/mesh_library.h>

// SDK
#include <sdk/audio_api.h>
#include <sdk/entity_api.h>
#include <sdk/iengineentity.h>
#include <sdk/model_api.h>
#include <sdk/engine_api.h>
#include <sdk/game_api.h>
#include <sdk/experimental_api.h>
#include <sdk/shared.h>
#include <sdk/physics_api.h>

#include "input.h"

#include "scenelink.h"
#include "audio.h"
#include "animation.h"
#include "physics/physics.h"
#include "hotloading.h"
#include "navigation.h"

#include <engine/model_instance_data.h>

// for MAX_BONES
#include <shared/shared_constants.h>

// uncomment this to draw bone debug information
//#define GEMINI_DEBUG_BONES


// enable this if you want to visualize the frame rate over time
#define DEBUG_FRAMERATE

using namespace platform;
using namespace core;
using namespace gemini; // for renderer

#include <ui/ui.h>
#include <ui/compositor.h>
#include <ui/graph.h>
#include <ui/button.h>

#include "guirenderer.h"

#include <map>

// this is required at the moment because our render method needs it!
gui::Compositor* _compositor = 0;

//void render_scene_from_camera(::renderer::RenderStream& stream, gemini::IEngineEntity** entity_list, View& view, SceneLink& scenelink)
//{
//	// use the entity list to render
//	scenelink.clear();
//	scenelink.queue_entities(entity_list, MAX_ENTITIES, RENDER_VISIBLE);
//	scenelink.sort();
//	scenelink.draw(stream, &view.modelview, &view.projection);
//}

namespace gemini
{
	GameMessage event_to_gamemessage(const kernel::KeyboardEvent& event, uint64_t physics_tick);
	GameMessage event_to_gamemessage(const kernel::MouseEvent& event, uint64_t physics_tick);
	GameMessage event_to_gamemessage(const kernel::SystemEvent& event, uint64_t physics_tick);
	GameMessage event_to_gamemessage(const kernel::GameControllerEvent& event, uint64_t physics_tick);


	GameMessage event_to_gamemessage(const kernel::KeyboardEvent& event, uint64_t physics_tick)
	{
		GameMessage out;
		out.timestamp = physics_tick;
		out.type = GameMessage::KeyboardEvent;
		out.button = event.key;
		out.params[0] = event.is_down;
		out.params[1] = event.modifiers;
		return out;
	}

	GameMessage event_to_gamemessage(const kernel::MouseEvent& event, uint64_t physics_tick)
	{
		GameMessage out;
		out.timestamp = physics_tick;
		switch (event.subtype)
		{
		case kernel::MouseButton:
			out.type = GameMessage::MouseEvent;
			out.button = event.button;
			out.params[0] = event.is_down;
			break;

		case kernel::MouseMoved:
			out.type = GameMessage::MouseMove;
			out.params[0] = event.mx;
			out.params[1] = event.my;
			break;

		case kernel::MouseDelta:
			out.type = GameMessage::MouseDelta;
			out.params[0] = event.dx;
			out.params[1] = event.dy;
			break;

		case kernel::MouseWheelMoved:
			out.type = GameMessage::MouseWheel;
			out.button = event.wheel_direction;
			out.params[0] = event.mx;
			out.params[1] = event.my;
			out.params[2] = event.dx;
			out.params[3] = event.dy;
			break;

		default:
			assert(0);
			break;
		}
		return out;
	}

	GameMessage event_to_gamemessage(const kernel::SystemEvent& event, uint64_t physics_tick)
	{
		GameMessage out;
		out.timestamp = physics_tick;
		out.type = GameMessage::SystemEvent;
		if (event.subtype == kernel::WindowGainFocus)
		{
			out.params[0] = 1;
		}
		else if (event.subtype == kernel::WindowLostFocus)
		{
			out.params[1] = 1;
		}
		return out;
	}

	GameMessage event_to_gamemessage(const kernel::GameControllerEvent& event, uint64_t physics_tick)
	{
		// TODO@APP: Implement.
		GameMessage out;
		out.timestamp = physics_tick;
		out.params[0] = event.gamepad_id;

		switch (event.subtype)
		{
		case kernel::JoystickConnected:
			out.type = GameMessage::GamePadConnected;
			break;

		case kernel::JoystickDisconnected:
			out.type = GameMessage::GamePadDisconnected;
			break;

		case kernel::JoystickButton:
			out.type = GameMessage::GamePadButton;
			out.button = event.button;
			out.params[1] = event.is_down;
			break;

		case kernel::JoystickAxisMoved:
			out.type = GameMessage::GamePadAxis;
			out.params[1] = event.axis_id;
			out.params[2] = event.axis_value;
			break;

		default:
			// Unhandled gamepad input!
			assert(0);
			break;
		}

		return out;
	}
}






class EntityManager : public IEntityManager
{
	gemini::IEngineEntity* entity_list[MAX_ENTITIES];
	size_t index;

public:
	EntityManager() : index(0)
	{
		memset(entity_list, 0, sizeof(gemini::IEngineEntity*) * MAX_ENTITIES);
	}

	virtual uint16_t add(IEngineEntity* entity);
	virtual void remove(IEngineEntity* entity);

	virtual gemini::IEngineEntity* at_index(uint16_t index);

	virtual void startup();
	virtual void shutdown();

	IEngineEntity** get_entity_list() { return entity_list; }
};


uint16_t EntityManager::add(IEngineEntity* entity)
{
	uint16_t entity_index = index;
	entity_list[index++] = entity;
	return entity_index;
}

void EntityManager::remove(IEngineEntity* entity)
{
	for(size_t i = 0; i < MAX_ENTITIES; ++i)
	{
		gemini::IEngineEntity* e = entity_list[i];
		if (e && e == entity)
		{
			entity_list[i] = 0;
			break;
		}
	}
}

gemini::IEngineEntity* EntityManager::at_index(uint16_t index)
{
	return entity_list[index];
}

void EntityManager::startup()
{

}

void EntityManager::shutdown()
{

}


class Experimental : public gemini::IExperimental
{
	gui::Panel* root;
	gui::Compositor* compositor;

public:
	Experimental(gui::Panel* root) : root(root), compositor(0)
	{
	}


	void set_root(gui::Panel* rootpanel)
	{
		root = rootpanel;
	}

	void set_compositor(gui::Compositor* c)
	{
		compositor = c;
	}

	virtual gui::Panel* root_panel() const
	{
		return root;
	}

	virtual gui::Compositor* get_compositor() const
	{
		return compositor;
	}

	virtual void navmesh_find_poly(gemini::NavMeshPolyRef* poly, const glm::vec3& position, const glm::vec3& extents)
	{
		navigation::find_poly(poly, position, extents);
	}

	virtual void navmesh_find_path(NavMeshPath* path, const glm::vec3& start, const glm::vec3& end)
	{
		navigation::find_path(path, start, end);
	}

	virtual void navmesh_draw_path(NavMeshPath* path)
	{
		navigation::debugdraw_path(path);
	}

	virtual void navmesh_generate_from_model(const char* path)
	{
		assert(0); // TODO: 01-19-17: fix this (meshes)
		//assets::Mesh* mesh = assets::meshes()->load_from_path(path);
		//assets::Geometry* geom = mesh->geometry[0];
		//navigation::create_from_geometry(geom->vertices, geom->indices, geom->mins, geom->maxs);
	}

	virtual void navmesh_find_straight_path(NavMeshPath* path, glm::vec3* positions, uint32_t* total_positions)
	{
		navigation::find_straight_path(path, positions, total_positions);
	}

	virtual glm::vec3 navmesh_find_random_location()
	{
		return navigation::find_random_location();
	}
};

namespace gemini
{
	IExperimental::~IExperimental()
	{
	}
}

struct SharedState
{
	bool has_focus;

	SharedState() :
		has_focus(true)
	{
	}
};


static SharedState _sharedstate;

//const size_t GEMINI_MAX_RENDERSTREAM_BYTES = 8192;
//const size_t GEMINI_MAX_RENDER_STREAM_COMMANDS = 512;

class AudioInterface : public IAudioInterface
{
public:
	virtual void precache_sound(const char* path)
	{
		sound_load(path, false);
	}

	virtual gemini::AudioHandle play(const char* path, int num_repeats)
	{
		return gemini::audio::play_sound(
			sound_from_handle(sound_load(path, false)),
			num_repeats
		);
	}

	virtual void stop(gemini::AudioHandle handle)
	{
		gemini::audio::stop_sound(handle);
	}

	virtual void stop_all_sounds()
	{
		audio::stop_all_sounds();
	}
};


struct ModelInstanceData;


class EngineInterface : public IEngineInterface, public IModelInterface
{
	IEntityManager* entity_manager;
	IModelInterface* model_interface;
	gemini::physics::IPhysicsInterface* physics_interface;
	IExperimental* experimental_interface;

	platform::window::NativeWindow* main_window;
	//SceneLink& scenelink;
	render2::Device* device;
	render2::Pipeline* pipeline;

	//Array<char> render_stream_data;
	//Array<::renderer::RenderState> render_commands;

	gemini::Allocator& engine_allocator;
	unsigned int diffuse_unit;

	RenderScene* render_scene;

	// model instance data
	typedef std::map<int32_t, gemini::ModelInstanceData> ModelInstanceMap;
	ModelInstanceMap id_to_instance;

public:

	EngineInterface(gemini::Allocator& _allocator,
					IEntityManager* em,
					//IModelInterface* mi,
					gemini::physics::IPhysicsInterface* pi,
					IExperimental* ei,
					//SceneLink& scene_link,
					render2::Device* render_device,
					platform::window::NativeWindow* window)
		: engine_allocator(_allocator)
		, entity_manager(em)
		//, model_interface(mi)
		//, id_to_instance(_allocator)
		, physics_interface(pi)
		, experimental_interface(ei)
		, main_window(window)
		, device(render_device)
		//, scenelink(scene_link)
		//, render_stream_data(_allocator)
		//, render_commands(_allocator)
	{
		//render_stream_data.resize(GEMINI_MAX_RENDERSTREAM_BYTES);
		//render_commands.resize(GEMINI_MAX_RENDER_STREAM_COMMANDS);

		render2::PipelineDescriptor desc;
		render2::VertexDescriptor& vertex_format = desc.vertex_description;
		vertex_format.add("in_position", render2::VD_FLOAT, 3);
		vertex_format.add("in_normal", render2::VD_FLOAT, 3);
		vertex_format.add("in_uv", render2::VD_FLOAT, 2);
		desc.shader = shader_load("rendertest");
		desc.input_layout = device->create_input_layout(desc.vertex_description, desc.shader);
		pipeline = device->create_pipeline(desc);

		diffuse_unit = 0;

		render_scene = render_scene_create(_allocator, device);
	}


	virtual ~EngineInterface()
	{
		device->destroy_pipeline(pipeline);

		render_scene_destroy(render_scene, device);
	}

	virtual IEntityManager* entities() { return entity_manager; }
	virtual IModelInterface* models() { return this; }
	virtual gemini::physics::IPhysicsInterface* physics() { return physics_interface; }
	virtual IExperimental* experiment() { return experimental_interface; }
	virtual core::logging::ILog* log() { return core::logging::instance(); }
	virtual gemini::IAudioInterface* audio() { return gemini::audio::instance(); }

	virtual void* allocate(size_t size)
	{
		return MEMORY2_ALLOC(engine_allocator, size);
		//return game_allocator.allocate(size, sizeof(void*), __FILE__, __LINE__);
	}

	virtual void deallocate(void* pointer)
	{
		MEMORY2_DEALLOC(engine_allocator, pointer);
		//game_allocator.deallocate(pointer);
	}

	virtual void render_view(const View& view, const Color& clear_color)
	{
		// old rendering code
#if 0
		// TODO: need to validate this origin/orientation is allowed.
		// otherwise, client could ask us to render from anyone's POV.
		EntityManager* em = static_cast<EntityManager*>(engine::instance()->entities());

		gemini::IEngineEntity** entity_list = em->get_entity_list();


		::renderer::RenderStream rs((char*)&render_stream_data[0], GEMINI_MAX_RENDERSTREAM_BYTES, (::renderer::RenderState*)&render_commands[0], GEMINI_MAX_RENDER_STREAM_COMMANDS);
		rs.add_cullmode(::renderer::CullMode::CULLMODE_BACK);
//		rs.add_state(::renderer::STATE_DEPTH_WRITE, 1);
		rs.add_state(::renderer::STATE_BACKFACE_CULLING, 1);
		rs.add_state(::renderer::STATE_DEPTH_TEST, 1);
		rs.add_clearcolor(clear_color.red, clear_color.green, clear_color.blue, 1.0f);
		rs.add_clear(::renderer::CLEAR_COLOR_BUFFER | ::renderer::CLEAR_DEPTH_BUFFER );
		rs.run_commands();

		View newview = view;
		platform::window::Frame frame = platform::window::get_render_frame(main_window);
		newview.width = frame.width;
		newview.height = frame.height;

		render_scene_from_camera(rs, entity_list, newview, scenelink);
#endif
		//EntityManager* entity_manager = static_cast<EntityManager*>(engine::instance()->entities());
		//IEngineEntity** entity_list = entity_manager->get_entity_list();

		//for (uint32_t entity_index = 0; entity_index < MAX_ENTITIES; ++entity_index)
		//{
		//	gemini::IEngineEntity* entity = entity_list[entity_index];

		//	// draw visible entities
		//	if (!entity)
		//	{
		//		continue;
		//	}
		//}

		render_scene_draw(render_scene, device, view.modelview, view.projection);
	}

	virtual void render_gui()
	{
		if (_compositor)
		{
			_compositor->draw();
		}
	}

	virtual Allocator& allocator()
	{
		return engine_allocator;
	}

	virtual void render_debug(const View& view) override
	{
		View newview = view;
		platform::window::Frame frame = platform::window::get_render_frame(main_window);
		newview.width = frame.width;
		newview.height = frame.height;
		debugdraw::render(newview.modelview, newview.projection, newview.width, newview.height);
	}

	virtual void get_render_resolution(uint32_t& render_width, uint32_t& render_height)
	{
		assert(main_window);
		platform::window::Frame frame = platform::window::get_render_frame(main_window);
		render_width = frame.width;
		render_height = frame.height;
	}


	virtual void center_cursor()
	{
		if (_sharedstate.has_focus)
		{
			platform::window::Frame frame = platform::window::get_render_frame(main_window);
			platform::window::set_cursor(
				frame.x + (frame.width/2.0f),
				frame.y + (frame.height/2.0f)
			);
		}
	}

	virtual void show_cursor(bool show)
	{
		platform::window::show_cursor(show);
	}

	virtual void set_cursor(int x, int y)
	{
		platform::window::set_cursor(x, y);
	}

	virtual void get_cursor(int& x, int& y)
	{
		float fx, fy;
		platform::window::get_cursor(fx, fy);
		x = static_cast<int>(fx);
		y = static_cast<int>(fy);
	}

	virtual void terminate_application()
	{
		kernel::instance()->set_active(false);
	}

	virtual void set_relative_mouse_mode(bool enable) override
	{
		platform::window::set_relative_mouse_mode(main_window, enable);
		center_cursor();
	}

	void tick()
	{
		static float the_time = 0.0f;
		render_scene->light_position_world.x = cosf(the_time);
		render_scene->light_position_world.y = 2.0f;
		render_scene->light_position_world.z = sinf(the_time);
		the_time += 0.01f;

		render_scene_extract(render_scene);
	}

	// IModelInterface
	virtual int32_t create_instance_data(uint16_t entity_index, const char* model_path);
	virtual void destroy_instance_data(int32_t index);
	virtual IModelInstanceData* get_instance_data(int32_t index);

};

int32_t EngineInterface::create_instance_data(uint16_t entity_index, const char* model_path)
{
	AssetHandle mesh_handle = mesh_load(model_path);
	Mesh* mesh = mesh_from_handle(mesh_handle);
	if (mesh)
	{
		gemini::ModelInstanceData data(engine_allocator);
		data.set_mesh_index(mesh_handle);
		int32_t index = (int32_t)id_to_instance.size();
		id_to_instance.insert(ModelInstanceMap::value_type(index, data));

		uint32_t component_id = 0;
		if (mesh->skeleton.empty())
		{
			component_id = render_scene_add_static_mesh(render_scene, mesh_handle, entity_index, glm::mat4(1.0f));
		}
		else
		{
			component_id = render_scene_add_animated_mesh(render_scene, mesh_handle, entity_index, glm::mat4(1.0f));
		}

		return index;
	}

	return -1;
}

void EngineInterface::destroy_instance_data(int32_t index)
{
	ModelInstanceMap::iterator it = id_to_instance.find(index);
	if (it != id_to_instance.end())
	{
		gemini::ModelInstanceData& data = it->second;
		id_to_instance.erase(it);
	}
}


IModelInstanceData* EngineInterface::get_instance_data(int32_t index)
{
	ModelInstanceMap::iterator it = id_to_instance.find(index);
	if (it != id_to_instance.end())
	{
		return &(*it).second;
	}

	return 0;
}



typedef gemini::IGameInterface* (*connect_engine_fn)(gemini::IEngineInterface*);
typedef void (*disconnect_engine_fn)();

#if defined(GEMINI_STATIC_GAME)
extern "C"
{
	gemini::IGameInterface* connect_engine(gemini::IEngineInterface* engine_interface);
	void disconnect_engine();
}
#endif

class EngineKernel : public kernel::IKernel,
public kernel::IEventListener<kernel::KeyboardEvent>,
public kernel::IEventListener<kernel::MouseEvent>,
public kernel::IEventListener<kernel::SystemEvent>,
public kernel::IEventListener<kernel::GameControllerEvent>
{

private:
	// kernel stuff
	bool active;
	StackString<MAX_PATH_SIZE> game_path;
	bool draw_physics_debug;
	bool draw_navigation_debug;

	platform::window::NativeWindow* main_window;


	Array<gemini::GameMessage>* queued_messages;


	// Kernel State variables
	double accumulator;
	uint64_t last_time;

	// rendering
	//SceneLink* scenelink;
	render2::Device* device;

	// game library
	platform::DynamicLibrary* gamelib;
	disconnect_engine_fn disconnect_engine;
	EntityManager entity_manager;
	//ModelInterface model_interface;
	Experimental experimental;
	IEngineInterface* engine_interface;
	IGameInterface* game_interface;


	// GUI stuff
	GUIRenderer* gui_renderer;
	gui::Compositor* compositor;
	gui::Graph* graph;
	gui::Panel* root;

	::renderer::StandaloneResourceCache* resource_cache;

	gemini::Allocator audio_allocator;
	gemini::Allocator renderer_allocator;
	gemini::Allocator animation_allocator;
	gemini::Allocator gui_allocator;
	gemini::Allocator engine_allocator;

	void open_gamelibrary()
	{
#if !defined(GEMINI_STATIC_GAME)
		core::filesystem::IFileSystem* fs = core::filesystem::instance();

		// load game library
		PathString game_library_path = fs->content_directory();

		PathString relative_library_path;
		relative_library_path = "bin";
		relative_library_path.append(PATH_SEPARATOR_STRING);
#if defined(PLATFORM_LINUX)
		relative_library_path.append("lib");
#endif
		relative_library_path.append("game");
		relative_library_path.append(platform::dylib_extension());
//		fs->get_absolute_path_for_content(game_library_path, relative_library_path());
		core::filesystem::absolute_path_from_relative(game_library_path, relative_library_path(), fs->content_directory());




//		const char* dynamiclibrary_extension = platform::dylib_extension();
//		game_library_path.append(PATH_SEPARATOR_STRING).append("bin").append(PATH_SEPARATOR_STRING).append("game").append(dynamiclibrary_extension);

		gamelib = platform::dylib_open(game_library_path());
		if (!gamelib)
		{
			LOGV("unable to open game: \"%s\"\n", game_library_path());
			assert(0);
		}
		else
		{
			LOGV("opened game \"%s\"\n", game_library_path());

			// link the engine interface

			connect_engine_fn connect_engine = (connect_engine_fn)platform::dylib_find(gamelib, "connect_engine");
			disconnect_engine = (disconnect_engine_fn)platform::dylib_find(gamelib, "disconnect_engine");
			if (connect_engine)
			{
				game_interface = connect_engine(gemini::engine::instance());
			}
			if (!game_interface)
			{
				LOGE("Unable to connect engine to game library\n");
				assert(game_interface != 0);
			}

			game_interface->startup();
		}
#else
		game_interface = ::connect_engine(gemini::engine::instance());
		if (!game_interface)
		{
			LOGE("Unable to connect engine to game library\n");
			assert(game_interface != 0);
		}

		game_interface->startup();
#endif
	}

	void close_gamelibrary()
	{
#if !defined(GEMINI_STATIC_GAME)
		// shutdown game
		if (game_interface)
		{
			game_interface->shutdown();
		}

		if (disconnect_engine)
		{
			disconnect_engine();
		}

		platform::dylib_close(gamelib);
#else
		// shutdown game
		if (game_interface)
		{
			game_interface->shutdown();
		}

		::disconnect_engine();
		game_interface = nullptr;
#endif
	}

	glm::vec3 last_p;
	glm::vec3 p;


public:
	EngineKernel() :
		active(true),
		draw_physics_debug(false),
		draw_navigation_debug(false),
		accumulator(0.0f),
		last_time(0),
		experimental(0),
		engine_interface(0),
		game_interface(0)
		, engine_allocator(memory_allocator_default(MEMORY_ZONE_DEFAULT))
		//, model_interface(engine_allocator)
		, queued_messages(nullptr)
	{
		game_path = "";
		compositor = nullptr;
		gui_renderer = nullptr;
		resource_cache = nullptr;

		last_p = glm::vec3(0.0f, 0.0f, 0.0f);
		p = glm::vec3(0.0f, 0.0f, 0.0f);
	}

	virtual ~EngineKernel()
	{
	}

	virtual bool is_active() const { return active; }
	virtual void set_active(bool isactive) { active = isactive; }

	void queue_game_message(const GameMessage& message)
	{
		if (queued_messages)
		{
			queued_messages->push_back(message);
		}
	}

	virtual void event(kernel::KeyboardEvent& event)
	{
		if (event.is_down)
		{
			if (event.key == gemini::BUTTON_P)
			{
				draw_physics_debug = !draw_physics_debug;
				LOGV("draw_physics_debug = %s\n", draw_physics_debug?"ON":"OFF");
			}
			else if (event.key == gemini::BUTTON_N)
			{
				draw_navigation_debug = !draw_navigation_debug;
				LOGV("draw_navigation_debug = %s\n", draw_navigation_debug?"ON":"OFF");
			}
		}

		queue_game_message(event_to_gamemessage(event, kernel::parameters().current_physics_tick));
	}

	virtual void event(kernel::MouseEvent& event)
	{
		queue_game_message(event_to_gamemessage(event, kernel::parameters().current_physics_tick));
	}

	virtual void event(kernel::SystemEvent& event)
	{
		if (event.subtype == kernel::WindowLostFocus)
		{
			_sharedstate.has_focus = false;
		}
		else if (event.subtype == kernel::WindowGainFocus)
		{
			_sharedstate.has_focus = true;
		}
		else if (event.subtype == kernel::WindowResized)
		{
			platform::window::Frame frame = platform::window::get_render_frame(main_window);

			assert(device);
			device->backbuffer_resized(frame.width, frame.height);

			if (compositor)
			{
				compositor->resize(frame.width, frame.height);
			}
		}
		else if (event.subtype == kernel::WindowClosed)
		{
			LOGV("Window was closed!\n");
			set_active(false);
		}

		queue_game_message(event_to_gamemessage(event, kernel::parameters().current_physics_tick));
	}

	virtual void event(kernel::GameControllerEvent& event)
	{
		//if (game_interface)
		//{
		//	//if (event.subtype == kernel::JoystickButton && event.is_down)
		//	//{
		//	//	gemini::audio::SoundHandle_t sound_handle = gemini::audio::play_sound(test_sound, 0);
		//	//}

		//	game_interface->on_event(event);
		//}

		queue_game_message(event_to_gamemessage(event, kernel::parameters().current_physics_tick));
	}

	void setup_gui(render2::Device* device, gemini::Allocator& renderer_allocator, uint32_t width, uint32_t height)
	{
		gui_allocator = memory_allocator_default(MEMORY_ZONE_DEFAULT);

		gui::set_allocator(gui_allocator);
		resource_cache = MEMORY2_NEW(renderer_allocator, ::renderer::StandaloneResourceCache)(renderer_allocator);

		gui_renderer = MEMORY2_NEW(renderer_allocator, GUIRenderer)(gui_allocator, *resource_cache);
		gui_renderer->set_device(device);

		compositor = new gui::Compositor(width, height, resource_cache, gui_renderer);
		compositor->set_name("compositor");
		_compositor = compositor;

		root = new gui::Panel(compositor);
		root->set_name("root");

		experimental.set_root(root);
		experimental.set_compositor(compositor);

		platform::window::Frame frame = platform::window::get_render_frame(main_window);
		root->set_origin(0, 0);
		root->set_size(frame.width, frame.height);
		root->set_background_color(Color(0, 0, 0, 0));

#if defined(DEBUG_FRAMERATE)
		// setup the framerate graph
		graph = new gui::Graph(root);
		graph->set_name("frametime_graph");
		graph->set_origin(width - 250, 0);
		graph->set_size(250, 100);
		graph->set_font("debug", 16);
		graph->set_background_color(Color::from_rgba(10, 10, 10, 210));
		graph->set_foreground_color(Color::from_rgba(255, 255, 255, 255));
		graph->create_samples(100, 1);
		graph->configure_channel(0, Color::from_rgba(0, 255, 0, 255));
		graph->set_range(0.0f, 33.3f);
		graph->enable_baseline(true, 16.6f, Color::from_rgba(255, 0, 255, 255));
#endif
	}

	virtual kernel::Error startup()
	{
		// parse command line values
		std::vector<std::string> arguments;
		core::argparse::ArgumentParser parser;

		runtime_load_arguments(arguments, parser);

		core::argparse::VariableMap vm;
		const char* docstring = R"(
Usage:
	--game=<game_path>

Options:
	-h, --help  Show this help screen
	--version  Display the version number
	--game=<game_path>  The game path to load content from
	)";

		if (parser.parse(docstring, arguments, vm, "1.0.0-alpha"))
		{
			std::string path = vm["--game"];
			game_path = platform::make_absolute_path(path.c_str());
		}
		else
		{
			return kernel::CoreFailed;
		}

		kernel::Parameters& params = kernel::parameters();

		// initialize timer
		last_time = platform::microseconds();

#if defined(PLATFORM_MOBILE)
#else
		kernel::parameters().device_flags |= kernel::DeviceDesktop;
#endif

		// runtime setup
		PathString root_path = platform::get_program_directory();

		// if no game is specified on the command line, construct the content path
		// from the current root directory
		StackString<MAX_PATH_SIZE> content_path;
		if (game_path.is_empty())
		{
			content_path = platform::fs_content_directory();
		}
		else
		{
			// dev builds (passed by -game) are located at:
			// "<game_path>/builds/<PLATFORM_NAME>"
			content_path = game_path;
			content_path.append(PATH_SEPARATOR_STRING);
			content_path.append("builds");
			content_path.append(PATH_SEPARATOR_STRING);
			content_path.append(PLATFORM_NAME);
		}

		// setup filesystem paths
		Settings config;

		std::function<void(const char*)> custom_path_setup = [&](const char* /*application_data_path*/)
		{
			core::filesystem::IFileSystem* filesystem = core::filesystem::instance();

			// the root path is the current binary path
			filesystem->root_directory(root_path);

			// the content directory is where we'll find our assets
			filesystem->content_directory(content_path);

			// load engine settings (from content path)
			runtime_load_application_config(config);

			// the application path can be specified in the config (per-game basis)
			const platform::PathString application_path = platform::get_user_application_directory(config.application_directory.c_str());
			filesystem->user_application_directory(application_path);
		};

		const uint32_t runtime_flags = RF_CORE | RF_WINDOW_SYSTEM;
		gemini::runtime_startup(nullptr, custom_path_setup, runtime_flags);

		LOGV("Logging system initialized.\n");

		const core::filesystem::IFileSystem* filesystem = core::filesystem::instance();
		LOGV("filesystem root_path = '%s'\n", filesystem->root_directory().c_str());
		LOGV("filesystem content_path = '%s'\n", content_path.c_str());
		LOGV("filesystem user_application_directory = '%s'\n", filesystem->user_application_directory().c_str());

		params.step_interval_seconds = (1.0f/(float)config.physics_tick_rate);

		platform::window::Parameters window_params;

		platform::window::Frame screen_frame = platform::window::screen_frame(0);
		window_params.enable_fullscreen = false;
		window_params.enable_vsync = true;

		// TODO: we should load these from a config; for now just set them.
		window_params.frame = platform::window::centered_window_frame(0, config.window_width, config.window_height);
		window_params.window_title = config.window_title();

		if (window_params.enable_fullscreen)
		{
			config.window_width = screen_frame.width;
			config.window_height = screen_frame.height;
			window_params.frame = screen_frame;
		}

		// create the window
		main_window = platform::window::create(window_params);
		platform::window::focus(main_window);

		engine_allocator = memory_allocator_default(MEMORY_ZONE_DEFAULT);
		renderer_allocator = memory_allocator_default(MEMORY_ZONE_RENDERER);

		queued_messages = MEMORY2_NEW(engine_allocator, Array<GameMessage>)(engine_allocator);

		// initialize rendering subsystems
		render2::RenderParameters render_params(renderer_allocator);

		// set some options
		render_params["vsync"] = "true";
		render_params["double_buffer"] = "true";
		render_params["depth_size"] = "24";
		render_params["multisample"] = "4";

		render_params["gamma_correct"] = "false";

		// set opengl specific options
		render_params["rendering_backend"] = "opengl";
		render_params["opengl.major"] = "3";
		render_params["opengl.minor"] = "2";
		render_params["opengl.profile"] = "core";
		render_params["opengl.share_context"] = "true";

		device = render2::create_device(renderer_allocator, render_params);
		assert(device != nullptr);

		device->init(config.window_width, config.window_height);

		//scenelink = MEMORY2_NEW(renderer_allocator, SceneLink)(renderer_allocator);
		assets::startup(device);

		render_scene_startup(device, renderer_allocator);

		// initialize debug draw
		debugdraw::startup(renderer_allocator, device);

		audio_allocator = memory_allocator_default(MEMORY_ZONE_AUDIO);

		// initialize main subsystems
		audio::startup(audio_allocator);
		IAudioInterface* audio_instance = MEMORY2_NEW(audio_allocator, AudioInterface);
		audio::set_instance(audio_instance);

		animation_allocator = memory_allocator_default(MEMORY_ZONE_DEFAULT);

		gemini::physics::startup();
		animation::startup(animation_allocator);

		if (config.enable_asset_reloading)
		{
			hotloading::startup(renderer_allocator);
		}

		// setup interfaces
		engine_interface = MEMORY2_NEW(engine_allocator, EngineInterface)
			(engine_allocator,
			&entity_manager,
			//&model_interface,
			physics::instance(),
			&experimental,
			device,
			//*scenelink,
			main_window
		);
		gemini::engine::set_instance(engine_interface);

		platform::window::Frame frame = platform::window::get_render_frame(main_window);
		setup_gui(device, renderer_allocator, frame.width, frame.height);

		 open_gamelibrary();

		//navigation::startup();

		// for debugging
		if (game_interface)
		{
			game_interface->level_load();
		}

		// TODO: post_application_startup

		uint64_t current_time = platform::microseconds();
		LOGV("startup in %2.2fms\n", (current_time-last_time) * MillisecondsPerMicrosecond);
		last_time = current_time;

		return kernel::NoError;
	}


	virtual void tick()
	{
		uint64_t current_time = platform::microseconds();
		kernel::Parameters& params = kernel::parameters();

		PROFILE_BEGIN("platform_update");
		platform::update(kernel::parameters().framedelta_milliseconds);
		PROFILE_END("platform_update");

		// update accumulator
		accumulator += params.framedelta_seconds;

		// set the baseline for the font
		int x = 250;
		int y = 16;
		debugdraw::text(x, y, core::str::format("frame delta = %2.2fms\n", params.framedelta_milliseconds), Color());
		y += 12;
		//debugdraw::text(x, y, core::str::format("allocations = %i, total %2.2f MB\n",
		//	core::memory::global_allocator().get_zone()->get_active_allocations(),
		//	core::memory::global_allocator().get_zone()->get_active_bytes() / (float)(1024 * 1024)), Color());
		//y += 12;

		uint32_t reset_queue = 0;
		static glm::vec3 poz;
		static float t = 0.0f;
		static bool fwd = false;
		static bool back = false;

		while (accumulator > params.step_interval_seconds)
		{
			last_p = p;
			// iterate over queued messages and play until we hit the time cap
			for (size_t index = 0; index < queued_messages->size(); ++index)
			{
				if ((*queued_messages)[index].timestamp <= params.current_physics_tick)
				{
					if (game_interface)
					{
						GameMessage& message = (*queued_messages)[index];
						if (message.type == GameMessage::KeyboardEvent)
						{
							if (message.button == BUTTON_W)
							{
								fwd = message.params[0] ? true : false;
							}
							else if (message.button == BUTTON_S)
							{
								back = message.params[0] ? true : false;
							}
						}

						game_interface->handle_game_message((*queued_messages)[index]);
					}
				} // execute
			}

			if (game_interface)
			{
				game_interface->fixed_step(params.current_physics_tick, params.step_interval_seconds, params.step_alpha);
				if (fwd)
				{
					p.z -= 2.f * params.step_interval_seconds;
				}
				if (back)
				{
					p.z += 2.f * params.step_interval_seconds;
				}
			}

			// subtract the interval from the accumulator
			accumulator -= params.step_interval_seconds;

			// increment tick counter
			params.current_physics_tick++;

			reset_queue = 1;
			t = 0.0f;
		}

		if (reset_queue)
		{
			queued_messages->resize(0);

			if (game_interface)
			{
				game_interface->reset_events();
			}
		}

		if (game_interface)
		{
			game_interface->tick(params.current_physics_tick, params.framedelta_seconds);
		}






		poz = lerp(last_p, p, t / 0.1f);

		debugdraw::sphere(poz, Color::from_rgba(255, 0, 0, 255), 2.0f);

		debugdraw::update(params.step_interval_seconds * MillisecondsPerSecond);

		params.step_alpha = accumulator / params.step_interval_seconds;
		if (params.step_alpha >= 1.0f)
		{
			params.step_alpha -= 1.0f;
		}

		EngineInterface* ei = reinterpret_cast<EngineInterface*>(engine_interface);
		ei->tick();

		/*debugdraw::update(kernel::parameters().framedelta_seconds * MillisecondsPerSecond);*/

		animation::update(kernel::parameters().framedelta_seconds);
		hotloading::tick();
		post_tick();
		kernel::parameters().current_frame++;

		// calculate delta ticks in milliseconds
		params.framedelta_milliseconds = (current_time - last_time) * MillisecondsPerMicrosecond;

		// cache the value in seconds
		params.framedelta_seconds = params.framedelta_milliseconds * SecondsPerMillisecond;

		t += params.framedelta_seconds;

		// record the current frametime milliseconds
#if defined(DEBUG_FRAMERATE)
		graph->record_value(params.framedelta_milliseconds, 0);
#endif

		last_time = current_time;

		//gemini::profiler::report();
		//gemini::profiler::reset();
	}

	void post_tick()
	{
		platform::window::activate_context(main_window);

		if (compositor)
		{
			compositor->tick(kernel::parameters().framedelta_milliseconds);
		}

		if (draw_physics_debug)
		{
			physics::debug_draw();
		}

		if (game_interface)
		{
			game_interface->render_frame();
		}

		//if (draw_navigation_debug)
		//{
		//	navigation::debugdraw();
		//}


		device->submit();

		// TODO: this needs to be controlled somehow
		// as the rift sdk performs buffer swaps during end frame.
		if (kernel::parameters().swap_buffers)
		{
			platform::window::swap_buffers(main_window);
		}
	}

	virtual void shutdown()
	{
		//navigation::shutdown();

		// shutdown gui
		if (compositor)
		{
			delete compositor;
			compositor = 0;
			_compositor = 0;
		}

		if (gui_renderer)
		{
			MEMORY2_DELETE(renderer_allocator, gui_renderer);
		}



		// since the game can create gui elements, we need to shutdown
		// the gui before shutting down the game library.
		 close_gamelibrary();

		// we need to explicitly shut this down so it cleans up before
		// our memory detects any leaks.
		if (resource_cache)
		{
			MEMORY2_DELETE(renderer_allocator, resource_cache);
		}

		// shutdown subsystems
		hotloading::shutdown();
		animation::shutdown();
		gemini::physics::shutdown();
		debugdraw::shutdown();
		audio::shutdown();

		render_scene_shutdown();

		assets::shutdown();

		IAudioInterface* interface = audio::instance();
		MEMORY2_DELETE(audio_allocator, interface);
		audio::set_instance(nullptr);

		MEMORY2_DELETE(engine_allocator, queued_messages);
		queued_messages = nullptr;

		MEMORY2_DELETE(engine_allocator, engine_interface);

		// must shutdown the renderer before our window
		render2::destroy_device(renderer_allocator, device);

		platform::window::destroy(main_window);
		main_window = 0;
		platform::window::shutdown();

#if defined(GEMINI_ENABLE_PROFILER)
		gemini::profiler::report();
#endif

		gemini::runtime_shutdown();


		//MEMORY2_DELETE(renderer_allocator, scenelink);
	}
};

PLATFORM_MAIN
{
	PLATFORM_IMPLEMENT_PARAMETERS();
	PLATFORM_RETURN(platform::run_application(new EngineKernel()));
}


#if 0
class InputRenameMe
{
public:

	static util::ConfigLoadStatus load_input(const Json::Value& root, void* context)
	{
		InputRenameMe* in = static_cast<InputRenameMe*>(context);

		const Json::Value& axes_root = root["axes"];
		if (!axes_root.isNull())
		{
			LOGV("load the axes table\n");
			Json::ValueIterator iter = axes_root.begin();
			for (; iter != axes_root.end(); ++iter)
			{
				const Json::Value& axis_name = iter.key();
				const Json::Value& axis_table = (*iter);

				LOGV("axis name: %s\n", axis_name.asString().c_str());
				LOGV("input sources: %i\n", axis_table.size());
			}
		}



		return util::ConfigLoad_Success;
	}

	void load_input_table(const char* path)
	{
		util::json_load_with_callback(path, InputRenameMe::load_input, this, true);
	}
};


// load the input table
//		InputRenameMe irm;
//		irm.load_input_table("conf/input.conf");


#endif



#if 0

		struct TempVertex
		{
			glm::vec2 pos;
			Color color;
			glm::vec2 uv;
		};

		// create the render target and texture for the gui
		image::Image image;
		image.width = 512;
		image.height = 512;
		image.channels = 3;
		gui_texture = ::renderer::driver()->texture_create(image);

		gui_render_target = ::renderer::driver()->render_target_create(image.width, image.height);
		::renderer::driver()->render_target_set_attachment(gui_render_target, ::renderer::RenderTarget::COLOR, 0, gui_texture);
		::renderer::driver()->render_target_set_attachment(gui_render_target, ::renderer::RenderTarget::DEPTHSTENCIL, 0, 0);


		if (alt_window)
		{
			alt_vs.desc.add(::renderer::VD_FLOAT2);
			alt_vs.desc.add(::renderer::VD_FLOAT4);
			alt_vs.desc.add(::renderer::VD_FLOAT2);


			alt_vs.create(6, 10, ::renderer::DRAW_INDEXED_TRIANGLES);

			platform::window::Frame frame = platform::window::get_render_frame(alt_window);
			float cx = frame.width / 2.0f;
			float cy = frame.height / 2.0f;

			if (alt_vs.has_room(4, 6))
			{
				TempVertex* v = (TempVertex*)alt_vs.request(4);

				const float RECT_SIZE = 150.0f;


				// this is intentionally inverted along the y
				// so that texture renderered appears correctly.
				v[0].pos = glm::vec2(cx-RECT_SIZE, cy-RECT_SIZE); v[0].uv = glm::vec2(0,0); v[0].color = Color::from_rgba(255, 255, 255, 255);
				v[1].pos = glm::vec2(cx-RECT_SIZE, cy+RECT_SIZE); v[1].uv = glm::vec2(0,1); v[1].color = Color::from_rgba(255, 255, 255, 255);
				v[2].pos = glm::vec2(cx+RECT_SIZE, cy+RECT_SIZE); v[2].uv = glm::vec2(1,1); v[2].color = Color::from_rgba(255, 255, 255, 255);
				v[3].pos = glm::vec2(cx+RECT_SIZE, cy-RECT_SIZE); v[3].uv = glm::vec2(1,0); v[3].color = Color::from_rgba(255, 255, 255, 255);

				::renderer::IndexType indices[] = {0, 1, 2, 2, 3, 0};
				alt_vs.append_indices(indices, 6);
				alt_vs.update();
			}
		}

#endif
