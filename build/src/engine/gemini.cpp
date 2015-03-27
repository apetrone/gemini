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

#include <core/core.h>
#include <core/str.h>
#include <core/logging.h>
#include <core/filesystem.h>
#include <core/configloader.h>

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
#include "camera.h"
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
	
	renderer::RenderSettings render_settings;
	
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
	renderer::RenderSettings* settings = static_cast<renderer::RenderSettings*>(data);
	
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



void render_scene_from_camera(gemini::IEngineEntity** entity_list, Camera& camera, renderer::SceneLink& scenelink)
{
	// setup constant buffer
	glm::vec3 light_position;
	light_position = camera.pos + -camera.view + glm::vec3(0.0f, 1.0f, 0.0f);
	renderer::ConstantBuffer cb;
	cb.modelview_matrix = &camera.matCam;
	cb.projection_matrix = &camera.matProj;
	cb.viewer_direction = &camera.view;
	cb.viewer_position = &camera.eye_position;
	cb.light_position = &light_position;
	
	// use the entity list to render
	scenelink.clear();
	scenelink.queue_entities(cb, entity_list, MAX_ENTITIES, RENDER_VISIBLE);
	scenelink.sort();
	scenelink.draw(cb);
}

void render_entity_from_camera(gemini::IEngineEntity* entity, Camera& camera, renderer::SceneLink& scenelink)
{
	// setup constant buffer
	glm::vec3 light_position;
	light_position = camera.pos + -camera.view + glm::vec3(0.0f, 1.0f, 0.0f);
	renderer::ConstantBuffer cb;
	
	cb.modelview_matrix = &camera.matCam;
	cb.projection_matrix = &camera.matProj;
	cb.viewer_direction = &camera.view;
	cb.viewer_position = &camera.eye_position;
	cb.light_position = &light_position;
	
	scenelink.clear();
	
	scenelink.queue_entities(cb, &entity, 1, RENDER_VIEWMODEL);
	
	scenelink.draw(cb);
}



class SceneRenderMethod
{
public:
	virtual ~SceneRenderMethod() {}
	virtual void render_view(gemini::IEngineEntity** entity_list, const kernel::Parameters& params, const glm::vec3& origin, const glm::vec2& view_angles) = 0;
	virtual void render_viewmodel(gemini::IEngineEntity* entity, const kernel::Parameters& params, const glm::vec3& origin, const glm::vec2& view_angles) = 0;
	
	virtual void render_gui() = 0;
};


class VRRenderMethod : public SceneRenderMethod
{
	vr::HeadMountedDevice* device;
	
	renderer::SceneLink& scenelink;
	
public:
	VRRenderMethod(vr::HeadMountedDevice* in_device, renderer::SceneLink& in_link) : device(in_device), scenelink(in_link) {}
	
	virtual void render_view(gemini::IEngineEntity** entity_list, const kernel::Parameters& params, const glm::vec3& origin, const glm::vec2& view_angles)
	{
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
		
		Camera camera;
		camera.type = Camera::FIRST_PERSON;
		camera.perspective(50.0f, params.render_width, params.render_height, 0.01f, 8192.0f);
		camera.set_absolute_position(origin);
		camera.eye_position = origin;
		camera.view = glm::vec3(0, 0, -1);
		camera.pitch = view_angles.x;
		camera.yaw = view_angles.y;
		camera.update_view();
		
		
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
	}
	
	virtual void render_viewmodel(gemini::IEngineEntity* entity, const kernel::Parameters& params, const glm::vec3& origin, const glm::vec2& view_angles)
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
	renderer::SceneLink& scenelink;
public:
	DefaultRenderMethod(renderer::SceneLink& in_link) : scenelink(in_link) {};
	
	
	virtual void render_view(gemini::IEngineEntity** entity_list, const kernel::Parameters& params, const glm::vec3& origin, const glm::vec2& view_angles)
	{
		RenderStream rs;
		rs.add_cullmode(renderer::CullMode::CULLMODE_BACK);
		rs.add_state(renderer::STATE_DEPTH_WRITE, 1);
		//		rs.add_state(renderer::STATE_BACKFACE_CULLING, 0);
		rs.add_state(renderer::STATE_DEPTH_TEST, 1);
		rs.add_viewport( 0, 0, params.render_width, params.render_height );
		rs.add_clearcolor( 0.0, 0.0, 0.0, 1.0f );
		rs.add_clear( renderer::CLEAR_COLOR_BUFFER | renderer::CLEAR_DEPTH_BUFFER );
		rs.run_commands();
		
		Camera camera;
		camera.type = Camera::FIRST_PERSON;
		
		camera.perspective(50.0f, params.render_width, params.render_height, 0.01f, 8192.0f);
		
		camera.set_absolute_position(origin);
		camera.eye_position = origin;
		camera.view = glm::vec3(0, 0, -1);
		camera.pitch = view_angles.x;
		camera.yaw = view_angles.y;
		camera.update_view();
		
		render_scene_from_camera(entity_list, camera, scenelink);
		
		debugdraw::render(camera.matCam, camera.matProj, 0, 0, params.render_width, params.render_height);
	}
	
