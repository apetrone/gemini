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
#include <platform/kernel.h>
#include <fixedsizequeue.h>

#include <core/core.h>
#include <core/str.h>
#include <core/logging.h>
#include <core/filesystem.h>
#include <core/configloader.h>
#include <core/argumentparser.h>
#include <core/mathlib.h>

#include <renderer/renderer.h>
#include <renderer/renderstream.h>
#include <renderer/constantbuffer.h>

// SDK
#include <sdk/audio_api.h>
#include <sdk/entity_api.h>
#include <sdk/iengineentity.h>
#include <sdk/model_api.h>
#include <sdk/engine_api.h>
#include <sdk/game_api.h>
#include <sdk/experimental_api.h>
#include <sdk/debugdraw_api.h>
#include <sdk/shared.h>
#include <sdk/physics_api.h>

#include "input.h"

#include "debugdraw.h"
#include "assets/asset_font.h"
#include "assets/asset_shader.h"
#include "assets/asset_mesh.h"
#include "scenelink.h"
#include "audio.h"
#include "animation.h"
#include "physics/physics.h"
#include "hotloading.h"
#include "vr.h"
#include "navigation.h"

#include <platform/windowlibrary.h>

typedef FixedSizeQueue<gemini::GameMessage, 64> EventQueueType;

struct DataInput
{
	bool execute;
	
	glm::quat orientation;
	platform::Serial* device;
	platform::Thread thread_data;
	EventQueueType* event_queue;
};

void data_thread(void* context)
{
	DataInput* block = static_cast<DataInput*>(context);
	
	LOGV("entering data thread\n");
	
	while(block->execute)
	{
		const uint32_t PACKET_SIZE = 128;
		uint8_t buffer[ PACKET_SIZE ];
		if (platform::serial_read(block->device, buffer, PACKET_SIZE) == PACKET_SIZE)
		{
			uint64_t current = platform::microseconds();			
			
			glm::quat* q = static_cast<glm::quat*>((void*)&buffer);
			block->orientation = *q;
//			LOGV("%2.2f, %2.2f, %2.2f, %2.2f\n", q->x, q->y, q->z, q->w);
			
			{
				// push a new event for the latest orientation
				gemini::GameMessage orientation_message;
				orientation_message.type = gemini::GameMessage::Orientation;
				glm::quat flipped(q->w, q->x, -q->z, -q->y);
				glm::quat y = glm::quat(glm::vec3(0, mathlib::degrees_to_radians(180), 0));
				orientation_message.orientation = glm::inverse(y * flipped);
				block->event_queue->push_back(orientation_message);
			}
		}
	}
	
	LOGV("exiting data thread\n");
}

using namespace platform;
using namespace core;
using namespace gemini; // for renderer
using namespace input;

struct Settings
{
	uint32_t physics_tick_rate;
	int32_t debugdraw_max_primitives;
	uint32_t enable_asset_reloading : 1;
	uint32_t vr_render : 1; // render to the VR render target(s)
	uint32_t vr_require_headset : 1; // don't render to VR unless a headset is attached
	
	uint32_t window_width;
	uint32_t window_height;
	
	::renderer::RenderSettings render_settings;
	
	Settings()
	{
		// setup sane defaults
		physics_tick_rate = 60;
		debugdraw_max_primitives = 2048;
		enable_asset_reloading = 0;
		vr_render = false;
		vr_require_headset = false;
		window_width = 1280;
		window_height = 720;
	}
};

static util::ConfigLoadStatus load_render_config(const Json::Value& root, void* data)
{
	::renderer::RenderSettings* settings = static_cast<::renderer::RenderSettings*>(data);
	
	// TODO: there should be a better way to do this?
	if (!root["gamma_correct"].isNull())
	{
		settings->gamma_correct = root["gamma_correct"].asBool();
	}
	
	return util::ConfigLoad_Success;
}

static util::ConfigLoadStatus settings_conf_loader( const Json::Value & root, void * data )
{
	util::ConfigLoadStatus result = util::ConfigLoad_Success;
	
	Settings* cfg = (Settings*)data;
	if ( !cfg )
	{
		return util::ConfigLoad_Failure;
	}
	
	LOGV( "loading settings...\n" );
	const Json::Value& physics_tick_rate = root["physics_tick_rate"];
	if (!physics_tick_rate.isNull())
	{
		cfg->physics_tick_rate = physics_tick_rate.asUInt();
	}
	
	const Json::Value& debugdraw_max_primitives = root["debugdraw_max_primitives"];
	if (!debugdraw_max_primitives.isNull())
	{
		cfg->debugdraw_max_primitives = debugdraw_max_primitives.asInt();
	}
	
	const Json::Value& enable_asset_reloading = root["enable_asset_reloading"];
	if (!enable_asset_reloading.isNull())
	{
		cfg->enable_asset_reloading = enable_asset_reloading.asBool();
	}
	
	const Json::Value& vr_render = root["vr_render"];
	if (!vr_render.isNull())
	{
		cfg->vr_render = vr_render.asBool();
	}
	
	const Json::Value& vr_require_headset = root["vr_require_headset"];
	if (!vr_require_headset.isNull())
	{
		cfg->vr_require_headset = vr_require_headset.asBool();
	}
	
	const Json::Value& window_width = root["window_width"];
	if (!window_width.isNull())
	{
		cfg->window_width = window_width.asInt();
	}
	
	const Json::Value& window_height = root["window_height"];
	if (!window_height.isNull())
	{
		cfg->window_height = window_height.asInt();
	}
	
	const Json::Value& renderer = root["renderer"];
	if (!renderer.isNull())
	{
		// load renderer settings
		result = load_render_config(renderer, &cfg->render_settings);
	}
	
	return result;
}


#include <nom/nom.hpp>
#include <nom/compositor.hpp>
#include <nom/graph.hpp>
#include <nom/button.hpp>

#include "guirenderer.h"

// this is required at the moment because our render method needs it!
gui::Compositor* _compositor = 0;



class GUIStyle : public gui::Style
{
public:
	
	virtual void draw_bounds(gui::Renderer* renderer, const gui::Bounds& bounds, const gui::Color& color)
	{
		renderer->draw_bounds(bounds, color);
	}
	
	
	virtual void draw_font(gui::Renderer* renderer, const gui::FontHandle& handle, const char* string, const gui::Bounds& bounds, const gui::Color& color)
	{
		renderer->font_draw(handle, string, bounds, color);
	}
};



