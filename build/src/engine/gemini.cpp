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

#include <runtime/debugvar.h>
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

#include "audio.h"
#include "animation.h"
#include "physics/physics.h"
#include "hotloading.h"
#include "navigation.h"

#include <engine/model_instance_data.h>

// for MAX_BONES
#include <shared/shared_constants.h>

#include <rapid/rapid.h>

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

	// If you hit this, more entities have been created than will fit
	// in a short.
	assert(index < USHRT_MAX);
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
public:
	Experimental()
	{
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
	render2::Device* device;

	gemini::Allocator& engine_allocator;

	// model instance data
	typedef std::map<int32_t, gemini::ModelInstanceData> ModelInstanceMap;
	ModelInstanceMap id_to_instance;

public:
	RenderScene* render_scene;

	EngineInterface(gemini::Allocator& _allocator,
					IEntityManager* em,
					gemini::physics::IPhysicsInterface* pi,
					IExperimental* ei,
					render2::Device* render_device,
					platform::window::NativeWindow* window)
		: engine_allocator(_allocator)
		, entity_manager(em)
		, physics_interface(pi)
		, experimental_interface(ei)
		, main_window(window)
		, device(render_device)
		, render_scene(nullptr)
	{
	}


	virtual ~EngineInterface()
	{
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
	}

	virtual void deallocate(void* pointer)
	{
		MEMORY2_DEALLOC(engine_allocator, pointer);
	}

	virtual Allocator& allocator()
	{
		return engine_allocator;
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

	virtual void play_animation(IModelInstanceData* model, const char* animation_name)
	{
		gemini::ModelInstanceData* instance = reinterpret_cast<gemini::ModelInstanceData*>(model);
		render_scene_animation_play(render_scene, instance->get_component_index(), animation_name);
	}

	virtual bool is_animation_finished(IModelInstanceData* model)
	{
		gemini::ModelInstanceData* instance = reinterpret_cast<gemini::ModelInstanceData*>(model);

		return render_scene_animation_finished(render_scene, instance->get_component_index());
	}

	// IModelInterface
	virtual int32_t create_instance_data(uint16_t entity_index, const char* model_path);
	virtual void destroy_instance_data(int32_t index);
	virtual IModelInstanceData* get_instance_data(int32_t index);
}; // EngineInterface

int32_t EngineInterface::create_instance_data(uint16_t entity_index, const char* model_path)
{
	AssetHandle mesh_handle = mesh_load(model_path);
	Mesh* mesh = mesh_from_handle(mesh_handle);
	if (mesh)
	{
		uint32_t component_id = 0;
		if (mesh->skeleton.empty())
		{
			component_id = render_scene_add_static_mesh(render_scene, mesh_handle, entity_index, glm::mat4(1.0f));
		}
		else
		{
			component_id = render_scene_add_animated_mesh(render_scene, mesh_handle, entity_index, glm::mat4(1.0f));
		}

		gemini::ModelInstanceData data(engine_allocator);
		data.set_mesh_index(mesh_handle);
		data.set_component_index(component_id);

		int32_t index = (int32_t)id_to_instance.size();
		id_to_instance.insert(ModelInstanceMap::value_type(index, data));

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

		AssetHandle mesh_handle = data.get_mesh_handle();
		Mesh* mesh = mesh_from_handle(mesh_handle);

		uint32_t component_id = data.get_component_index();
		if (mesh->skeleton.empty())
		{
			render_scene_remove_static_mesh(render_scene, component_id);
		}
		else
		{
			render_scene_remove_animated_mesh(render_scene, component_id);
		}

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




void extract_entities(EntityRenderState* ers, IEngineEntity** entities)
{
	for (size_t index = 0; index < MAX_ENTITIES; ++index)
	{
		IEngineEntity* entity = entities[index];
		if (entity)
		{
			entity->get_world_transform(ers->position[index], ers->orientation[index]);
			entity->get_render_position(ers->position[index]);
			entity->get_pivot_point(ers->pivot_point[index]);
		}
	}
}

void interpolate_states(EntityRenderState* out, EntityRenderState* a, EntityRenderState* b, float alpha)
{
	for (size_t index = 0; index < MAX_ENTITIES; ++index)
	{
		glm::quat orientation = gemini::slerp(a->orientation[index], b->orientation[index], alpha);
		glm::vec3 position = gemini::lerp(a->position[index], b->position[index], alpha);

		// Wait, since when does a pivot point get interpolated ?
		// I have no idea what this produces.
		glm::vec3 pivot_point = gemini::lerp(a->pivot_point[index], b->pivot_point[index], alpha);

		glm::mat4 rotation = glm::toMat4(orientation);
		glm::mat4 translation = glm::translate(glm::mat4(1.0f), position);
		glm::mat4 to_pivot = glm::translate(glm::mat4(1.0f), -pivot_point);
		glm::mat4 from_pivot = glm::translate(glm::mat4(1.0f), pivot_point);

		out->model_matrix[index] = translation * from_pivot * rotation * to_pivot;
	}
}

void copy_state(EntityRenderState* out, EntityRenderState* in)
{
	for (size_t index = 0; index < MAX_ENTITIES; ++index)
	{
		out->orientation[index] = in->orientation[index];
		out->position[index] = in->position[index];
		out->pivot_point[index] = in->pivot_point[index];
	}
}


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
	bool debug_camera;

	platform::window::NativeWindow* main_window;

	Array<gemini::GameMessage>* queued_messages;

	// Kernel State variables
	double accumulator;
	uint64_t last_time;

	// rendering
	render2::Device* device;

	// game library
	platform::DynamicLibrary* gamelib;
	disconnect_engine_fn disconnect_engine;
	EntityManager entity_manager;
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

	float interpolate_alpha;
	RenderScene* render_scene;
	EntityRenderState* entity_render_state;

	CameraState camera_state[2];


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

			game_interface->startup(compositor, root);
		}
#else
		game_interface = ::connect_engine(gemini::engine::instance());
		if (!game_interface)
		{
			LOGE("Unable to connect engine to game library\n");
			assert(game_interface != 0);
		}

		game_interface->startup(compositor, root);
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

public:
	EngineKernel()
		: active(true)
		, draw_physics_debug(false)
		, draw_navigation_debug(false)
		, debug_camera(false)
		, accumulator(0.0f)
		, last_time(0)
		, engine_interface(0)
		, game_interface(0)
		, engine_allocator(memory_allocator_default(MEMORY_ZONE_DEFAULT))
		, queued_messages(nullptr)
		, interpolate_alpha(0.0f)
	{
		game_path = "";
		compositor = nullptr;
		gui_renderer = nullptr;
		resource_cache = nullptr;
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
	} // queue_game_message

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
			else if (event.key == gemini::BUTTON_SPACE)
			{
				debug_camera = !debug_camera;
				LOGV("debug_camera = %s\n", debug_camera ? "ON" : "OFF");
			}

			if (event.key == BUTTON_F2)
			{
				runtime_unload_rapid();
				LOGV("unloaded rapid interface\n");
			}
			else if (event.key == BUTTON_F3)
			{
				runtime_load_rapid();
				LOGV("loading rapid interface\n");
			}
		}

		queue_game_message(event_to_gamemessage(event, kernel::parameters().current_physics_tick));
	} // event

	virtual void event(kernel::MouseEvent& event)
	{
		queue_game_message(event_to_gamemessage(event, kernel::parameters().current_physics_tick));
	} // event


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
	} // event


	virtual void event(kernel::GameControllerEvent& event)
	{
		queue_game_message(event_to_gamemessage(event, kernel::parameters().current_physics_tick));
	} // event


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
	} // setup_gui


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

		entity_render_state = static_cast<EntityRenderState*>(MEMORY2_ALLOC(engine_allocator, sizeof(EntityRenderState) * 2));
		memset(entity_render_state, 0, sizeof(EntityRenderState) * 2);
		queued_messages = MEMORY2_NEW(engine_allocator, Array<GameMessage>)(engine_allocator);

		// initialize rendering subsystems
		render2::RenderParameters render_params(renderer_allocator);

		// set some options
		render_params["double_buffer"] = "true";
		render_params["depth_size"] = "24";
		render_params["multisample"] = "4";

		render_params["gamma_correct"] = "false";
		render_params["vsync"] = "false";

		// set opengl specific options
		render_params["rendering_backend"] = "opengl";
		render_params["opengl.major"] = "3";
		render_params["opengl.minor"] = "2";
		render_params["opengl.profile"] = "core";
		render_params["opengl.share_context"] = "true";

		device = render2::create_device(renderer_allocator, render_params);
		assert(device != nullptr);

		device->init(config.window_width, config.window_height);

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
			physics::instance(),
			&experimental,
			device,
			main_window
		);
		gemini::engine::set_instance(engine_interface);

		// create the render scene
		render_scene = render_scene_create(engine_allocator, device);

		EngineInterface* engine_instance = static_cast<EngineInterface*>(engine_interface);
		engine_instance->render_scene = render_scene;

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
	} // startup


	virtual void tick()
	{
		uint64_t current_time = platform::microseconds();
		kernel::Parameters& params = kernel::parameters();

		PROFILE_BEGIN("platform_update");
		platform::update(kernel::parameters().framedelta_milliseconds);
		PROFILE_END("platform_update");


		// calculate delta ticks in milliseconds
		params.framedelta_milliseconds = (current_time - last_time) * MillisecondsPerMicrosecond;

		// cache the value in seconds
		params.framedelta_seconds = params.framedelta_milliseconds * SecondsPerMillisecond;

		interpolate_alpha += kernel::parameters().framedelta_seconds;

		// record the current frametime milliseconds
#if defined(DEBUG_FRAMERATE)
		graph->record_value(params.framedelta_milliseconds, 0);
#endif

		last_time = current_time;

		// update accumulator
		accumulator += params.framedelta_seconds;

		// set the baseline for the font
		int x = 250;
		int y = 16;
		debugdraw::text(x, y, core::str::format("frame delta = %2.2fms (%i fps)\n",
												params.framedelta_milliseconds,
												static_cast<uint32_t>(1000.0f / params.framedelta_milliseconds)),
												Color());
		y += 12;
		//debugdraw::text(x, y, core::str::format("allocations = %i, total %2.2f MB\n",
		//	core::memory::global_allocator().get_zone()->get_active_allocations(),
		//	core::memory::global_allocator().get_zone()->get_active_bytes() / (float)(1024 * 1024)), Color());
		//y += 12;

		uint32_t reset_queue = 0;

		EngineInterface* ei = reinterpret_cast<EngineInterface*>(engine_interface);


		while (accumulator > params.step_interval_seconds)
		{
			// iterate over queued messages and play until we hit the time cap
			for (size_t index = 0; index < queued_messages->size(); ++index)
			{
				if ((*queued_messages)[index].timestamp <= params.current_physics_tick)
				{
					if (game_interface)
					{
						game_interface->handle_game_message((*queued_messages)[index]);
					}
				} // execute
			}

			if (game_interface)
			{
				game_interface->fixed_step(params.current_physics_tick, params.step_interval_seconds);
				engine::instance()->physics()->step_simulation(params.step_interval_seconds);
			}


			// subtract the interval from the accumulator
			accumulator -= params.step_interval_seconds;

			// increment tick counter
			params.current_physics_tick++;

			reset_queue = 1;
		}

		if (game_interface)
		{
			game_interface->tick(params.current_physics_tick, params.framedelta_seconds);

			if (reset_queue)
			{
				debugdraw::update(params.step_interval_seconds);
			}
		}

		if (reset_queue)
		{
			static float the_time = 0.0f;
			render_scene->light_position_world.x = cosf(the_time);
			render_scene->light_position_world.y = 2.0f;
			render_scene->light_position_world.z = sinf(the_time);
			the_time += 0.01f;

			copy_state(&entity_render_state[0], &entity_render_state[1]);

			extract_entities(&entity_render_state[1], entity_manager.get_entity_list());
			interpolate_alpha = 0.0f;

			camera_state[0] = camera_state[1];
			game_interface->extract_camera(&camera_state[1], nullptr);

			// 1. Assess camera position
			// 2. Correct camera position
			// 3. Determine if camera is too close to the player
			// 4. optionally: fade out player when too close
			//LOGV("Collision Correct the camera position?\n");

			queued_messages->resize(0);

			if (game_interface)
			{
				game_interface->render_frame(0.0f);
				game_interface->reset_events();
			}
		}

		params.step_alpha = accumulator / params.step_interval_seconds;
		if (params.step_alpha >= 1.0f)
		{
			params.step_alpha -= 1.0f;
		}

		animation::update(kernel::parameters().framedelta_seconds);
		hotloading::tick();
		post_tick();
		kernel::parameters().current_frame++;

		//gemini::profiler::report();
		//gemini::profiler::reset();
	} // tick

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
			float alpha = glm::clamp(static_cast<float>(accumulator / kernel::parameters().step_interval_seconds), 0.0f, 1.0f);

			/*game_interface->render_frame(alpha);*/

			// render the main view
			gemini::View view;
			const gemini::Color background_color = gemini::Color::from_rgba(128, 128, 128, 255);

			platform::window::Frame frame = platform::window::get_render_frame(main_window);
			view.width = frame.width;
			view.height = frame.height;

			glm::vec3 player_offset;
			game_interface->get_render_view(view, player_offset);

			// interpolate
			EntityRenderState ers;

			interpolate_states(&ers, &entity_render_state[0], &entity_render_state[1], alpha);

			CameraState interpolated_camera_state;

			const uint32_t enable_interpolation = 1;
			if (enable_interpolation)
			{
				interpolated_camera_state.world_position = gemini::lerp(camera_state[0].world_position, camera_state[1].world_position, alpha);
				interpolated_camera_state.position = gemini::lerp(camera_state[0].position, camera_state[1].position, alpha);
				interpolated_camera_state.distance_from_pivot = gemini::lerp(camera_state[0].distance_from_pivot, camera_state[1].distance_from_pivot, alpha);
				interpolated_camera_state.rotation = gemini::slerp(camera_state[0].rotation, camera_state[1].rotation, alpha);
				interpolated_camera_state.field_of_view = gemini::lerp(camera_state[0].field_of_view, camera_state[1].field_of_view, alpha);
				interpolated_camera_state.vertical_offset = gemini::lerp(camera_state[0].vertical_offset, camera_state[1].vertical_offset, alpha);
				interpolated_camera_state.horizontal_offset = gemini::lerp(camera_state[0].horizontal_offset, camera_state[1].horizontal_offset, alpha);
				interpolated_camera_state.view = gemini::lerp(camera_state[0].view, camera_state[1].view, alpha);
			}
			else
			{
				interpolated_camera_state = camera_state[1];
			}

			// setup the inverse camera transform.
			//glm::mat4 to_world = glm::translate(glm::mat4(), -interpolated_camera_state.position);

			glm::vec3 cam_origin;

			RapidInterface* rapid = runtime_rapid();
			if (rapid)
			{
				view.modelview = rapid->camera_test(interpolated_camera_state.world_position,
																interpolated_camera_state.position,
																interpolated_camera_state.rotation);
			}
			else
			{
				view.modelview = camera_state_to_transform(interpolated_camera_state);// *to_world;
				glm::vec4 row = glm::column(view.modelview, 3);
			}

			cam_origin = glm::vec3(glm::column(view.modelview, 3));
			view.modelview = glm::inverse(view.modelview);

			//glm::mat4 tx = glm::translate(glm::mat4(1.0f), player_offset);

			//debugdraw::axes(glm::inverse(tx) * view.modelview, 1.0f, 0.0f);

			//debugdraw::camera(
			//	cam_origin,
			//	interpolated_camera_state.view,
			//	0.0f
			//);

			if (debug_camera)
			{
				// 5 is centered
				const glm::vec3 wall_collision_offset(5.f, -1.1f, 0.0f);
				const float pitch_degrees = 0.0f; // -35.0f
				view.modelview = glm::inverse(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 3.0f, 0.0f) + wall_collision_offset) * glm::toMat4(glm::angleAxis(glm::radians(pitch_degrees), glm::vec3(1.0f, 0.0f, 0.0f))));
			}

			// this is what happens when we interpolate the vectors; but suffers artifacts from lerping vector used as orientation.
			//view.modelview = glm::lookAt(interpolated_camera_pos, interpolated_camera_pos + interpolated_target_pos, glm::vec3(0.0f, 1.0f, 0.0f));

			const float nearz = 0.01f;
			const float farz = 4096.0f;
			const float screen_aspect_ratio = (view.width / (float)view.height);
			view.projection = glm::perspective(glm::radians(interpolated_camera_state.field_of_view), screen_aspect_ratio, nearz, farz);


			render_scene_update(render_scene, &ers);
			render_scene_draw(render_scene, device, view.modelview, view.projection);

			//debugdraw::text(30, 230, core::str::format("static meshes: %i", render_scene->stat_static_meshes_drawn), Color(1.0f, 1.0f, 0.5f));
			//debugdraw::text(30, 244, core::str::format("animated meshes: %i", render_scene->stat_animated_meshes_drawn), Color(1.0f, 1.0f, 0.5f));

			// draw all render vars...
			uint32_t x_offset = 10;
			uint32_t y_offset = 150;

			for (DebugVarBase* variable = debugvar_first(); variable != nullptr;)
			{
				debugdraw::text(x_offset,
								y_offset,
								variable->to_string(),
								gemini::Color(1.0f, 1.0f, 1.0f));
				variable = variable->next;
				y_offset += 12;
			}


			debugdraw::render(view.modelview, view.projection, view.width, view.height);
			if (_compositor)
			{
				_compositor->draw();
			}
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
	} // post_tick

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

		render_scene_destroy(render_scene, device);
		render_scene_shutdown();

		// shutdown subsystems
		hotloading::shutdown();
		animation::shutdown();
		gemini::physics::shutdown();
		debugdraw::shutdown();
		audio::shutdown();

		assets::shutdown();

		IAudioInterface* interface = audio::instance();
		MEMORY2_DELETE(audio_allocator, interface);
		audio::set_instance(nullptr);

		MEMORY2_DEALLOC(engine_allocator, entity_render_state);
		entity_render_state = nullptr;

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
	} // shutdown
}; // EngineKernel

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