	virtual void render_viewmodel(gemini::IEngineEntity* entity, const kernel::Parameters& params, const glm::vec3& origin, const glm::vec2& view_angles)
	{
		RenderStream rs;
		rs.add_cullmode(renderer::CullMode::CULLMODE_BACK);
		rs.add_state(renderer::STATE_BACKFACE_CULLING, 1);
		rs.add_state(renderer::STATE_DEPTH_TEST, 1);
		
		rs.add_clear( renderer::CLEAR_DEPTH_BUFFER );
		rs.run_commands();
		
		Camera camera;
		camera.type = Camera::FIRST_PERSON;
		
		camera.perspective(35.0f, params.render_width, params.render_height, 0.01f, 32.0f);
		
		camera.set_absolute_position(glm::vec3(-1.0f, 0.6f, 2.5f));
		camera.eye_position = origin;
		camera.view = glm::vec3(0, 0, -1);
		camera.pitch = 0;
		camera.yaw = 0;
		camera.update_view();
		
		render_entity_from_camera(entity, camera, scenelink);
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
	class AnimationInstanceData
	{
	private:
		// TODO: needs a list of animation sequences
		
		
	public:
		
	};
	
	
	
	// Each entity that has a model associated with it
	// will have a model instance data allocated.
	class ModelInstanceData : public IModelInstanceData
	{
		unsigned int mesh_asset_index;
		assets::Mesh* mesh;
		glm::mat4 transform;
		
		glm::mat4* bone_transforms;
		glm::mat4* local_transforms;
		
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
		local_transforms(0),
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
			if (!mesh->skeleton.empty())
			{
				bone_transforms = new glm::mat4[mesh->skeleton.size()];
				local_transforms = new glm::mat4[mesh->skeleton.size()];
				//				for (size_t index = 0; index < mesh->animation.total_bones; ++index)
				//				{
				//					scale_channel.set_data_source(&mesh->animation.scale[index], mesh->animation.frame_delay_seconds);
				//					rotation_channel.set_data_source(&mesh->animation.rotation[index], mesh->animation.frame_delay_seconds);
				//					translation_channel.set_data_source(&mesh->animation.translation[index], mesh->animation.frame_delay_seconds);
				//				}
				//				animated_node->scale_channel.set_data_source(&mesh->animation.scale[node_index], mesh->animation.frame_delay_seconds);
				//				animated_node->rotation_channel.set_data_source(&mesh->animation.rotation[node_index], mesh->animation.frame_delay_seconds);
				//				animated_node->translation_channel.set_data_source(&mesh->animation.translation[node_index], mesh->animation.frame_delay_seconds);
				
				
#if 0
				for (size_t index = 0; index < mesh->animation.total_keys; ++index)
				{
					assets::Joint* joint = &mesh->skeleton[index];
					glm::mat4& transform = bone_transforms[index];
					transform = glm::translate(glm::mat4(1.0f), mesh->animation.translation[index].keys[0]);
					
					if (joint->parent_index > -1)
					{
						transform = bone_transforms[joint->parent_index] * transform;
					}
					
					transform = transform * joint->inverse_bind_matrix;
				}
#endif
				//				glm::mat4& b0 = bone_transforms[0];
				//				b0 = glm::rotate(glm::mat4(1.0f), mathlib::degrees_to_radians(45), glm::vec3(0.0f, 1.0f, 0.0f));
				//				b0 = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.5f, 0.0f));
			}
		}
		
		void destroy_bones()
		{
			if (bone_transforms)
			{
				delete [] bone_transforms;
				bone_transforms = 0;
			}
			
			if (local_transforms)
			{
				delete [] local_transforms;
				local_transforms = 0;
			}
		}
		
		virtual unsigned int asset_index() const { return mesh_asset_index; }
		virtual glm::mat4& get_local_transform() { return transform; }
		virtual void set_local_transform(const glm::mat4& _transform) { transform = _transform; }
		
		//		virtual void get_geometry_data(unsigned int index, GeometryInstanceData& geometry_data) const
		//		{
		//			assert(mesh != 0);
		//
		//			// TODO: verify index is valid
		//			assets::Geometry* geometry = &mesh->geometry[index];
		//			geometry_data.material_id = geometry->material_id;
		//			geometry_data.shader_id = geometry->shader_id;
		//		}
		
		virtual glm::mat4* get_bone_transforms() const { return bone_transforms; }
		
		virtual void set_animation_enabled(int32_t index, bool enabled)
		{
			animation::AnimatedInstance* instance = animation::get_instance_by_index(index);
			assert(instance != 0);
			if (instance)
			{
				instance->enabled = enabled;
			}
		}
		
		virtual void get_animation_pose(int32_t index, glm::vec3* positions, glm::quat* rotations, float t)
		{
#if 0
			if (mesh->skeleton.empty())
			{
				return;
			}
			
			mesh->animation.get_pose(positions, rotations, t);
#else

			animation::SequenceId instance_index = animations[index];
			animation::AnimatedInstance* instance = animation::get_instance_by_index(instance_index);

			size_t node_index = 0;
			for (core::FixedArray<animation::Channel>& node : instance->ChannelSet)
			{
				glm::vec3& pos = positions[node_index];
				glm::quat& rot = rotations[node_index];
				const animation::Channel& tx = node[0];
				const animation::Channel& ty = node[1];
				const animation::Channel& tz = node[2];
				pos = glm::vec3(tx(), ty(), tz());
				
				if (node.size() > 3)
				{
					const animation::Channel& rx = node[3];
					const animation::Channel& ry = node[4];
					const animation::Channel& rz = node[5];
					
					// pitch, yaw, roll (in radians)
					rot = glm::quat(glm::vec3(rx(), ry(), rz()));
					
//					LOGV("%g %g %g\n", rot.x, rot.y, rot.z);					
				}
				

				
//				for (animation::Channel& channel : node)
//				{
//				}
				
				++node_index;
			}
			
			
#endif
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
			
			// recalculate
			for (size_t index = 0; index < mesh->skeleton.size(); ++index)
			{
				assets::Joint* joint = &mesh->skeleton[index];
				glm::mat4& global_pose = bone_transforms[index];
				glm::mat4& saved_pose = local_transforms[index];
				
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
				
				global_pose = saved_pose * joint->inverse_bind_matrix;
				
				
				glm::mat4 bone_world_transform = tx * (saved_pose);
				debugdraw::axes(bone_world_transform, 0.25f);
			}
		}
		
		virtual int32_t get_animation_index(const char* name)
		{
			int32_t index = -1;
			for (auto& id : animations)
			{
				animation::Sequence* sequence = animation::get_sequence_by_index(id);
				if (0 == core::str::case_insensitive_compare(name, sequence->name(), 0))
				{
					return sequence->index;
					break;
				}
			}
			
			assert(index > -1);
			return index;
		}
		
		virtual int32_t add_animation(const char* name)
		{
			animation::SequenceId id = animation::load_sequence(name, mesh);
			if (id > -1)
			{
				animations.push_back(id);
				LOGV("[engine] added animation %s to index: %i\n", name, animations.size()-1);
			}
			else
			{
				LOGW("Unable to load sequence %s\n", name);
			}
			return id;
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
public:
	
	virtual void get_player_command(uint8_t index, physics::MovementCommand& command)
	{
		// add the inputs and then normalize
		input::JoystickInput& joystick = input::state()->joystick(0);
		
		command.left += input::state()->keyboard().is_down(input::KEY_A) * input::AxisValueMaximum;
		command.right += input::state()->keyboard().is_down(input::KEY_D) * input::AxisValueMaximum;
		command.forward += input::state()->keyboard().is_down(input::KEY_W) * input::AxisValueMaximum;
		command.back += input::state()->keyboard().is_down(input::KEY_S) * input::AxisValueMaximum;
		
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
	}
};


class EngineInterface : public IEngineInterface
{
	IEntityManager* entity_manager;
	IModelInterface* model_interface;
	gemini::physics::IPhysicsInterface* physics_interface;
	IExperimental* experimental_interface;
	
	SceneRenderMethod* render_method;
	Camera* camera;
	
public:
	
	EngineInterface(
					IEntityManager* em,
					IModelInterface* mi,
					gemini::physics::IPhysicsInterface* pi,
					IExperimental* ei,
					SceneRenderMethod* rm,
					Camera* cam) :
	entity_manager(em),
	model_interface(mi),
	physics_interface(pi),
	experimental_interface(ei),
	render_method(rm),
	camera(cam)
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
	
	virtual void* allocate(size_t bytes)
	{
		return platform::memory::allocator().allocate(bytes, __FILE__, __LINE__);
	}
	
	virtual void deallocate(void* pointer)
	{
		platform::memory::allocator().deallocate(pointer);
	}
	
	virtual void render_view(const glm::vec3& origin, const glm::vec2& view_angles)
	{
		// TODO: need to validate this origin/orientation is allowed.
		// otherwise, client could ask us to render from anyone's POV.
		EntityManager* em = static_cast<EntityManager*>(engine::api::instance()->entities());
		render_method->render_view(em->get_entity_list(), kernel::parameters(), origin, view_angles);
	}
	
	virtual void render_gui()
	{
		render_method->render_gui();
	}
	
	virtual void render_viewmodel(IEngineEntity* entity, const glm::vec3& origin, const glm::vec2& view_angles)
	{
		render_method->render_viewmodel(entity, kernel::parameters(), origin, view_angles);
	}
	
	virtual void get_view_angles(glm::vec2& view_angles)
	{
		if (camera)
		{
			view_angles.x = camera->pitch;
			view_angles.y = camera->yaw;
		}
	}
};



typedef gemini::IGameInterface* (*connect_engine_fn)(gemini::IEngineInterface*);
typedef void (*disconnect_engine_fn)();

class CustomListener : public gui::Listener
{
private:
	
	audio::SoundHandle hover_sound;
	IGameInterface* game_interface;
	gui::Panel* root;
	
	bool& is_in_gui;
	
	
	
	renderer::RenderTarget* cubemap;
	renderer::Texture* cubemap_texture;
	
public:
	CustomListener(bool& in_gui) : is_in_gui(in_gui)
	{
		cubemap = 0;
		cubemap_texture = 0;
	}
	
	
	
	void setup_rendering(const kernel::Parameters& params)
	{
		renderer::IRenderDriver* driver = renderer::driver();
		
//		image::Image image;
//		image.type = image::TEX_CUBE;
//		image.width = 128;
//		image.height = 128;
//		
//		cubemap_texture = driver->texture_create(image);
		
		
		
		
//		cubemap = driver->render_target_create(image.width, image.height);
//		driver->render_target_set_attachment(cubemap, renderer::RenderTarget::COLOR, 0, cubemap_texture);
	}
	
	void shutdown()
	{
		renderer::IRenderDriver* driver = renderer::driver();
//		driver->render_target_destroy(cubemap);
		
		
//		driver->texture_destroy(cubemap_texture);
	}
	
	void test_rendercubemap()
	{
		LOGV("rendering a cubemap...\n");
	}
	
	void set_root(gui::Panel* root_panel) { root = root_panel; }
	void set_game_interface(IGameInterface* game) { game_interface = game; }
	void set_hover_sound(audio::SoundHandle handle) { hover_sound = handle; }
	
	virtual void focus_changed(gui::Panel* old_focus, gui::Panel* new_focus) {}
	virtual void hot_changed(gui::Panel* old_hot, gui::Panel* new_hot)
	{
		size_t data = reinterpret_cast<size_t>(new_hot->get_userdata());
		if (data > 0)
		{
			audio::play(hover_sound);
		}
	}
	
	virtual void handle_event(const gui::EventArgs& event)
	{
		LOGV("focus: %p\n", event.focus);
		LOGV("hot: %p\n", event.hot);
		LOGV("handle event: %i\n", event.type);
		size_t data = reinterpret_cast<size_t>(event.focus->get_userdata());
		if (event.focus->is_button())
		{
			switch(data)
			{
				case 1:
					kernel::instance()->set_active(false);
					break;
				case 2:
					// need to hide the root panel since we loaded a scene as well!
					root->set_visible(false);
					is_in_gui = false;
					game_interface->level_load();
					break;
				case 3:
					// DEBUG: test rendering a cubemap
					test_rendercubemap();
					break;
			}
		}
	}
};

#include <platform/windowlibrary.h>



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
	bool has_focus;
	bool in_gui;
	bool draw_physics_debug;
	
	platform::IWindowLibrary* window_interface;
		
	// Kernel State variables
	double accumulator;
	uint64_t last_time;
	
	// rendering
	renderer::SceneLink* scenelink;
	SceneRenderMethod* render_method;
	Camera main_camera;
	
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
	CustomListener gui_listener;
	
	// audio
	audio::SoundHandle menu_show;
	audio::SoundHandle menu_hide;
	
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
		
	}
	