void render_scene_from_camera(gemini::IEngineEntity** entity_list, View& view, SceneLink& scenelink)
{
	// setup constant buffer
	glm::vec3 light_position = view.eye_position;
	::renderer::ConstantBuffer cb;
	cb.modelview_matrix = &view.modelview;
	cb.projection_matrix = &view.projection;
	cb.viewer_direction = &view.view_direction;
	cb.viewer_position = &view.eye_position;
	cb.light_position = &light_position;
	
	// use the entity list to render
	scenelink.clear();
	scenelink.queue_entities(cb, entity_list, MAX_ENTITIES, RENDER_VISIBLE);
	scenelink.sort();
	scenelink.draw(cb);
}

void render_entity_from_camera(gemini::IEngineEntity* entity, View& view, SceneLink& scenelink)
{
	// setup constant buffer
	glm::vec3 light_position = view.eye_position;
	::renderer::ConstantBuffer cb;
	cb.modelview_matrix = &view.modelview;
	cb.projection_matrix = &view.projection;
	cb.viewer_direction = &view.view_direction;
	cb.viewer_position = &view.eye_position;
	cb.light_position = &light_position;
	
	scenelink.clear();
	
	scenelink.queue_entities(cb, &entity, 1, RENDER_VIEWMODEL);
	
	scenelink.draw(cb);
}



class SceneRenderMethod
{
public:
	virtual ~SceneRenderMethod() {}
	virtual void render_view(gemini::IEngineEntity** entity_list, platform::NativeWindow* window, const View& view) = 0;
	virtual void render_viewmodel(gemini::IEngineEntity* entity, platform::NativeWindow* window, const glm::vec3& origin, const glm::vec2& view_angles) = 0;
	
	virtual void render_gui() = 0;
};


class VRRenderMethod : public SceneRenderMethod
{
	vr::HeadMountedDevice* device;
	
	SceneLink& scenelink;
	
public:
	VRRenderMethod(vr::HeadMountedDevice* in_device, SceneLink& in_link) : device(in_device), scenelink(in_link) {}
	
	virtual void render_view(gemini::IEngineEntity** entity_list, platform::NativeWindow* window, const View& view)
	{
#if 0
		assert( device != nullptr );
		
		renderer::IRenderDriver * driver = renderer::driver();
		device->begin_frame(driver);
		
		renderer::RenderTarget* rt = device->render_target();
		
		RenderStream rs;
		rs.add_clearcolor(0.0f, 0.5f, 0.5f, 1.0f);
		rs.add_clear(renderer::CLEAR_COLOR_BUFFER | renderer::CLEAR_DEPTH_BUFFER);
		rs.add_state(renderer::STATE_DEPTH_TEST, 1);
		rs.run_commands();
		rs.rewind();
//		
////		Camera camera;
////		camera.type = Camera::FIRST_PERSON;
////		camera.perspective(50.0f, window->render_width, window->render_height, 0.01f, 8192.0f);
////		camera.set_absolute_position(origin);
////		camera.eye_position = origin;
////		camera.view = glm::vec3(0, 0, -1);
////		camera.pitch = view_angles.x;
////		camera.yaw = view_angles.y;
////		camera.update_view();
		
		
		glm::vec3 old_camera_position = camera.pos;
		glm::vec3 camera_position = camera.pos + glm::vec3(0, 1.6f, 0.0f);
		vr::EyePose eye_poses[2];
		glm::mat4 proj[2];
		device->get_eye_poses(eye_poses, proj);
		
		
		
		for (uint32_t eye_index = 0; eye_index < 2; ++eye_index)
		{
			glm::mat4 old_matcam = camera.matCam;
			vr::EyePose& eye_pose = eye_poses[eye_index];
			
			int x;
			int y = 0;
			int width, height;
			
			width = rt->width/2;
			height = rt->height;
			
			if (eye_pose.is_left_eye())
			{
				x = 0;
				y = 0;
			}
			else if (eye_pose.is_right_eye())
			{
				x = width;
				y = 0;
			}
			
			// setup camera matrix for this eye
			
			// rotation = character rotation * eye pose rotation
			// translation = characer position + character rotation.rotate_vector(eye pose translation) // q(t) * V * q(t)^-1
			
			
			glm::quat character_rotation;
			glm::quat rotation = eye_pose.rotation;
			glm::vec3 translation = eye_pose.offset + eye_pose.translation;
			
			glm::mat4 tr = glm::translate(glm::mat4(1.0f), translation);
			glm::mat4 ro = glm::toMat4(rotation);
			glm::mat4 final_rift = glm::inverse(tr * ro);
			// TODO: the camera needs work because we need the base character rotation
			// then we need to add the rift rotation
			// then need to translate the character
			// and finally add the rift translation to that.
			
			//			camera.matCam = final_rift; //camera.get_inverse_rotation() * glm::translate(glm::mat4(1.0), -camera.pos);
			
			
			glm::mat4 character_transform = glm::translate(glm::mat4(1.0), camera_position) * glm::transpose(camera.get_inverse_rotation());
			glm::mat4 head_transform = tr * ro;
			
			camera.matCam = glm::inverse(character_transform * head_transform);
			
			
			camera.matProj = proj[eye_index];
			//camera.pos = translation;
			
			rs.add_viewport(x, y, width, height);
			
			// render nodes
			rs.run_commands();
			rs.rewind();
			
			render_scene_from_camera(entity_list, camera, scenelink);
			
			// draw debug graphics
			debugdraw::render(camera.matCam, camera.matProj, x, y, width, height);
			
			camera.matCam = old_matcam;
			
			// TODO: this HAS to be drawn when the render target is active for the
			// rift.
			if (_compositor)
			{
				_compositor->render();
			}
		}
		
		camera.pos = old_camera_position;
		
		device->end_frame(driver);
#endif
	}
	
	virtual void render_viewmodel(gemini::IEngineEntity* entity, platform::NativeWindow* window, const glm::vec3& origin, const glm::vec2& view_angles)
	{
		
	}
	
	virtual void render_gui()
	{
		if (_compositor)
		{
			_compositor->render();
		}
	}
};


