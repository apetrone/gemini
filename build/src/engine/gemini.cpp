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

class EntityManager : public IEntityManager
{
	gemini::IEngineEntity* entity_list[MAX_ENTITIES];
	size_t index;

public:
	EntityManager() : index(0)
	{
		memset(entity_list, 0, sizeof(gemini::IEngineEntity*)*MAX_ENTITIES);
	}

	virtual void add(IEngineEntity* entity);
	virtual void remove(IEngineEntity* entity);

	virtual void startup();
	virtual void shutdown();

	IEngineEntity** get_entity_list() { return entity_list; }
};


void EntityManager::add(IEngineEntity* entity)
{
	entity_list[index++] = entity;
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

void EntityManager::startup()
{

}

void EntityManager::shutdown()
{

}


class ModelInterface : public gemini::IModelInterface
{
	// Each entity that has a model associated with it
	// will have a model instance data allocated.
	class ModelInstanceData : public IModelInstanceData
	{
		AssetHandle mesh_handle;
		Mesh* mesh;
		glm::mat4 transform;

		// parent-local bone transforms
		glm::mat4* local_bone_transforms;

		// model bone transforms
		glm::mat4* model_bone_transforms;

		glm::mat4* inverse_bind_transforms;

		Channel<glm::vec3> scale_channel;
		Channel<glm::quat> rotation_channel;
		Channel<glm::vec3> translation_channel;

		glm::vec3 scale;
		glm::quat rotation;
		glm::vec3 translation;

		std::vector<animation::SequenceId> animations;

		gemini::Allocator& allocator;
	public:

		ModelInstanceData(gemini::Allocator& allocator)
			: mesh(nullptr)
			, local_bone_transforms(0)
			, model_bone_transforms(0)
			, inverse_bind_transforms(0)
			, scale_channel(scale)
			, rotation_channel(rotation)
			, translation_channel(translation)
			, allocator(allocator)
		{
		}

		void set_mesh_index(AssetHandle in_mesh_handle)
		{
			mesh_handle = in_mesh_handle;
			mesh = mesh_from_handle(mesh_handle);
		}

		void create_bones()
		{
			assert(mesh != 0);

			assert(0); // TODO: 01-19-17: fix this (meshes)
			// does this have an animation?
			//if (mesh->has_skeletal_animation)
			//{
			//	size_t total_elements = (mesh->geometry.size() * mesh->skeleton.size());
			//	local_bone_transforms = new glm::mat4[total_elements];
			//	model_bone_transforms = new glm::mat4[total_elements];
			//	inverse_bind_transforms = new glm::mat4[total_elements];
			//}
		}

		void destroy_bones()
		{
			if (local_bone_transforms)
			{
				delete [] local_bone_transforms;
				local_bone_transforms = 0;
			}

			if (model_bone_transforms)
			{
				delete [] model_bone_transforms;
				model_bone_transforms = 0;
			}

			if (inverse_bind_transforms)
			{
				delete [] inverse_bind_transforms;
				inverse_bind_transforms = 0;
			}
		}

		virtual AssetHandle asset_index() const { return mesh_handle; }
		virtual glm::mat4& get_local_transform() { return transform; }
		virtual void set_local_transform(const glm::mat4& _transform) { transform = _transform; }
		virtual glm::mat4* get_model_bone_transforms(uint32_t geometry_index) const
		{
			if (!model_bone_transforms)
			{
				return nullptr;
			}
			assert(0); // TODO: 01-19-17: fix this (meshes)
			return nullptr; // &model_bone_transforms[mesh->skeleton.size() * geometry_index];
		}

		virtual const Hitbox* get_hitboxes() const
		{
			if (!model_bone_transforms)
			{
				return nullptr;
			}

			assert(0); // TODO: 01-19-17: fix this (meshes)
			return nullptr;
			//return &mesh->hitboxes[0];
		}

		virtual glm::mat4* get_inverse_bind_transforms(uint32_t geometry_index) const
		{
			if (!inverse_bind_transforms)
			{
				return nullptr;
			}
			assert(0); // TODO: 01-19-17: fix this (meshes)
			return nullptr;
			//return &inverse_bind_transforms[mesh->skeleton.size() * geometry_index];
		}

		virtual uint32_t get_total_transforms() const
		{
			assert(0); // TODO: 01-19-17: fix this (meshes)
			return 0;
			//return mesh->skeleton.size();
		}

		virtual void set_animation_enabled(int32_t index, bool enabled)
		{
			animation::SequenceId global_instance_index = animations[index];
			animation::AnimatedInstance* instance = animation::get_instance_by_index(global_instance_index);
			assert(instance != 0);
			if (instance)
			{
				instance->enabled = enabled;
			}
		}

		virtual void get_animation_pose(int32_t index, glm::vec3* positions, glm::quat* rotations)
		{
			animation::SequenceId instance_index = animations[index];
			animation::AnimatedInstance* instance = animation::get_instance_by_index(instance_index);

#if defined(GEMINI_DEBUG_BONES)
			const glm::vec2 origin(10.0f, 30.0f);
#endif

			const size_t total_joints = instance->animation_set.size() / ANIMATION_KEYFRAME_VALUES_MAX;

			for (size_t bone_index = 0; bone_index < total_joints; ++bone_index)
			{
				animation::Channel* channel = &instance->channel_set[bone_index * ANIMATION_KEYFRAME_VALUES_MAX];

				assert(bone_index < MAX_BONES);

				glm::vec3& pos = positions[bone_index];
				glm::quat& rot = rotations[bone_index];
				const animation::Channel& tx = channel[0];
				const animation::Channel& ty = channel[1];
				const animation::Channel& tz = channel[2];
				pos = glm::vec3(tx(), ty(), tz());

				const animation::Channel& rx = channel[3];
				const animation::Channel& ry = channel[4];
				const animation::Channel& rz = channel[5];
				const animation::Channel& rw = channel[6];

				rot = glm::quat(rw(), rx(), ry(), rz());

#if defined(GEMINI_DEBUG_BONES)
				debugdraw::text(origin.x,
					origin.y + (12.0f * bone_index),
					core::str::format("%2i) '%s' | rot: [%2.2f, %2.2f, %2.2f, %2.2f]", bone_index,
					mesh->skeleton[bone_index].name(),
					rot.x, rot.y, rot.z, rot.w),
					Color(0.0f, 0.0f, 0.0f));
#endif
			}
		}

		virtual void set_pose(glm::vec3* positions, glm::quat* rotations)
		{
			assert(0); // TODO: 01-19-17: fix this (meshes)
#if 0
			if (mesh->skeleton.empty())
			{
				return;
			}

			// You've hit the upper bounds for skeletal bones for a single
			// model. Congrats.
			assert(mesh->skeleton.size() < MAX_BONES);

			Hitbox* hitboxes = &mesh->hitboxes[0];

			size_t geometry_index = 0;
			// we must update the transforms for each geometry instance
			for (const assets::Geometry* geo : mesh->geometry)
			{
				// If you hit this assert, one mesh in this model didn't have
				// blend weights.
				assert(!geo->bind_poses.empty());

				size_t transform_index;

				for (size_t index = 0; index < mesh->skeleton.size(); ++index)
				{
					transform_index = (geometry_index * mesh->skeleton.size()) + index;
					assets::Joint* joint = &mesh->skeleton[index];

					glm::mat4& local_bone_pose = local_bone_transforms[transform_index];
					glm::mat4& model_pose = model_bone_transforms[transform_index];

					glm::mat4 parent_pose;
					glm::mat4& inverse_bind_pose = inverse_bind_transforms[transform_index];

					glm::mat4 local_rotation = glm::toMat4(rotations[index]);
					glm::mat4 local_transform = glm::translate(glm::mat4(1.0f), positions[index]);

					const glm::mat4 local_pose = local_transform * local_rotation;
					if (joint->parent_index > -1)
					{
						parent_pose = model_bone_transforms[joint->parent_index];
					}

					// this will be cached in local transforms
					local_bone_pose = geo->bind_poses[index] * local_pose;

					// this will be used for skinning in the vertex shader
					model_pose = parent_pose * local_bone_pose;

					// set the inverse_bind_pose
					inverse_bind_pose = geo->inverse_bind_poses[index];

					glm::mat4 local_bbox_xf = glm::toMat4(glm::angleAxis(glm::radians(15.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
					Hitbox* hitbox = (hitboxes + index);
					//glm::vec3 pos(0.0f, 0.0f, 0.0f);
					//glm::vec3 dims(0.5f, 0.5f, 0.5f);
					//pos = mathlib::transform_point(local_bone_pose, pos);
					//debugdraw::box(-dims + pos, dims + pos, gemini::Color(0.0f, 1.0f, 1.0f));
					debugdraw::axes(glm::mat4(hitbox->rotation) * model_pose, 1.0f, 0.0f);
				}

				++geometry_index;
			}
#endif
		}

		virtual int32_t get_animation_index(const char* name)
		{
			size_t index = 0;
			for (const animation::SequenceId& id : animations)
			{
				animation::AnimatedInstance* instance = animation::get_instance_by_index(id);
				animation::Sequence* sequence = animation::get_sequence_by_index(instance->sequence_index);
				if (0 == core::str::case_insensitive_compare(name, sequence->name(), 0))
				{
					return index;
					break;
				}
				++index;
			}

			return -1;
		}

		virtual int32_t add_animation(const char* name)
		{
			animation::SequenceId id = animation::load_sequence(allocator, name, mesh);
			if (id > -1)
			{
				animations.push_back(id);
				LOGV("[engine] added animation %s to index: %i\n", name, animations.size()-1);
				return animations.size()-1;
			}
			else
			{
				LOGW("Unable to load sequence %s\n", name);
				return -1;
			}
		}

		virtual int32_t get_total_animations() const
		{
			return animations.size();
		}

		virtual void reset_channels(int32_t index)
		{
			animation::SequenceId instance_index = animations[index];
			animation::AnimatedInstance* instance = animation::get_instance_by_index(instance_index);

			// reset all the channels
			instance->reset_channels();

			// force an advance, to fetch the first frame
			// but don't advance time.
			instance->advance(0.0f);
		}

		virtual float get_animation_duration(int32_t index) const
		{
			float duration_seconds = 0;
			animation::SequenceId instance_index = animations[index];
			animation::AnimatedInstance* instance = animation::get_instance_by_index(instance_index);
			assert(instance != 0);

			animation::Sequence* sequence = animation::get_sequence_by_index(instance->sequence_index);
			assert(sequence != 0 );

			duration_seconds = sequence->duration_seconds;

			return duration_seconds;
		}

		virtual uint32_t get_total_bones(int32_t /*index*/) const
		{
			assert(mesh != nullptr);
			return mesh->skeleton.size();
		}

		virtual int32_t find_bone_named(const char* bone)
		{
			assert(0); // TODO: 01-19-17: fix this (meshes)
			//for (const assets::Joint& joint : mesh->skeleton)
			//{
			//	if (joint.name == bone)
			//		return joint.index;
			//}

			return -1;
		}

		virtual void get_local_bone_pose(int32_t /*animation_index*/, int32_t bone_index, glm::vec3& position, glm::quat& rotation)
		{
			assert(bone_index != -1);

			const glm::mat4 model_matrix = local_bone_transforms[bone_index];
			rotation = glm::toQuat(model_matrix);
			position = glm::vec3(glm::column(model_matrix, 3));
		}

		virtual void get_model_bone_pose(int32_t /*animation_index*/, int32_t bone_index, glm::vec3& position, glm::quat& rotation)
		{
			assert(bone_index != -1);

			const glm::mat4& model_matrix = model_bone_transforms[bone_index];
			rotation = glm::toQuat(model_matrix);
			position = glm::vec3(glm::column(model_matrix, 3));
		}

		virtual const glm::vec3& get_mins() const
		{
			assert(0); // TODO: 01-19-17: fix this (meshes)
			return glm::vec3();
			//return mesh->aabb_mins;
		}

		virtual const glm::vec3& get_maxs() const
		{
			assert(0); // TODO: 01-19-17: fix this (meshes)
			return glm::vec3();
			//return mesh->aabb_maxs;
		}

		virtual const glm::vec3& get_center_offset() const
		{
			assert(0); // TODO: 01-19-17: fix this (meshes)
			//return mesh->mass_center_offset;
			return glm::vec3();
		}
	};


	typedef std::map<int32_t, ModelInstanceData> ModelInstanceMap;
	ModelInstanceMap id_to_instance;

	gemini::Allocator& allocator;


public:
	ModelInterface(gemini::Allocator& _allocator)
		: allocator(_allocator)
	{
	}

	virtual int32_t create_instance_data(const char* model_path)
	{
		AssetHandle mesh_handle = mesh_load(model_path);
		Mesh* mesh = mesh_from_handle(mesh_handle);
		if (mesh)
		{
			if (mesh->is_dirty)
			{
				mesh->prepare_geometry();
			}
			ModelInstanceData data(allocator);
			data.set_mesh_index(mesh_handle);
			data.create_bones();
			int32_t index = (int32_t)id_to_instance.size();
			id_to_instance.insert(ModelInstanceMap::value_type(index, data));
			return index;
		}

		return -1;
	}

	virtual void destroy_instance_data(int32_t index)
	{
		ModelInstanceMap::iterator it = id_to_instance.find(index);
		if (it != id_to_instance.end())
		{
			ModelInstanceData& data = it->second;
			data.destroy_bones();

			id_to_instance.erase(it);
		}
	}


	IModelInstanceData* get_instance_data(int32_t index)
	{
		ModelInstanceMap::iterator it = id_to_instance.find(index);
		if (it != id_to_instance.end())
		{
			return &(*it).second;
		}

		return 0;
	}
};


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





class EngineInterface : public IEngineInterface
{
	IEntityManager* entity_manager;
	IModelInterface* model_interface;
	gemini::physics::IPhysicsInterface* physics_interface;
	IExperimental* experimental_interface;

	platform::window::NativeWindow* main_window;
	//SceneLink& scenelink;
	render2::Device* device;

	//Array<char> render_stream_data;
	//Array<::renderer::RenderState> render_commands;

	gemini::Allocator& engine_allocator;

public:

	EngineInterface(gemini::Allocator& _allocator,
					IEntityManager* em,
					IModelInterface* mi,
					gemini::physics::IPhysicsInterface* pi,
					IExperimental* ei,
					//SceneLink& scene_link,
					render2::Device* render_device,
					platform::window::NativeWindow* window)
		: engine_allocator(_allocator)
		, entity_manager(em)
		, model_interface(mi)
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
	}


	virtual ~EngineInterface() {};

	virtual IEntityManager* entities() { return entity_manager; }
	virtual IModelInterface* models() { return model_interface; }
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


		render2::Pass render_pass;
		render_pass.target = device->default_render_target();
		render_pass.color(0.0f, 0.0f, 0.0f, 1.0f);
		render_pass.clear_color = true;
		render_pass.clear_depth = true;
		render_pass.depth_test = false;

		render2::CommandQueue* queue = device->create_queue(render_pass);
		//render2::CommandSerializer* serializer = device->create_serializer(queue);
		device->queue_buffers(queue, 1);
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
};



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
	ModelInterface model_interface;
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
#endif
	}



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
		, model_interface(engine_allocator)
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

		if (game_interface)
		{
			game_interface->on_event(event);
		}
	}

	virtual void event(kernel::MouseEvent& event)
	{
		if (game_interface)
		{
			game_interface->on_event(event);
		}
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

		if (game_interface)
		{
			game_interface->on_event(event);
		}
	}

	virtual void event(kernel::GameControllerEvent& event)
	{
		if (game_interface)
		{
			//if (event.subtype == kernel::JoystickButton && event.is_down)
			//{
			//	gemini::audio::SoundHandle_t sound_handle = gemini::audio::play_sound(test_sound, 0);
			//}

			game_interface->on_event(event);
		}
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

		// initialize rendering subsystems
		render2::RenderParameters render_params(renderer_allocator);
		// set some options
		render_params["vsync"] = "true";
		render_params["double_buffer"] = "true";
		render_params["depth_size"] = "24";
		render_params["multisample"] = "4";

		render_params["gamma_correct"] = "true";

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
			&model_interface,
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

		// record the current frametime milliseconds
#if defined(DEBUG_FRAMERATE)
		graph->record_value(params.framedelta_milliseconds, 0);
#endif
		// set the baseline for the font
		int x = 250;
		int y = 16;
		debugdraw::text(x, y, core::str::format("frame delta = %2.2fms\n", params.framedelta_milliseconds), Color());
		y += 12;
		//debugdraw::text(x, y, core::str::format("allocations = %i, total %2.2f MB\n",
		//	core::memory::global_allocator().get_zone()->get_active_allocations(),
		//	core::memory::global_allocator().get_zone()->get_active_bytes() / (float)(1024 * 1024)), Color());
		//y += 12;

		// causing jittery frametime graph.
		//while (accumulator >= params.step_interval_seconds)
		{
			if (game_interface)
			{
				game_interface->tick(params.current_tick, params.step_interval_seconds, params.step_alpha);
			}

			// this is going to be incorrect unless this is placed in the step.
			// additionally, these aren't interpolated: figure how to; for example,
			// draw hit boxes for a moving player with this system.
			//debugdraw::update(params.step_interval_seconds /** MillisecondsPerSecond*/);

			// subtract the interval from the accumulator
			accumulator -= params.step_interval_seconds;

			// increment tick counter
			params.current_tick++;
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


		// calculate delta ticks in milliseconds
		params.framedelta_milliseconds = (current_time - last_time) * SecondsPerMillisecond;

		// cache the value in seconds
		params.framedelta_seconds = params.framedelta_milliseconds * SecondsPerMillisecond;
		last_time = current_time;

		//gemini::profiler::report();
		//gemini::profiler::reset();
	}

	void post_tick()
	{
		platform::window::activate_context(main_window);

		if (game_interface)
		{
			game_interface->execute_frame(kernel::parameters().framedelta_seconds);
		}

		if (compositor)
		{
			compositor->tick(kernel::parameters().framedelta_milliseconds);
		}

		if (draw_physics_debug)
		{
			physics::debug_draw();
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
		assets::shutdown();

		IAudioInterface* interface = audio::instance();
		MEMORY2_DELETE(audio_allocator, interface);
		audio::set_instance(nullptr);

		// must shutdown the renderer before our window
		render2::destroy_device(renderer_allocator, device);

		platform::window::destroy(main_window);
		main_window = 0;
		platform::window::shutdown();

#if defined(GEMINI_ENABLE_PROFILER)
		gemini::profiler::report();
#endif

		gemini::runtime_shutdown();

		MEMORY2_DELETE(engine_allocator, engine_interface);
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