	void close_gamelibrary()
	{
		
	}
	
	
	void center_mouse(const kernel::Parameters& params)
	{
		if (has_focus && !in_gui)
		{
			window_interface->warp_mouse(params.window_width/2, params.window_height/2);
		}
	}
	
public:
	EngineKernel() :
		active(true),
		accumulator(0.0f),
		last_time(0),
		engine_interface(0),
		game_interface(0),
		has_focus(true),
		in_gui(false),
		gui_listener(in_gui),
		render_method(0),
		draw_physics_debug(false)
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
		if (event.is_down)
		{
			if (event.key == input::KEY_ESCAPE)
			{
#if 0
				in_gui = !in_gui;
				if (!in_gui)
				{
					center_mouse(kernel::parameters());
					audio::play(menu_hide);
				}
				else
				{
					audio::play(menu_show);
				}
				
				window_interface->show_mouse(in_gui);
				
				root->set_visible(in_gui);
#else
				set_active(false);

#endif
			}
			else if (event.key == input::KEY_SPACE)
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
			else if (event.key == input::KEY_M)
			{
				LOGV("level load\n");
				game_interface->level_load();
			}
		}
		
		
		if (in_gui && compositor)
		{
			compositor->key_event(event.key, event.is_down, 0);
		}
	}
	
	virtual void event(kernel::MouseEvent& event)
	{
		if (in_gui)
		{
			gui::CursorButton::Type input_to_gui[] = {
				gui::CursorButton::Left,
				gui::CursorButton::Right,
				gui::CursorButton::Middle,
				gui::CursorButton::Mouse4,
				gui::CursorButton::Mouse5
			};
			
			gui::CursorButton::Type button;
			
			
			switch( event.subtype )
			{
				case kernel::MouseMoved:
				{
					if ( compositor )
					{
						compositor->cursor_move_absolute( event.mx, event.my );
					}
					break;
				}
				case kernel::MouseButton:
					
					button = input_to_gui[ event.button ];
					if ( event.is_down )
					{
						fprintf( stdout, "mouse button %i is pressed\n", event.button );
					}
					else
					{
						fprintf( stdout, "mouse button %i is released\n", event.button );
					}
					
					if ( compositor )
					{
						compositor->cursor_button( button, event.is_down );
					}
					break;
					
				case kernel::MouseWheelMoved:
					if ( event.wheel_direction > 0 )
					{
						fprintf( stdout, "mouse wheel toward screen\n" );
					}
					else
					{
						fprintf( stdout, "mouse wheel away from screen\n" );
					}
					break;
				default:
					fprintf( stdout, "mouse event received!\n" );
					break;
			}
		}
	}
	
	virtual void event( kernel::SystemEvent & event )
	{
		if (event.subtype == kernel::WindowLostFocus)
		{
			//			kernel::instance()->show_mouse(true);
			has_focus = false;
		}
		else if (event.subtype == kernel::WindowGainFocus)
		{
			//			kernel::instance()->show_mouse(false);
			has_focus = true;
		}
	}
	
	virtual void event(kernel::GameControllerEvent& event)
	{
		//		if (event.subtype == kernel::JoystickConnected)
		//		{
		//			LOGV("gamepad [%i] connected\n", event.gamepad_id);
		//		}
		//		else if (event.subtype == kernel::JoystickDisconnected)
		//		{
		//			LOGV("gamepad [%i] disconnected\n", event.gamepad_id);
		//		}
		//		else if (event.subtype == kernel::JoystickButton)
		//		{
		//			LOGV("gamepad [%i] button: %i, is_down: %i\n", event.gamepad_id, event.button, event.is_down);
		//		}
		//		else if (event.subtype == kernel::JoystickAxisMoved)
		//		{
		//			LOGV("gamepad [%i] joystick: %i, value: %i\n", event.gamepad_id, event.joystick_id, event.joystick_value);
		//		}
		//		else
		//		{
		//			LOGV("gamepad [%i] controller event received: %i\n", event.gamepad_id, event.subtype);
		//		}
	}
	
	void setup_gui(const kernel::Parameters& params)
	{
		compositor = new gui::Compositor(params.render_width, params.render_height);
		_compositor = compositor;
		
		gui_renderer = CREATE(GUIRenderer);
		compositor->set_renderer(gui_renderer);
		
		gui_style = CREATE(GUIStyle);
		compositor->set_style(gui_style);
		
		root = new gui::Panel(compositor);
		root->set_bounds(0, 0, params.render_width, params.render_height);
		root->set_background_color(gui::Color(0, 0, 0, 192));
		root->set_visible(in_gui);
		compositor->add_child(root);
		
		gui::Color button_background(128, 128, 128, 255);
		gui::Color button_hover(255, 255, 128, 255);
		
		uint32_t button_width = 320;
		uint32_t button_height = 50;
		uint32_t button_spacing = 10;
		uint32_t total_buttons = 2;
		uint32_t vertical_offset = 0;
		uint32_t origin_x = (params.render_width/2.0f) - (button_width/2.0f);
		uint32_t origin_y = (params.render_height/2.0f) - ((button_height*total_buttons)/2.0f);
		
		
		
		newgame = new gui::Button(root);
		newgame->set_bounds(origin_x, origin_y, button_width, button_height);
		newgame->set_font(compositor, "fonts/default16");
		newgame->set_text("New Game");
		newgame->set_background_color(button_background);
		newgame->set_hover_color(button_hover);
		newgame->set_userdata((void*)2);
		root->add_child(newgame);
		origin_y += (button_height+button_spacing);
		
		test = new gui::Button(root);
		test->set_bounds(origin_x, origin_y, button_width, button_height);
		test->set_font(compositor, "fonts/default16");
		test->set_text("[test] render cubemap");
		test->set_background_color(button_background);
		test->set_hover_color(button_hover);
		test->set_userdata((void*)3);
		root->add_child(test);
		origin_y += (button_height+button_spacing);
		
		quit = new gui::Button(root);
		quit->set_bounds(origin_x, origin_y, button_width, button_height);
		quit->set_font(compositor, "fonts/default16");
		quit->set_text("Quit Game");
		quit->set_background_color(button_background);
		quit->set_hover_color(button_hover);
		quit->set_userdata((void*)1);
		root->add_child(quit);
		origin_y += (button_height+button_spacing);
		
		
		//		gui::Panel* root = new gui::Panel(compositor);
		//		gui::Label* b = new gui::Label(compositor);
		//		b->set_bounds(50, 50, 250, 250);
		//		b->set_font(compositor, "fonts/default16");
		//		b->set_text("hello");
		//		compositor->add_child(b);
		
		
		//		gui::Panel* root = new gui::Panel(compositor);
		//		root->set_bounds(0, 0, 500, 500);
		//		compositor->add_child(root);
		
		graph = new gui::Graph(root);
		graph->set_bounds(params.render_width-250, 0, 250, 250);
		graph->set_font(compositor, "fonts/debug");
		graph->set_background_color(gui::Color(10, 10, 10, 210));
		graph->set_foreground_color(gui::Color(255, 255, 255, 255));
		graph->create_samples(100, 1);
		graph->configure_channel(0, gui::Color(255, 0, 0, 255));
		graph->set_range(-10.0f, 30.0f);
		root->add_child(graph);
		graph->enable_baseline(true, 16.0f, gui::Color(255, 0, 255, 255));
		
		// setup the listener
		gui_listener.set_hover_sound(audio::create_sound("sounds/8b_select1"));
		gui_listener.set_game_interface(game_interface);
		gui_listener.set_root(root);
		gui_listener.setup_rendering(params);
		compositor->set_listener(&gui_listener);
	}
	
	virtual kernel::Error startup()
	{
		// parse command line values
		const platform::MainParameters& mainparams = platform::get_mainparameters();
#if defined(PLATFORM_LINUX) || defined(PLATFORM_APPLE)
		const char* arg;
		for(int i = 0; i < mainparams.argc; ++i)
		{
			arg = mainparams.argv[i];
			if (String(arg) == "--game")
			{
				game_path = mainparams.argv[i+1];
			}
		}
#endif

		const char FONT_SHADER[] = "shaders/fontshader";
		const char DEBUG_FONT[] = "fonts/debug";
		const char DEBUG_SHADER[] = "shaders/debug";
		kernel::Parameters& params = kernel::parameters();
	
		// initialize timer
		last_time = platform::instance()->get_time_microseconds();

#if defined(PLATFORM_MOBILE)
#else
		kernel::parameters().device_flags |= kernel::DeviceDesktop;
#endif

		//
		// setup our file system...
		StackString< MAX_PATH_SIZE > root_path;
		platform::Result result = platform::instance()->get_program_directory(&root_path[0], root_path.max_size());
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
		
		// TODO: we should load these from a config; for now just set them.
		params.window_width = config.window_width;
		params.window_height = config.window_height;
		params.window_title = "gemini";
		
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
						params.target_display = 2;
					}