class DefaultRenderMethod : public SceneRenderMethod
{
	SceneLink& scenelink;
	platform::IWindowLibrary* window_library;
	
public:
	DefaultRenderMethod(SceneLink& in_link, platform::IWindowLibrary* wl) :
		scenelink(in_link),
		window_library(wl)
	{
	}
	
	
	virtual void render_view(gemini::IEngineEntity** entity_list, platform::NativeWindow* window, const View& view)
	{
		::renderer::RenderStream rs;
		rs.add_cullmode(::renderer::CullMode::CULLMODE_BACK);
		rs.add_state(::renderer::STATE_DEPTH_WRITE, 1);
		//		rs.add_state(renderer::STATE_BACKFACE_CULLING, 0);
		rs.add_state(::renderer::STATE_DEPTH_TEST, 1);
		rs.add_viewport( 0, 0, window->render_width, window->render_height );
		rs.add_clearcolor( 0.0, 0.0, 0.0, 1.0f );
		rs.add_clear(::renderer::CLEAR_COLOR_BUFFER | ::renderer::CLEAR_DEPTH_BUFFER );
		rs.run_commands();

		View newview = view;
		newview.width = window->render_width;
		newview.height = window->render_height;

		render_scene_from_camera(entity_list, newview, scenelink);
		
		debugdraw::render(view.modelview, view.projection, 0, 0, view.width, view.height);
	}
	
	virtual void render_viewmodel(gemini::IEngineEntity* entity, platform::NativeWindow* window, const glm::vec3& origin, const glm::vec2& view_angles)
	{
		::renderer::RenderStream rs;
		rs.add_cullmode(::renderer::CullMode::CULLMODE_BACK);
		rs.add_state(::renderer::STATE_BACKFACE_CULLING, 1);
		rs.add_state(::renderer::STATE_DEPTH_TEST, 1);
		rs.add_clear(::renderer::CLEAR_DEPTH_BUFFER );
		rs.run_commands();
		
//		Camera camera;
//		camera.type = Camera::FIRST_PERSON;
//		
//		camera.perspective(35.0f, window->render_width, window->render_height, 0.01f, 32.0f);
//		
//		camera.set_absolute_position(glm::vec3(-1.0f, 0.6f, 2.5f));
//		camera.eye_position = origin;
//		camera.view = glm::vec3(0, 0, -1);
//		camera.pitch = 0;
//		camera.yaw = 0;
//		camera.update_view();
//		
//		render_entity_from_camera(entity, camera, scenelink);
	}
	
	virtual void render_gui()
	{
		if (_compositor)
		{
			_compositor->render();
		}
	}
};

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
		unsigned int mesh_asset_index;
		assets::Mesh* mesh;
		glm::mat4 transform;
		
		glm::mat4* bone_transforms;
		glm::mat4 debug_bone_transforms[MAX_BONES];

		Channel<glm::vec3> scale_channel;
		Channel<glm::quat> rotation_channel;
		Channel<glm::vec3> translation_channel;
		
		glm::vec3 scale;
		glm::quat rotation;
		glm::vec3 translation;
		
		std::vector<animation::SequenceId> animations;
		
	public:
		
		ModelInstanceData() :
		mesh_asset_index(0),
		mesh(0),
		bone_transforms(0),
		scale_channel(scale),
		rotation_channel(rotation),
		translation_channel(translation)
		{
		}
		
		void set_mesh_index(unsigned int mesh_asset_id)
		{
			mesh_asset_index = mesh_asset_id;
			mesh = assets::meshes()->find_with_id(mesh_asset_index);
		}
		
		void create_bones()
		{
			assert(mesh != 0);
			
			// does this have an animation?
			if (mesh->has_skeletal_animation)
			{
				size_t total_elements = (mesh->geometry.size() * mesh->skeleton.size());
				bone_transforms = new glm::mat4[total_elements];
			}
		}
		
		void destroy_bones()
		{
			if (bone_transforms)
			{
				delete [] bone_transforms;
				bone_transforms = 0;
			}
		}
		
		virtual unsigned int asset_index() const { return mesh_asset_index; }
		virtual glm::mat4& get_local_transform() { return transform; }
		virtual void set_local_transform(const glm::mat4& _transform) { transform = _transform; }
		virtual glm::mat4* get_bone_transforms(uint32_t geometry_index) const
		{
			if (!bone_transforms)
			{
				return nullptr;
			}
			return &bone_transforms[mesh->skeleton.size()*geometry_index];
		}
		
		virtual glm::mat4* get_debug_bone_transforms()
		{
			return debug_bone_transforms;
		}
		
		virtual uint32_t get_total_transforms() const
		{
			return mesh->skeleton.size();
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

			size_t bone_index = 0;
			for (FixedArray<animation::Channel>& node : instance->ChannelSet)
			{
				assert(bone_index < MAX_BONES);
				
				glm::vec3& pos = positions[bone_index];
				glm::quat& rot = rotations[bone_index];
				const animation::Channel& tx = node[0];
				const animation::Channel& ty = node[1];
				const animation::Channel& tz = node[2];
				pos = glm::vec3(tx(), ty(), tz());
				
				if (node.size() > 3)
				{
					const animation::Channel& rx = node[3];
					const animation::Channel& ry = node[4];
					const animation::Channel& rz = node[5];
					const animation::Channel& rw = node[6];
					
					rot = glm::quat(rw(), rx(), ry(), rz());
				}

				
				++bone_index;
			}
		}
		
		virtual void set_pose(glm::vec3* positions, glm::quat* rotations)
		{
			if (mesh->skeleton.empty())
			{
				return;
			}
			
			// You've hit the upper bounds for skeletal bones for a single
			// model. Congrats.
			assert(mesh->skeleton.size() < MAX_BONES);
			
			const glm::mat4& tx = this->get_local_transform();

			size_t geometry_index = 0;
			// we must update the transforms for each geometry instance
			for (const assets::Geometry& geo : mesh->geometry)
			{
				size_t transform_index;
				// recalculate
				
				glm::mat4 local_transforms[MAX_BONES];
				
				for (size_t index = 0; index < mesh->skeleton.size(); ++index)
				{
					transform_index = (geometry_index * mesh->skeleton.size()) + index;
					assets::Joint* joint = &mesh->skeleton[index];
					glm::mat4& global_pose = bone_transforms[transform_index];
					glm::mat4& saved_pose = local_transforms[index];
					glm::mat4& debug_bone_transform = debug_bone_transforms[index];
					
					glm::mat4 local_scale;
					glm::mat4 local_rotation = glm::toMat4(rotations[index]);
					glm::mat4 local_transform = glm::translate(glm::mat4(1.0f), positions[index]);
					glm::mat4 local_pose = local_transform * local_rotation * local_scale;
					//				local_to_world = tr * pivot * ro * sc * inv_pivot;
					
					if (joint->parent_index > -1)
					{
						const glm::mat4& parent_transform = local_transforms[joint->parent_index];
						saved_pose = parent_transform * local_pose;
					}
					else
					{
						saved_pose = local_pose;
					}
					
					// this will be used for skinning in the vertex shader
					global_pose = saved_pose * geo.bind_poses[index];
					
					// this will be used for debug rendering
					debug_bone_transform = tx * (saved_pose);
				}
				
				++geometry_index;
			}
		}
		
		virtual int32_t get_animation_index(const char* name)
		{
			size_t index = 0;
			for (auto& id : animations)
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
			animation::SequenceId id = animation::load_sequence(name, mesh);
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
		
		virtual uint32_t get_total_bones(int32_t index) const
		{
			animation::SequenceId instance_index = animations[index];
			animation::AnimatedInstance* instance = animation::get_instance_by_index(instance_index);

			return instance->ChannelSet.size();
		}
	};
	
	
	typedef std::map<int32_t, ModelInstanceData> ModelInstanceMap;
	ModelInstanceMap id_to_instance;
	
	