#endif
					params.use_vsync = true;
					params.window_width = (uint16_t)width;
					params.window_height = (uint16_t)height;
				}
			}
		}
		
		// create the window
		window_interface->create_window(kernel::parameters());

		// initialize rendering subsystems
		{
			scenelink = CREATE(renderer::SceneLink);
			int render_result =	renderer::startup(renderer::Default, config.render_settings);
			if ( render_result == 0 )
			{
				LOGE("renderer initialization failed!\n");
				return kernel::RendererFailed;
			}
			
			assets::startup();
			
			assets::Shader* fontshader = assets::shaders()->load_from_path(FONT_SHADER);
			assert(fontshader != 0);
			font::startup(fontshader->program, params.render_width, params.render_height);
			
			assets::Shader* debugshader = assets::shaders()->load_from_path(DEBUG_SHADER);
			assets::Font* debugfont = assets::fonts()->load_from_path(DEBUG_FONT);
			assert(debugshader != 0);
			assert(debugfont != 0);
			debugdraw::startup(config.debugdraw_max_primitives, debugshader->program, debugfont->handle);
		}
		
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
			vr::setup_rendering(device, params.render_width, params.render_height);
			// TODO: instance render method for VR
			render_method = CREATE(VRRenderMethod, device, *scenelink);
		}
		else
		{
			// TODO: instance render method for default
			render_method = CREATE(DefaultRenderMethod, *scenelink);
		}
		