public:
	virtual int32_t create_instance_data(const char* model_path)
	{
		assets::Mesh* mesh = assets::meshes()->load_from_path(model_path);
		if (mesh)
		{
			if (mesh->is_dirty)
			{
				mesh->prepare_geometry();
			}
			ModelInstanceData data;
			data.set_mesh_index(mesh->Id());
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
//		command.left += input::state()->keyboard().is_down(input::KEY_A) * input::AxisValueMaximum;
//		command.right += input::state()->keyboard().is_down(input::KEY_D) * input::AxisValueMaximum;
//		command.forward += input::state()->keyboard().is_down(input::KEY_W) * input::AxisValueMaximum;
//		command.back += input::state()->keyboard().is_down(input::KEY_S) * input::AxisValueMaximum;
		
		return root;
	}
	
	virtual gui::Compositor* get_compositor() const
	{
		return compositor;
	}
};

void center_mouse(platform::IWindowLibrary* window_interface, platform::NativeWindow* window)
{
	window_interface->warp_mouse(window->window_width/2, window->window_height/2);
}

struct MemoryTagGame {};


struct SharedState
{
	bool has_focus;
};


static SharedState _sharedstate;

class EngineInterface : public IEngineInterface
{
	IEntityManager* entity_manager;
	IModelInterface* model_interface;
	gemini::physics::IPhysicsInterface* physics_interface;
	IExperimental* experimental_interface;
	
	SceneRenderMethod* render_method;
	
	platform::NativeWindow* main_window;
	platform::IWindowLibrary* window_interface;
	
	platform::memory::GlobalDebugAllocator<MemoryTagGame> game_allocator;
public:
	
	EngineInterface(
					IEntityManager* em,
					IModelInterface* mi,
					gemini::physics::IPhysicsInterface* pi,
					IExperimental* ei,
					SceneRenderMethod* rm,
					platform::NativeWindow* window,
					platform::IWindowLibrary* window_interface) :
	entity_manager(em),
	model_interface(mi),
	physics_interface(pi),
	experimental_interface(ei),
	render_method(rm),
	main_window(window),
	window_interface(window_interface)
	{
	}
	
	
	virtual ~EngineInterface() {};
	
	virtual IEntityManager* entities() { return entity_manager; }
	virtual IModelInterface* models() { return model_interface; }
	virtual gemini::physics::IPhysicsInterface* physics() { return physics_interface; }
	virtual IExperimental* experiment() { return experimental_interface; }
	virtual core::logging::ILog* log() { return core::log::instance(); }
	virtual gemini::IDebugDraw* debugdraw() { return debugdraw::instance(); }
	virtual gemini::IAudioInterface* audio() { return gemini::audio::instance(); }
	
	virtual void* allocate(size_t size)
	{
		return game_allocator.allocate(size, __FILE__, __LINE__);
	}
	
	virtual void deallocate(void* pointer)
	{
		game_allocator.deallocate(pointer);
	}
	
	virtual void render_view(const View& view)
	{
		// TODO: need to validate this origin/orientation is allowed.
		// otherwise, client could ask us to render from anyone's POV.
		EntityManager* em = static_cast<EntityManager*>(engine::api::instance()->entities());
		render_method->render_view(em->get_entity_list(), main_window, view);
	}
	
	virtual void render_gui()
	{
		render_method->render_gui();
	}
	
	virtual platform::memory::GlobalAllocator* allocator()
	{
		return &platform::memory::global_allocator();
	}
	
	virtual void render_viewmodel(IEngineEntity* entity, const glm::vec3& origin, const glm::vec2& view_angles)
	{
		render_method->render_viewmodel(entity, main_window, origin, view_angles);
	}

	virtual void get_render_resolution(uint32_t& render_width, uint32_t& render_height)
	{
		assert(main_window);
		render_width = main_window->render_width;
		render_height = main_window->render_height;
	}
	
	virtual void center_cursor()
	{
		if (_sharedstate.has_focus)
		{
			center_mouse(window_interface, main_window);
		}
	}
	
	virtual void show_cursor(bool show)
	{
		window_interface->show_mouse(show);
	}
	
	virtual void terminate_application()
	{
		kernel::instance()->set_active(false);
	}
};



typedef gemini::IGameInterface* (*connect_engine_fn)(gemini::IEngineInterface*);
typedef void (*disconnect_engine_fn)();

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
	
	platform::IWindowLibrary* window_interface;
	platform::NativeWindow* main_window;
	platform::NativeWindow* alt_window;
	
	::renderer::Texture* gui_texture;
	::renderer::RenderTarget* gui_render_target;
		
	// Kernel State variables
	double accumulator;
	uint64_t last_time;
	
	EventQueueType* event_queue;
	
	// rendering
	SceneLink* scenelink;
	SceneRenderMethod* render_method;
	
	// game library
	platform::DynamicLibrary* gamelib;
	disconnect_engine_fn disconnect_engine;
	EntityManager entity_manager;
	ModelInterface model_interface;
	Experimental experimental;
	IEngineInterface* engine_interface;
	IGameInterface* game_interface;
	
	// VR stuff
	vr::HeadMountedDevice* device;
	
	// GUI stuff
	GUIRenderer* gui_renderer;
	GUIStyle* gui_style;
	gui::Compositor* compositor;
	gui::Graph* graph;
	gui::Panel* root;
	gui::Button* newgame;
	gui::Button* test;
	gui::Button* quit;
	
	::renderer::VertexStream alt_vs;

	DataInput data_input;

private:
	bool load_config(Settings& config)
	{
		bool success = util::json_load_with_callback( "conf/settings.conf", settings_conf_loader, &config, true );
		if ( !success )
		{
			LOGV("Unable to load settings.conf! Let's hope wise defaults were chosen...\n");
		}
		
		return success;
	} // load_config
	
	
	void open_gamelibrary()
	{
		// load game library
		StackString<MAX_PATH_SIZE> game_library_path = ::core::filesystem::content_directory();
		const char* dynamiclibrary_extension = platform::dylib_extension();
		game_library_path.append(PATH_SEPARATOR_STRING).append("bin").append(PATH_SEPARATOR_STRING).append("game").append(dynamiclibrary_extension);
		gamelib = platform::dylib_open(game_library_path());
		if (!gamelib)
		{
			LOGV("unable to open game: \"%s\"\n", game_library_path());
			assert(0);
		}
		else
		{
			LOGV("opened game library!\n");
			
			// link the engine interface
			
			connect_engine_fn connect_engine = (connect_engine_fn)platform::dylib_find(gamelib, "connect_engine");
			disconnect_engine = (disconnect_engine_fn)platform::dylib_find(gamelib, "disconnect_engine");
			if (connect_engine)
			{
				game_interface = connect_engine(gemini::engine::api::instance());
			}
			if (!game_interface)
			{
				LOGE("Unable to connect engine to game library\n");
				assert(game_interface != 0);
			}
			
			game_interface->startup();
		}
	}
	
	void close_gamelibrary()
	{
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
	}
	

	
public:
	EngineKernel() :
		active(true),
		accumulator(0.0f),
		last_time(0),
		engine_interface(0),
		experimental(0),
		game_interface(0),
		render_method(0),
		draw_physics_debug(false),
		draw_navigation_debug(true)
	{
		game_path = "";
		device = 0;
	}
	
	virtual ~EngineKernel()
	{
		
	}
	
	virtual bool is_active() const { return active; }
	virtual void set_active(bool isactive) { active = isactive; }
	
	virtual void resolution_changed(int width, int height) {}
	
	
	
	
	
	
	virtual void event( kernel::KeyboardEvent & event )
	{
//		input::state()->keyboard().inject_key_event(event.key, event.is_down);
		
		GameMessage game_message;
		game_message.type = GameMessage::KeyboardEvent;
		game_message.button = event.key;
		game_message.params[0] = event.is_down;
		event_queue->push_back(game_message);
	
	
		if (event.is_down)
		{
			if (event.key == input::KEY_SPACE)
			{
				if (device)
				{
					device->dismiss_warning();
				}
			}
			else if (event.key == input::KEY_TAB)
			{
				if (device)
				{
					device->reset_head_pose();
				}
			}
			else if (event.key == input::KEY_P)
			{
				draw_physics_debug = !draw_physics_debug;
				LOGV("draw_physics_debug = %s\n", draw_physics_debug?"ON":"OFF");
			}
			else if (event.key == input::KEY_N)
			{
				draw_navigation_debug = !draw_navigation_debug;
				LOGV("draw_navigation_debug = %s\n", draw_navigation_debug?"ON":"OFF");
			}
			else if (event.key == input::KEY_M)
			{
				LOGV("level load\n");
				game_interface->level_load();
			}
		}
	}
	
	virtual void event(kernel::MouseEvent& event)
	{
		GameMessage game_message;
		switch (event.subtype)
		{
			case kernel::MouseButton:
				game_message.type = GameMessage::MouseEvent;
				game_message.button = event.button;
				game_message.params[0] = event.is_down;
				break;
				
			case kernel::MouseMoved:
				game_message.type = GameMessage::MouseMove;
				game_message.params[0] = event.mx;
				game_message.params[1] = event.my;
				break;
				
			case kernel::MouseWheelMoved:
				game_message.type = GameMessage::MouseMove;
				game_message.params[0] = event.wheel_direction;
				break;
				
			default: break;
		}
		
		event_queue->push_back(game_message);
	}
	
	virtual void event( kernel::SystemEvent & event )
	{
		if (event.subtype == kernel::WindowLostFocus)
		{
			_sharedstate.has_focus = false;
		}
		else if (event.subtype == kernel::WindowGainFocus)
		{
			_sharedstate.has_focus = true;
		}
	}
	
	virtual void event(kernel::GameControllerEvent& event)
	{
		if (event.subtype == kernel::JoystickConnected)
		{
			LOGV("gamepad [%i] connected\n", event.gamepad_id);
		}
		else if (event.subtype == kernel::JoystickDisconnected)
		{
			LOGV("gamepad [%i] disconnected\n", event.gamepad_id);
		}
		else if (event.subtype == kernel::JoystickButton)
		{
			LOGV("gamepad [%i] button: %i, is_down: %i\n", event.gamepad_id, event.button, event.is_down);
		}
		else if (event.subtype == kernel::JoystickAxisMoved)
		{
			LOGV("gamepad [%i] joystick: %i, value: %i (%2.2f)\n", event.gamepad_id, event.joystick_id, event.joystick_value, event.normalized_value());
		}
		else
		{
			LOGV("gamepad [%i] controller event received: %i\n", event.gamepad_id, event.subtype);
		}
	}
	
	void setup_gui(uint32_t width, uint32_t height)
	{
		compositor = new gui::Compositor(width, height);
		_compositor = compositor;
		
		gui_renderer = MEMORY_NEW(GUIRenderer, platform::memory::global_allocator());
		compositor->set_renderer(gui_renderer);
		
		gui_style = MEMORY_NEW(GUIStyle, platform::memory::global_allocator());
		compositor->set_style(gui_style);
		
		root = new gui::Panel(compositor);
		
		experimental.set_root(root);
		experimental.set_compositor(compositor);
		
		root->set_bounds(0, 0, main_window->render_width, main_window->render_height);
		root->set_background_color(gui::Color(0, 0, 0, 0));
		
		// setup the framerate graph
		graph = new gui::Graph(root);
		graph->set_bounds(width-250, 0, 250, 100);
		graph->set_font(compositor, "fonts/debug");
		graph->set_background_color(gui::Color(10, 10, 10, 210));
		graph->set_foreground_color(gui::Color(255, 255, 255, 255));
		graph->create_samples(100, 1);
		graph->configure_channel(0, gui::Color(255, 0, 0, 255));
		graph->set_range(0.0f, 33.3f);

		graph->enable_baseline(true, 16.6f, gui::Color(255, 0, 255, 255));
	}
	
	virtual kernel::Error startup()
	{
		event_queue = new FixedSizeQueue<GameMessage, 64>;
	
		// parse command line values
		std::vector<std::string> arguments;
		core::argparse::ArgumentParser parser;
		const platform::MainParameters& mainparams = platform::get_mainparameters();
#if defined(PLATFORM_LINUX) || defined(PLATFORM_APPLE)
		arguments = parser.split_tokens(mainparams.argc, mainparams.argv);
#elif defined(PLATFORM_WINDOWS)
		arguments = parser.split_tokens(mainparams.commandline);
#else
	#error Not implemented on this platform!
#endif

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
			game_path = core::str::make_absolute_path(path).c_str();
		}

		const char FONT_SHADER[] = "shaders/fontshader";
		const char DEBUG_FONT[] = "fonts/debug";
		const char DEBUG_SHADER[] = "shaders/debug";
		kernel::Parameters& params = kernel::parameters();
	
		// initialize timer
		last_time = platform::microseconds();