//		animation::SequenceId seq = animation::load_sequence("models/bones9.animation");
//		animation::AnimatedInstance* i0 = animation::create_sequence_instance(seq);


		menu_show = audio::create_sound("sounds/menu_show3");
		menu_hide = audio::create_sound("sounds/menu_hide");
		
		// TODO: setup gui
		
		// TODO: setup interfaces
		engine_interface = CREATE(EngineInterface,
			&entity_manager,
			&model_interface,
			physics::api::instance(),
			&experimental,
			render_method,
			&main_camera
		);
		gemini::engine::api::set_instance(engine_interface);
		window_interface->show_mouse(in_gui);
		
		// TOOD: load game library
		StackString<MAX_PATH_SIZE> game_library_path = ::core::filesystem::content_directory();
		const char* dynamiclibrary_extension = platform::instance()->get_dynamiclibrary_extension();
		game_library_path.append(PATH_SEPARATOR_STRING).append("bin").append(PATH_SEPARATOR_STRING).append("game").append(dynamiclibrary_extension);
		gamelib = platform::instance()->open_dynamiclibrary(game_library_path());
		if (!gamelib)
		{
			LOGV("unable to open game: \"%s\"\n", game_library_path());
			assert(0);
		}
		else
		{
			LOGV("opened game library!\n");
			
			// link the engine interface
			
			connect_engine_fn connect_engine = (connect_engine_fn)platform::instance()->find_dynamiclibrary_symbol(gamelib, "connect_engine");
			disconnect_engine = (disconnect_engine_fn)platform::instance()->find_dynamiclibrary_symbol(gamelib, "disconnect_engine");
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

		setup_gui(kernel::parameters());

		// for debugging
		game_interface->level_load();

		
		
		// TODO: post_application_startup
		
		uint64_t current_time = platform::instance()->get_time_microseconds();
		LOGV("startup in %2.2fms\n", (current_time-last_time)*.001f);
		last_time = current_time;
		
		return kernel::NoError;
	}
	
	
	
	void update()
	{
		uint64_t current_time = platform::instance()->get_time_microseconds();
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
		update();
	
		audio::update();
		input::update();
		animation::update(kernel::parameters().framedelta_filtered_seconds);
		window_interface->process_events();
		
		hotloading::tick();
		// TODO: application -> tick
		post_tick();
		kernel::parameters().current_frame++;
	}
	
	void post_tick()
	{
		if (graph)
		{
			graph->record_value(kernel::parameters().framedelta_raw_msec, 0);
		}
		
		if (compositor)
		{
			compositor->update(kernel::parameters().framedelta_raw_msec);
		}
		
		if (!in_gui)
		{
			UserCommand command;
			
			// attempt to sample input here -- may need to be moved.
			bool left = input::state()->keyboard().is_down(input::KEY_A);
			bool right = input::state()->keyboard().is_down(input::KEY_D);
			bool forward = input::state()->keyboard().is_down(input::KEY_W);
			bool back = input::state()->keyboard().is_down(input::KEY_S);
			
			command.set_button(0, left);
			command.set_button(1, right);
			command.set_button(2, forward);
			command.set_button(3, back);
			
			command.set_button(5, input::state()->keyboard().is_down(input::KEY_E));
			command.set_button(6, input::state()->keyboard().is_down(input::KEY_SPACE));
			command.set_button(7, input::state()->keyboard().is_down(input::KEY_LALT) || input::state()->keyboard().is_down(input::KEY_RALT));
			
			command.set_button(8, input::state()->mouse().is_down(input::MOUSE_LEFT));
			command.set_button(9, input::state()->mouse().is_down(input::MOUSE_MIDDLE));
			command.set_button(10, input::state()->mouse().is_down(input::MOUSE_RIGHT));
			
			command.set_button(11, input::state()->keyboard().is_down(input::KEY_G));
			
			int mouse[2];
			window_interface->get_mouse(mouse[0], mouse[1]);
			
			int half_width = kernel::parameters().window_width/2;
			int half_height = kernel::parameters().window_height/2;
			
			// capture the state of the mouse
			int mdx, mdy;
			mdx = (mouse[0] - half_width);
			mdy = (mouse[1] - half_height);
			if (mdx != 0 || mdy != 0)
			{
				main_camera.move_view(mdx, mdy);
			}
			
			command.angles[0] = main_camera.pitch;
			command.angles[1] = main_camera.yaw;
			
			
			center_mouse(kernel::parameters());
			
			if (game_interface)
			{
				// loop through all players and process inputs
				game_interface->process_commands(0, &command, 1);
				
				//			game_interface->physics_update(params.step_interval_seconds);
				//			background_source = audio::play(background, 1);
			}
		}

		int x = 10;
		int y = 10;
		{

			debugdraw::text(x, y, core::str::format("active_camera->pos = %.2g %.2g %.2g", main_camera.pos.x, main_camera.pos.y, main_camera.pos.z), Color(255, 255, 255));
			debugdraw::text(x, y+12, core::str::format("eye_position = %.2g %.2g %.2g", main_camera.eye_position.x, main_camera.eye_position.y, main_camera.eye_position.z), Color(255, 0, 255));
			debugdraw::text(x, y+24, core::str::format("active_camera->view = %.2g %.2g %.2g", main_camera.view.x, main_camera.view.y, main_camera.view.z), Color(128, 128, 255));
			debugdraw::text(x, y+36, core::str::format("active_camera->right = %.2g %.2g %.2g", main_camera.side.x, main_camera.side.y, main_camera.side.z), Color(255, 0, 0));
		}
		debugdraw::text(x, y+48, core::str::format("frame delta = %2.2fms\n", kernel::parameters().framedelta_raw_msec), Color(255, 255, 255));
		debugdraw::text(x, y+60, core::str::format("# allocations = %i, total %i Kbytes\n", platform::memory::allocator().active_allocations(), platform::memory::allocator().active_bytes()/1024), Color(64, 102, 192));
		
		
		if (draw_physics_debug)
		{
			physics::debug_draw();
		}
	
	
		if (game_interface)
		{
			float framedelta_seconds = kernel::parameters().framedelta_raw_msec*0.001f;
			
			game_interface->server_frame(
				kernel::parameters().current_tick,
				framedelta_seconds,
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
			window_interface->swap_buffers();
		}
	}
	
	virtual void shutdown()
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
		
		platform::instance()->close_dynamiclibrary(gamelib);
		
		// shutdown gui
		DESTROY(GUIStyle, gui_style);
		DESTROY(GUIRenderer, gui_renderer);
		gui_listener.shutdown();
		delete compositor;
		compositor = 0;
		_compositor = 0;
		
		// shutdown scene render method
		DESTROY(SceneRenderMethod, render_method);
		
		// shutdown vr
		if (device)
		{
			vr::destroy_device(device);
		}
		vr::shutdown();
		

		// shutdown subsystems
		hotloading::shutdown();
		animation::shutdown();
		gemini::physics::shutdown();
		debugdraw::shutdown();
		font::shutdown();
		assets::shutdown();
		input::shutdown();
		audio::shutdown();
		renderer::shutdown();
		core::shutdown();
	
		window_interface->shutdown();
		platform::destroy_window_library();
		
		DESTROY(IEngineInterface, engine_interface);
		DESTROY(SceneLink, scenelink);
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