#if defined(PLATFORM_MOBILE)
#else
		kernel::parameters().device_flags |= kernel::DeviceDesktop;
#endif

		//
		// setup our file system...
		StackString< MAX_PATH_SIZE > root_path;
		platform::Result result = platform::get_program_directory(&root_path[0], root_path.max_size());
		assert(!result.failed());
		
		// set the startup directory: where the binary lives
		core::filesystem::root_directory(&root_path[0], root_path.max_size());
		
		
		// if no game is specified on the command line, construct the content path
		// from the current root directory
		StackString<MAX_PATH_SIZE> content_path;
		if (game_path.is_empty())
		{
			core::filesystem::construct_content_directory(content_path);
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
		
		// set the content path
		core::filesystem::content_directory(content_path());
		
		// startup duties; lower-level system init
		result = core::startup();
		if (result.failed())
		{
			fprintf(stderr, "Fatal error: %s\n", result.message);
			core::shutdown();
			return kernel::CoreFailed;
		}
		
		// create the window interface
		window_interface = platform::create_window_library();
		window_interface->startup(kernel::parameters());
			
		// load engine settings
		// load boot config
		Settings config;
		load_config(config);
		
		params.step_interval_seconds = (1.0f/(float)config.physics_tick_rate);
				
		
		const char* serial_device = "/dev/cu.usbmodem582211"; // config["input_serial_device"];
		data_input.device = platform::serial_open(serial_device, 1000000);
		if (!data_input.device)
		{
			LOGW("Unable to open serial device at '%s'\n", serial_device);
		}
		
		data_input.execute = true;
		data_input.event_queue = event_queue;
		
		
		if (data_input.device)
		{
			// fire up a thread
			platform::thread_create(data_input.thread_data, data_thread, &data_input);
		}
		
		
		platform::WindowParameters window_params;
		
		// TODO: we should load these from a config; for now just set them.
		window_params.window_width = config.window_width;
		window_params.window_height = config.window_height;
		window_params.window_title = "gemini";
		
		// needs to happen here if we want to rely on vr::total_devices
		vr::startup();
		
		// TODO: do old application config; setup VR headset
		if ((config.vr_require_headset && (vr::total_devices() > 0)) || (!config.vr_require_headset))
		{
			device = vr::create_device();
			if (device)
			{
				int32_t width, height;
				device->query_display_resolution(width, height);
				
				// required such that the kernel doesn't swap buffers
				// as this is handled by the rift sdk.
				params.swap_buffers = !config.vr_render;
				
				if (config.vr_render)
				{
//					params.use_fullscreen = 1;
					
					
#if PLATFORM_MACOSX
					// target the rift.
					if (config.vr_require_headset)
					{
						window_params.target_display = 2;
					}
#endif
					params.use_vsync = true;
					window_params.window_width = (uint16_t)width;
					window_params.window_height = (uint16_t)height;
				}
			}
		}
		
		// create the window
		
		main_window = window_interface->create_window(window_params);
		
		alt_window = 0;

		// uncomment this block to enable the alt window for gui diplay
//		window_params = platform::WindowParameters();
//		window_params.window_width = 800;
//		window_params.window_height = 600;
//		window_params.window_title = "Test Window";
//		window_params.target_display = 1;
//		alt_window = window_interface->create_window(window_params);

		window_interface->activate_window(main_window);
		window_interface->focus_window(main_window);
		

		// initialize rendering subsystems
		{
			scenelink = MEMORY_NEW(SceneLink, platform::memory::global_allocator());
			int render_result =	::renderer::startup(::renderer::Default, config.render_settings);
			if ( render_result == 0 )
			{
				LOGE("renderer initialization failed!\n");
				return kernel::RendererFailed;
			}
			
			assets::startup();
			
			assets::Shader* fontshader = assets::shaders()->load_from_path(FONT_SHADER);
			assert(fontshader != 0);
			font::startup(fontshader->program, main_window->render_width, main_window->render_height);
			
			assets::Shader* debugshader = assets::shaders()->load_from_path(DEBUG_SHADER);
			assets::Font* debugfont = assets::fonts()->load_from_path(DEBUG_FONT);
			assert(debugshader != 0);
			assert(debugfont != 0);
			debugdraw::startup(config.debugdraw_max_primitives, debugshader->program, debugfont->handle);
		}
		
//		renderer::IRenderDriver* driver = renderer::driver();
//		driver->render
		
		struct TempVertex
		{
			glm::vec2 pos;
			Color color;
			glm::vec2 uv;
		};
		
		if (alt_window)
		{
			window_interface->activate_window(alt_window);
			window_interface->focus_window(alt_window);
			
			alt_vs.desc.add(::renderer::VD_FLOAT2);
			alt_vs.desc.add(::renderer::VD_UNSIGNED_BYTE4);
			alt_vs.desc.add(::renderer::VD_FLOAT2);

			
			alt_vs.create(6, 10, ::renderer::DRAW_INDEXED_TRIANGLES);
			
			if (alt_vs.has_room(4, 6))
			{
				TempVertex* v = (TempVertex*)alt_vs.request(4);
				float cx = (alt_window->render_width/2.0f);
				float cy = (alt_window->render_height/2.0f);
				
				const float RECT_SIZE = 150.0f;
				
				
				// this is intentionally inverted along the y
				// so that texture renderered appears correctly.
				v[0].pos = glm::vec2(cx-RECT_SIZE, cy-RECT_SIZE); v[0].uv = glm::vec2(0,0); v[0].color = Color(255, 255, 255, 255);
				v[1].pos = glm::vec2(cx-RECT_SIZE, cy+RECT_SIZE); v[1].uv = glm::vec2(0,1); v[1].color = Color(255, 255, 255, 255);
				v[2].pos = glm::vec2(cx+RECT_SIZE, cy+RECT_SIZE); v[2].uv = glm::vec2(1,1); v[2].color = Color(255, 255, 255, 255);
				v[3].pos = glm::vec2(cx+RECT_SIZE, cy-RECT_SIZE); v[3].uv = glm::vec2(1,0); v[3].color = Color(255, 255, 255, 255);
				
				::renderer::IndexType indices[] = {0, 1, 2, 2, 3, 0};
				alt_vs.append_indices(indices, 6);
				alt_vs.update();
			}
		}
		
		window_interface->activate_window(main_window);
		window_interface->focus_window(main_window);
		
		
		// create the render target and texture for the gui
		image::Image image;
		image.width = 512;
		image.height = 512;
		image.channels = 3;
		gui_texture = ::renderer::driver()->texture_create(image);
		
		gui_render_target = ::renderer::driver()->render_target_create(image.width, image.height);
		::renderer::driver()->render_target_set_attachment(gui_render_target, ::renderer::RenderTarget::COLOR, 0, gui_texture);
		::renderer::driver()->render_target_set_attachment(gui_render_target, ::renderer::RenderTarget::DEPTHSTENCIL, 0, 0);
		
		
		
		// initialize main subsystems
		audio::startup();
		input::startup();
		gemini::physics::startup();
		animation::startup();

		if (config.enable_asset_reloading)
		{
			hotloading::startup();
		}
		
		// TODO: call the old Application startup function here.
		if (device && config.vr_render)
		{
			vr::setup_rendering(device, main_window->render_width, main_window->render_height);
			// TODO: instance render method for VR
			render_method = MEMORY_NEW(VRRenderMethod, platform::memory::global_allocator()) (device, *scenelink);
		}
		else
		{
			// TODO: instance render method for default
			render_method = MEMORY_NEW(DefaultRenderMethod, platform::memory::global_allocator()) (*scenelink, window_interface);
		}
		
		// setup interfaces
		engine_interface = MEMORY_NEW(EngineInterface, platform::memory::global_allocator())
			(&entity_manager,
			&model_interface,
			physics::api::instance(),
			&experimental,
			render_method,
			main_window,
			window_interface
		);
		gemini::engine::api::set_instance(engine_interface);
		window_interface->show_mouse(true);
		
		setup_gui(main_window->render_width, main_window->render_height);
		
		open_gamelibrary();

		// for debugging
		game_interface->level_load();

		
		navigation::startup();



		assets::Mesh* mesh = assets::meshes()->load_from_path("models/plane2");
		assets::Geometry* geom = &mesh->geometry[0];
		navigation::create_from_geometry(geom->vertices, geom->indices, geom->mins, geom->maxs);
		
		// TODO: post_application_startup
		
				
		uint64_t current_time = platform::microseconds();
		LOGV("startup in %2.2fms\n", (current_time-last_time)*.001f);
		last_time = current_time;
		
		return kernel::NoError;
	}
	
	
	
	void update()
	{
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
			// begin step
			
			// this is going to be incorrect unless this is placed in the step.
			// additionally, these aren't interpolated: figure how to; for example,
			// draw hit boxes for a moving player with this system.
			debugdraw::update(params.step_interval_seconds);
			
			// end step
			
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
		// 1. handle input; generate events?
		// 2. on frame duties:
		//	- pump event
		
		
		update();
		
		window_interface->process_events();
		input::update();
	
		audio::update();
		animation::update(kernel::parameters().framedelta_filtered_seconds);
		hotloading::tick();
		post_tick();
		kernel::parameters().current_frame++;
	}
	
	void post_tick()
	{
		window_interface->activate_window(main_window);
		
		if (graph)
		{
			graph->record_value(kernel::parameters().framedelta_raw_msec, 0);
		}
		
		if (compositor)
		{
			compositor->update(kernel::parameters().framedelta_raw_msec);
		}


		int mouse[2];
		window_interface->get_mouse(mouse[0], mouse[1]);
		int half_width = main_window->window_width/2;
		int half_height = main_window->window_height/2;
		
		// capture the state of the mouse
		int mdx, mdy;
		mdx = (mouse[0] - half_width);
		mdy = (mouse[1] - half_height);
		if (mdx != 0 || mdy != 0)
		{
			GameMessage game_message;
			game_message.type = GameMessage::MouseDelta;
			game_message.params[0] = mdx;
			game_message.params[1] = mdy;
			event_queue->push_back(game_message);
		}
			
#if 0
		// add the inputs and then normalize
		input::JoystickInput& joystick = input::state()->joystick(0);
		if (joystick.axes[0].value < 0)
		{
			command.left += (joystick.axes[0].value/(float)input::AxisValueMinimum) * input::AxisValueMaximum;
		}
		if (joystick.axes[0].value > 0)
		{
			command.right += (joystick.axes[0].value/(float)input::AxisValueMaximum) * input::AxisValueMaximum;
		}
		
		if (joystick.axes[1].value < 0)
		{
			command.forward += (joystick.axes[1].value/(float)input::AxisValueMinimum) * input::AxisValueMaximum;
		}
		if (joystick.axes[1].value > 0)
		{
			command.back += (joystick.axes[1].value/(float)input::AxisValueMaximum) * input::AxisValueMaximum;
		}
#endif

		
		while(!event_queue->empty())
		{
			GameMessage game_message = event_queue->pop();
			game_interface->server_process_message(game_message);
		}


		int x = 250;
		int y = 0;
		{

//			debugdraw::text(x, y, core::str::format("active_camera->pos = %.2g %.2g %.2g", main_camera.pos.x, main_camera.pos.y, main_camera.pos.z), Color(255, 255, 255));
//			debugdraw::text(x, y+12, core::str::format("eye_position = %.2g %.2g %.2g", main_camera.eye_position.x, main_camera.eye_position.y, main_camera.eye_position.z), Color(255, 0, 255));
//			debugdraw::text(x, y+24, core::str::format("active_camera->view = %.2g %.2g %.2g", main_camera.view.x, main_camera.view.y, main_camera.view.z), Color(128, 128, 255));
//			debugdraw::text(x, y+36, core::str::format("active_camera->right = %.2g %.2g %.2g", main_camera.side.x, main_camera.side.y, main_camera.side.z), Color(255, 0, 0));
		}
		debugdraw::text(x, y, core::str::format("frame delta = %2.2fms\n", kernel::parameters().framedelta_raw_msec), Color(255, 255, 255));
		y += 12;
		debugdraw::text(x, y, core::str::format("# allocations = %i, total %2.2f MB\n",
			platform::memory::MemoryCategoryTracking<platform::memory::MemTagGlobal>::active_allocations,
			platform::memory::MemoryCategoryTracking<platform::memory::MemTagGlobal>::active_bytes/(float)(1024*1024)), Color(64, 102, 192));
//		y += 12;
		
		
		if (draw_physics_debug)
		{
			physics::debug_draw();
		}
		
		if (draw_navigation_debug)
		{
			navigation::debugdraw();
		}
	
	
		if (game_interface)
		{
			// 1. game_interface->run_frame(framedelta_seconds);
			// 2. game_interface->draw_frame();
			
			float framedelta_seconds = kernel::parameters().framedelta_raw_msec*0.001f;
			
			game_interface->server_frame(
				kernel::parameters().current_tick,
				framedelta_seconds,
				kernel::parameters().step_interval_seconds,
				kernel::parameters().step_alpha
			);
			
			game_interface->client_frame(framedelta_seconds, kernel::parameters().step_alpha);
		}
		
//		if (device)
//		{
//			glm::mat4 xform;
//			device->test(xform);
//			debugdraw::axes(xform, 2.0f, 0.1f);
//		}


	
		// TODO: this needs to be controlled somehow
		// as the rift sdk performs buffer swaps during end frame.
		if (kernel::parameters().swap_buffers)
		{
			window_interface->swap_buffers(main_window);


		}
		
		// do drawing for the alt window
		if (alt_window)
		{
			window_interface->activate_window(alt_window);
			::renderer::CommandBuffer buffer;

#if 0
			render::Device* device = render::create_device();
//			renderer::CommandBuffer buffer;
//			buffer.viewport(0, 0, alt_window->render_width, alt_window->render_height);
//			buffer.clear_color(Color(255, 255, 255, 255));
//
//			device->execute(buffer);
#endif


			::renderer::IRenderDriver* device = ::renderer::driver();
			
			{
				
				::renderer::RenderStream rs;
				rs.add_viewport(0, 0, gui_texture->width, gui_texture->height);
				rs.add_clearcolor(0.0f, 0.0f, 0.0f, 1.0f);
				rs.add_clear(::renderer::CLEAR_COLOR_BUFFER | ::renderer::CLEAR_DEPTH_BUFFER);

				
				device->render_target_activate(gui_render_target);
				
				rs.run_commands();
				
				
				_compositor->render();
				
				
				device->render_target_deactivate(gui_render_target);
			}





			::renderer::RenderStream rs;
			rs.add_viewport(0, 0, alt_window->render_width, alt_window->render_height);
			rs.add_clearcolor(0.1f, 0.1f, 0.1f, 1.0f);
			rs.add_clear(::renderer::CLEAR_COLOR_BUFFER | ::renderer::CLEAR_DEPTH_BUFFER);
			
			
			glm::mat4 modelview;
			glm::mat4 projection = glm::ortho(0.0f, (float)alt_window->render_width, 0.0f, (float)alt_window->render_height, -1.0f, 1.0f);

			assets::Shader* shader = assets::shaders()->load_from_path("shaders/gui");
			rs.add_shader(shader->program);
			
			rs.add_uniform_matrix4(shader->program->get_uniform_location("modelview_matrix"), &modelview);
			rs.add_uniform_matrix4(shader->program->get_uniform_location("projection_matrix"), &projection);
			rs.add_uniform1i(shader->program->get_uniform_location("enable_sampler"), 1);
			rs.add_sampler2d(shader->program->get_uniform_location("diffusemap"), 0, gui_texture);
			
			rs.add_draw_call(alt_vs.vertexbuffer);
			
			rs.run_commands();
			
			// do rendering...
			
			window_interface->swap_buffers(alt_window);
		}
	}
	
	virtual void shutdown()
	{
		// shutdown data input thread
		if (data_input.device)
		{
			data_input.execute = false;
			LOGV("waiting for data input thread to finish...\n");
			platform::thread_join(data_input.thread_data);
			platform::serial_close(data_input.device);
		}
	
		navigation::shutdown();
	
		close_gamelibrary();
		
		// shutdown gui
		MEMORY_DELETE(gui_style, platform::memory::global_allocator());
		MEMORY_DELETE(gui_renderer, platform::memory::global_allocator());
		delete compositor;
		compositor = 0;
		_compositor = 0;
		
		// shutdown scene render method
		MEMORY_DELETE(render_method, platform::memory::global_allocator());
		
		// shutdown vr
		if (device)
		{
			vr::destroy_device(device);
		}
		vr::shutdown();
		
		
		
		::renderer::driver()->render_target_destroy(gui_render_target);
		::renderer::driver()->texture_destroy(gui_texture);

		if (alt_window)
		{
			window_interface->activate_window(alt_window);
			alt_vs.destroy();
			window_interface->destroy_window(alt_window);
		}

		// shutdown subsystems
		hotloading::shutdown();
		animation::shutdown();
		gemini::physics::shutdown();
		debugdraw::shutdown();
		font::shutdown();
		assets::shutdown();
		input::shutdown();
		audio::shutdown();
		::renderer::shutdown();
		core::shutdown();
	
		delete event_queue;
		event_queue = 0;
		
		window_interface->activate_window(main_window);
		window_interface->destroy_window(main_window);
		main_window = 0;
	
		window_interface->shutdown();
		platform::destroy_window_library();
		
		MEMORY_DELETE(engine_interface, platform::memory::global_allocator());
		MEMORY_DELETE(scenelink, platform::memory::global_allocator());
		
		platform::memory::MemoryCategoryTracking<MemoryTagGame>::report("game");
	}
};



PLATFORM_MAIN
{
	platform::MainParameters mainparameters;
#if defined(PLATFORM_LINUX) || defined(PLATFORM_APPLE)
	mainparameters.argc = argc;
	mainparameters.argv = argv;
#elif defined(PLATFORM_WINDOWS)
	mainparameters.commandline = commandline;
#elif defined(PLATFORM_ANDROID)
	// nothing to do.
#else
	#error TODO: Implement command line parsing on this platform!
#endif

	platform::set_mainparameters(mainparameters);

	int return_code;
	return_code = platform::run_application(new EngineKernel());
	return return_code;
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
