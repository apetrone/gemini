// -------------------------------------------------------------
// Copyright (C) 2013-, Adam Petrone
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

#include "kernel.h"

#include <platform/platform.h>

#include <stdio.h>
#include <core/logging.h>
#include <core/mathlib.h>
#include "debugdraw.h"
#include "input.h"
#include <renderer/renderer.h>
#include <renderer/renderstream.h>

#include "camera.h"

#include "assets/asset_mesh.h"
#include "assets/asset_material.h"

#include "btBulletDynamicsCommon.h"

#include "physics/physics.h"
#include "physics/physics_player_controller.h"

#include <renderer/font.h>
#include "assets/asset_font.h"

#include <core/mathlib.h>

#include "scenelink.h"
#include "vr.h"

#include <renderer/constantbuffer.h>

#include "audio.h"

#include <core/dictionary.h>

#include <core/filesystem.h>
#include <core/logging.h>
//#include <core/ringbuffer.h>

#include <sdk/audio_api.h>
#include <sdk/entity_api.h>
#include <sdk/iengineentity.h>
#include <sdk/model_api.h>
#include <sdk/engine_api.h>
#include <sdk/game_api.h>
#include <sdk/experimental_api.h>
#include <sdk/debugdraw_api.h>
#include <sdk/shared.h>

#include <platform/mem.h>

using namespace gemini;
using namespace gemini::physics;
using namespace core;

#define LOCK_CAMERA_TO_CHARACTER 1

// even if a VR device is attached, this will NOT render to it
// this allows debugging in some other mechanism to check sensor data.
uint8_t RENDER_TO_VR = 1;

uint8_t REQUIRE_RIFT = 1;

const uint16_t MAX_ENTITIES = 2048;


class IGameState
{
public:
	virtual ~IGameState() {}
	
	/// @desc Called when this game state becomes active
	virtual void activate() = 0;
	
	/// @desc Called when this game state should go inactive
	virtual void deactivate() = 0;
	
	/// @desc Called each frame
	/// @param framedelta_seconds The elapsed time since the last run_frame call (in seconds).
	virtual void run_frame(float framedelta_seconds) = 0;
};


class MainMenuState : public IGameState
{
public:
	virtual void activate() {}
	
	virtual void deactivate() {}
	
	virtual void run_frame(float framedelta_seconds) {}
};

class InGameMenuState : public IGameState
{
public:
	virtual void activate() {}
	
	virtual void deactivate() {}
	
	virtual void run_frame(float framedelta_seconds) {}
};

class GameState : public IGameState
{
public:
	virtual void activate() {}
	
	virtual void deactivate() {}
	
	virtual void run_frame(float framedelta_seconds) {}
};

class StateController
{
private:
	typedef Dictionary<IGameState*> StatesByHash;
	StatesByHash states;
	IGameState* active_game_state;

public:
	StateController() : active_game_state(0) {}
	virtual ~StateController() {}


	void add_state(const char* name, IGameState* state)
	{
		states.insert(name, state);
	}
	
	IGameState* state_by_name(const char* name)
	{
		IGameState* state = 0;
		states.get(name, state);
		return state;
	}
	
	IGameState* active_state() const { return active_game_state; }
};


#include <nom/nom.hpp>
#include <nom/compositor.hpp>
#include <nom/graph.hpp>
#include <nom/button.hpp>

gui::Compositor* _compositor = 0;

class GUIRenderer : public gui::Renderer
{
	struct VertexType
	{
		glm::vec3 position;
		Color color;
		glm::vec2 uv;
	};
	
	gui::Compositor* compositor;
	renderer::VertexStream stream;
	renderer::VertexStream lines;
	assets::Shader* shader;

	assets::Material * solid_color;
	assets::Material * texture_map;
	
	unsigned int vertex_attribs;
	
	float current_depth;
	
private:
	void render_buffer(renderer::VertexStream& stream, assets::Shader* shader, assets::Material* material)
	{
		stream.update();
		
		glm::mat4 modelview;
		glm::mat4 projection = glm::ortho( 0.0f, (float)compositor->width, (float)compositor->height, 0.0f, -0.1f, 256.0f );
		glm::mat4 object_matrix;

		RenderStream rs;
		rs.add_shader( shader->program );
		rs.add_uniform_matrix4( shader->program->get_uniform_location("modelview_matrix"), &modelview );
		rs.add_uniform_matrix4( shader->program->get_uniform_location("projection_matrix"), &projection );
		
		rs.add_material( material, shader->program );
		
		rs.add_draw_call( stream.vertexbuffer );
		
		rs.run_commands();
		stream.reset();
	}
	
	
public:
	GUIRenderer() :
		compositor(0),
		shader(0),
		solid_color(0),
		texture_map(0),
		vertex_attribs(0),
		current_depth(0.0f)
	{
	}

	virtual ~GUIRenderer()
	{
	}


	virtual void increment_depth()
	{
		current_depth += 1.0f;
	}

	virtual void startup(gui::Compositor* c)
	{
		this->compositor = c;
		stream.desc.add(renderer::VD_FLOAT3);
		stream.desc.add(renderer::VD_FLOAT2);
		stream.desc.add(renderer::VD_UNSIGNED_BYTE4);
		stream.create(64, 64, renderer::DRAW_INDEXED_TRIANGLES);
		
		lines.desc.add(renderer::VD_FLOAT3);
		lines.desc.add(renderer::VD_FLOAT2);
		lines.desc.add(renderer::VD_UNSIGNED_BYTE4);
		lines.create(128, 0, renderer::DRAW_LINES);

		// load shader
		shader = assets::shaders()->load_from_path("shaders/gui");

		// setup materials
		solid_color = assets::materials()->allocate_asset();
		if (solid_color)
		{
			renderer::MaterialParameter parameter;
			parameter.type = renderer::MP_VEC4;
			parameter.name = "diffusecolor";
			parameter.vector_value = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			solid_color->add_parameter(parameter);
			
			renderer::MaterialParameter enable_sampler;
			enable_sampler.type = renderer::MP_INT;
			enable_sampler.name = "enable_sampler";
			enable_sampler.int_value = 0;
			solid_color->add_parameter(enable_sampler);
			
			assets::materials()->take_ownership("gui/solid_color", solid_color);
		}
		
		texture_map = assets::materials()->allocate_asset();
		if (texture_map)
		{
			renderer::MaterialParameter parameter;
			parameter.type = renderer::MP_SAMPLER_2D;
			parameter.name = "diffusemap";
			parameter.int_value = assets::textures()->get_default()->Id();
			texture_map->add_parameter(parameter);
			
			renderer::MaterialParameter enable_sampler;
			enable_sampler.type = renderer::MP_INT;
			enable_sampler.name = "enable_sampler";
			enable_sampler.int_value = 1;
			texture_map->add_parameter(enable_sampler);
			
			assets::materials()->take_ownership("gui/texture_map", texture_map);
		}
	}
	
	virtual void shutdown(gui::Compositor* c)
	{
	}
		
	virtual void begin_frame(gui::Compositor* c)
	{
		RenderStream rs;
		
		rs.add_state( renderer::STATE_BLEND, 1 );
		rs.add_blendfunc( renderer::BLEND_SRC_ALPHA, renderer::BLEND_ONE_MINUS_SRC_ALPHA );
		rs.add_state(renderer::STATE_DEPTH_TEST, 0);
		rs.add_state(renderer::STATE_DEPTH_WRITE, 0);
		
		rs.run_commands();
	}
	
	virtual void end_frame()
	{
		RenderStream rs;
		
		rs.add_state( renderer::STATE_BLEND, 0 );
		rs.add_state(renderer::STATE_DEPTH_TEST, 1);
		rs.add_state(renderer::STATE_DEPTH_WRITE, 1);
		rs.run_commands();
	}
	
	
	virtual void draw_bounds(const gui::Bounds& bounds, const gui::Color& color)
	{
		//		gui::Size size = bounds.size;
		//		glm::vec3 start = glm::vec3( bounds.origin.x, bounds.origin.y, 0.0f );
		//		glm::vec3 end = start + glm::vec3( size.width, size.height, 0.0f );
		//		debugdraw::line( start, end, Color( 255, 0, 255 ) );
		//		debugdraw::point( glm::vec3( bounds.origin.x + size.width, bounds.origin.y + size.height, 0.0f ), Color(255, 255, 255) );
		
		float div = 1.0f/255.0f;
		solid_color->parameters[0].vector_value = glm::vec4( (color.r() * div), (color.g() * div), (color.b() * div), (color.a() * div) );
		//		debugdraw::box( start, end, Color(rgba[0], rgba[1], rgba[2], rgba[3]), 0.0f );
		
		stream.reset();
		
		RenderStream rs;
		
		if ( stream.has_room(4, 6) )
		{
			VertexType* v = (VertexType*)stream.request(4);
			
			gui::Size size = bounds.size;
			v[0].position = glm::vec3( bounds.origin.x, bounds.origin.y, 0.0f );
			v[1].position = v[0].position + glm::vec3( 0.0f, size.height, 0.0f );
			v[2].position = v[0].position + glm::vec3( size.width, size.height, 0.0f );
			v[3].position = v[0].position + glm::vec3( size.width, 0.0f, 0.0f );
			
			// lower left corner is the origin in OpenGL
			v[0].uv = glm::vec2(0, 0);
			v[1].uv = glm::vec2(0, 1);
			v[2].uv = glm::vec2(1, 1);
			v[3].uv = glm::vec2(1, 0);
			
			//v[0].color = v[1].color = v[2].color = v[3].color = Color(rgba[0], rgba[1], rgba[2], rgba[3]);
			
			renderer::IndexType indices[] = { 0, 1, 2, 2, 3, 0 };
			stream.append_indices( indices, 6 );
		}
		else
		{
			LOGV( "buffer be full\n" );
		}
		
		this->render_buffer(stream, shader, solid_color);
	}
	
	virtual void draw_textured_bounds(const gui::Bounds& bounds, const gui::TextureHandle& handle)
	{
		stream.reset();
		RenderStream rs;
		assets::Texture * tex = assets::textures()->find_with_id( handle );
		if ( !tex )
		{
			return;
		}
		
		texture_map->parameters[0].int_value = handle;
		texture_map->parameters[0].texture_unit = 0;
		
		if ( stream.has_room(4, 6) )
		{
			VertexType * v = (VertexType*)stream.request(4);
			
			gui::Size size = bounds.size;
			v[0].position = glm::vec3( bounds.origin.x, bounds.origin.y, 0.0f );
			v[1].position = v[0].position + glm::vec3( 0.0f, size.height, 0.0f );
			v[2].position = v[0].position + glm::vec3( size.width, size.height, 0.0f );
			v[3].position = v[0].position + glm::vec3( size.width, 0.0f, 0.0f );
			
			// lower left corner is the origin in OpenGL
			v[0].uv = glm::vec2(0, 0);
			v[1].uv = glm::vec2(0, 1);
			v[2].uv = glm::vec2(1, 1);
			v[3].uv = glm::vec2(1, 0);
			
			v[0].color = v[1].color = v[2].color = v[3].color = Color(255, 255, 255, 255);
			
			renderer::IndexType indices[] = { 0, 1, 2, 2, 3, 0 };
			stream.append_indices( indices, 6 );
		}
		
		this->render_buffer(stream, shader, texture_map);
	}
	
	void draw_line(const gui::Point& start, const gui::Point& end, const gui::Color& color)
	{
		lines.reset();

		float div = 1.0f/255.0f;
		solid_color->parameters[0].vector_value = glm::vec4( (color.r() * div), (color.g() * div), (color.b() * div), (color.a() * div) );


		if (lines.has_room(2, 0))
		{
			VertexType* v = (VertexType*)lines.request(2);
			
			v[0].position = glm::vec3(start.x, start.y, 0.0f);
			v[1].position = glm::vec3(end.x, end.y, 0.0f);
//			v[0].color = v[1].color = Color(255, 255, 255, 255);
		}
	
		this->render_buffer(lines, shader, solid_color);
	}
	
	virtual gui::TextureResult texture_create(const char* path, gui::TextureHandle& handle)
	{
		assets::Texture * tex = assets::textures()->load_from_path((char*)path);
		if ( !tex )
		{
			return gui::TextureResult_Failed;
		}
		
		handle = tex->Id();
		
		return gui::TextureResult_Success;
	}
	
	virtual void texture_destroy(const gui::TextureHandle& handle)
	{
		// nothing really to do in our system
	}
	
	virtual gui::TextureResult texture_info(const gui::TextureHandle& handle, uint32_t& width, uint32_t& height, uint8_t& channels)
	{
		assets::Texture * tex = assets::textures()->find_with_id( handle );
		if ( !tex )
		{
			return gui::TextureResult_Failed;
		}
		
		return gui::TextureResult_Success;
	}
	
	virtual gui::FontResult font_create(const char* path, gui::FontHandle& handle)
	{
		assets::Font* font = assets::fonts()->load_from_path((char*)path);
		if (font == 0)
		{
			return gui::FontResult_Failed;
		}
		
		handle = font->Id();
		
		return gui::FontResult_Success;
	}
	
	virtual void font_destroy(const gui::FontHandle& handle)
	{
		// nothing really to do in our system
	}
	
	virtual gui::FontResult font_measure_string(const gui::FontHandle& handle, const char* string, gui::Bounds& bounds)
	{
		assets::Font* font = assets::fonts()->find_with_id(handle);
		if (font)
		{
			unsigned int width = font::measure_width(font->handle, string);
			unsigned int height = font::measure_height(font->handle, string);
			bounds.set(0, 0, width, height);
			return gui::FontResult_Success;
		}
		
		return gui::FontResult_Failed;
	}
	
	virtual void font_draw(const gui::FontHandle& handle, const char* string, const gui::Bounds& bounds, const gui::Color& color)
	{
		assets::Font* font = assets::fonts()->find_with_id(handle);
		if (font)
		{
			font::draw_string(font->handle, bounds.origin.x, bounds.origin.y, string, Color(color.r(), color.g(), color.b(), color.a()));
		}
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
			
			
			camera.matCam = old_matcam;
			// draw debug graphics
			//			debugdraw::render(camera.matCam, camera.matProj, x, y, width, height);
		}
		
		camera.pos = old_camera_position;
		
		device->end_frame(driver);
	}
	
	virtual void render_viewmodel(gemini::IEngineEntity* entity, const kernel::Parameters& params, const glm::vec3& origin, const glm::vec2& view_angles)
	{
		
	}
	
	virtual void render_gui()
	{
		
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
	LOGV("TODO: implement entity removal\n");
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
		
	public:
	
		ModelInstanceData() : mesh_asset_index(0)
		{
		}
	
		void set_mesh_index(unsigned int mesh_asset_id)
		{
			mesh_asset_index = mesh_asset_id;
			mesh = assets::meshes()->find_with_id(mesh_asset_index);
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
	IPhysicsInterface* physics_interface;
	IExperimental* experimental_interface;
	
	SceneRenderMethod* render_method;
	Camera* camera;
	
public:

	EngineInterface(
		IEntityManager* em,
		IModelInterface* mi,
		IPhysicsInterface* pi,
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
	virtual IPhysicsInterface* physics() { return physics_interface; }
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
public:
	CustomListener(bool& in_gui) : is_in_gui(in_gui)
	{
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
					kernel::instance()->show_mouse(false);
					game_interface->level_load();
					break;
			}
		}
	}
};


class ProjectChimera : public kernel::IApplication,
public kernel::IEventListener<kernel::KeyboardEvent>,
public kernel::IEventListener<kernel::MouseEvent>,
public kernel::IEventListener<kernel::SystemEvent>,
public kernel::IEventListener<kernel::GameControllerEvent>
{
public:
	DECLARE_APPLICATION(ProjectChimera);

	Camera* active_camera;

	vr::HeadMountedDevice* device;

	renderer::SceneLink scenelink;
	SceneRenderMethod* render_method;
	

	Camera main_camera;
	
	audio::SoundHandle background;
	audio::SoundSource background_source;
	
	bool draw_physics_debug;

	platform::DynamicLibrary* gamelib;
	disconnect_engine_fn disconnect_engine;
	
	EntityManager entity_manager;
	ModelInterface model_interface;
	Experimental experimental;
		
	IEngineInterface* engine_interface;
	IGameInterface* game_interface;

//	core::RingBuffer<UserCommand, 32> client_commands;

	bool has_focus;
	
	GUIRenderer gui_renderer;
	gui::Compositor* compositor;
	bool in_gui;
	gui::Graph* graph;
	
	audio::SoundHandle menu_show;
	audio::SoundHandle menu_hide;
	
	
	gui::Panel* root;
	gui::Button* newgame;
	gui::Button* quit;
	
	CustomListener gui_listener;
	
public:
	ProjectChimera() : gui_listener(in_gui)
	{
		device = 0;
		render_method = 0;
		active_camera = &main_camera;
		draw_physics_debug = false;

		game_interface = 0;

		has_focus = true;
		
		compositor = 0;
		in_gui = true;
		graph = 0;
	}
	
	virtual ~ProjectChimera()
	{
		DESTROY(IEngineInterface, engine_interface);
	}
	
	virtual void event( kernel::KeyboardEvent & event )
	{
		if (event.is_down)
		{
			if (event.key == input::KEY_ESCAPE)
			{
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
				
				kernel::instance()->show_mouse(in_gui);
				
				root->set_visible(in_gui);
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

	virtual kernel::ApplicationResult config( kernel::Parameters& params )
	{
		params.window_title = "project_chimera";
		params.window_width = 1280;
		params.window_height = 720;

		vr::startup();
		
		// if there's a rift connected
		if ((REQUIRE_RIFT && (vr::total_devices() > 0)) || (!REQUIRE_RIFT))
		{
			// create one
			device = vr::create_device();
			if (device)
			{
				int32_t width, height;
				device->query_display_resolution(width, height);

				
				// required such that the kernel doesn't swap buffers
				// as this is handled by the rift sdk.
				params.swap_buffers = !RENDER_TO_VR;
				
				if (RENDER_TO_VR)
				{
//					params.use_fullscreen = 1;

					
#if PLATFORM_MACOSX
					// target the rift.
					if (REQUIRE_RIFT)
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
		
		return kernel::Application_Success;
	}

	void center_mouse(const kernel::Parameters& params)
	{
		if (has_focus && !in_gui)
		{
			kernel::instance()->warp_mouse(params.window_width/2, params.window_height/2);
		}
	}

	virtual kernel::ApplicationResult startup( kernel::Parameters& params )
	{
//		{
//			StateController sc;
//			
//			sc.add_state("mainmenu", new MainMenuState());
//			sc.add_state("ingame", new InGameMenuState());
//			sc.add_state("game", new GameState());
//			
//			
//			IGameState* mainmenu = sc.state_by_name("mainmenu");
//			mainmenu->activate();
//		}
	

		gui_listener.set_hover_sound(audio::create_sound("sounds/8b_select1"));
	
		menu_show = audio::create_sound("sounds/menu_show3");
		menu_hide = audio::create_sound("sounds/menu_hide");
	
		float camera_fov = 50.0f;
		if (device && RENDER_TO_VR)
		{
			vr::setup_rendering(device, params.render_width, params.render_height);
			render_method = CREATE(VRRenderMethod, device, scenelink);
		}
		else
		{
			render_method = CREATE(DefaultRenderMethod, scenelink);
		}
		
		engine_interface = CREATE(EngineInterface, &entity_manager, &model_interface, physics::api::instance(), &experimental, render_method, &main_camera);
		gemini::engine::api::set_instance(engine_interface);
		
//		background = audio::create_sound("sounds/8b_shoot");
//		background_source = audio::play(background, -1);

//		player_controller->camera = active_camera;
		active_camera->type = Camera::FIRST_PERSON;
		active_camera->move_speed = 0.1f;
		active_camera->perspective(camera_fov, params.render_width, params.render_height, 0.01f, 8192.0f);
		active_camera->set_absolute_position(glm::vec3(0, 4, 7));
		active_camera->pitch = 30;
		active_camera->update_view();
			
			
		compositor = new gui::Compositor(params.render_width, params.render_height);
		_compositor = compositor;
		compositor->set_renderer(&this->gui_renderer);
		compositor->set_listener(&gui_listener);
		
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
		
		// capture the mouse
//		kernel::instance()->capture_mouse( true );

		kernel::instance()->show_mouse(in_gui);

		// load the game library
		StackString<MAX_PATH_SIZE> game_library_path = ::core::filesystem::content_directory();


		const char* dynamiclibrary_extension = platform::instance()->get_dynamiclibrary_extension();

		game_library_path.append(PATH_SEPARATOR_STRING).append("bin").append(PATH_SEPARATOR_STRING).append("game").append(dynamiclibrary_extension);
		
		gamelib = platform::instance()->open_dynamiclibrary(game_library_path());
		
		if (gamelib == 0)
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
		
		gui_listener.set_game_interface(game_interface);
		gui_listener.set_root(root);

		center_mouse(params);

		return kernel::Application_Success;
	}

	virtual void step( kernel::Parameters& params )
	{
		// this is going to be incorrect unless this is placed in the step.
		// additionally, these aren't interpolated: figure how to; for example,
		// draw hit boxes for a moving player with this system.
		debugdraw::update(params.step_interval_seconds);
	}

	virtual void tick( kernel::Parameters& params )
	{
		if (graph)
		{
			graph->record_value(params.framedelta_raw_msec, 0);
		}
	
		if (compositor)
		{
			compositor->update(params.framedelta_raw_msec);
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
			
			int mouse[2];
			kernel::instance()->get_mouse_position(mouse[0], mouse[1]);
			
			int half_width = params.window_width/2;
			int half_height = params.window_height/2;
			
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
			
			
			center_mouse(params);
			
			if (game_interface)
			{
				// loop through all players and process inputs
				game_interface->process_commands(0, &command, 1);
				
				//			game_interface->physics_update(params.step_interval_seconds);
				//			background_source = audio::play(background, 1);
			}
		}
	
	
	
	
//		debugdraw::axes(glm::mat4(1.0), 1.0f);
		int x = 10;
		int y = 10;
		
		if (active_camera)
		{
			debugdraw::text(x, y, core::str::format("active_camera->pos = %.2g %.2g %.2g", active_camera->pos.x, active_camera->pos.y, active_camera->pos.z), Color(255, 255, 255));
			debugdraw::text(x, y+12, core::str::format("eye_position = %.2g %.2g %.2g", active_camera->eye_position.x, active_camera->eye_position.y, active_camera->eye_position.z), Color(255, 0, 255));
			debugdraw::text(x, y+24, core::str::format("active_camera->view = %.2g %.2g %.2g", active_camera->view.x, active_camera->view.y, active_camera->view.z), Color(128, 128, 255));
			debugdraw::text(x, y+36, core::str::format("active_camera->right = %.2g %.2g %.2g", active_camera->side.x, active_camera->side.y, active_camera->side.z), Color(255, 0, 0));
		}
		debugdraw::text(x, y+48, core::str::format("frame delta = %2.2fms\n", params.framedelta_raw_msec), Color(255, 255, 255));
		debugdraw::text(x, y+60, core::str::format("# allocations = %i, total %i Kbytes\n", platform::memory::allocator().active_allocations(), platform::memory::allocator().active_bytes()/1024), Color(64, 102, 192));
		
		
		if (draw_physics_debug)
		{
			physics::debug_draw();
		}

		
		// run server frame
		if (game_interface)
		{
			float framedelta_seconds = params.framedelta_raw_msec*0.001f;
			game_interface->server_frame(params.current_tick, framedelta_seconds, params.step_alpha);
						
			game_interface->client_frame(framedelta_seconds, params.step_alpha);
		}
		

	
		if (device)
		{
			glm::mat4 xform;
			device->test(xform);
			debugdraw::axes(xform, 2.0f, 0.1f);
		}
		
		//glm::mat4 char_mat = glm::mat4(1.0);
		// TODO: this should use the actual player height instead of
		// hard coding the value.
//		char_mat = glm::translate(active_camera->pos - glm::vec3(0,1.82,0));
// needs to be RADIANS now
//		char_mat = glm::rotate(char_mat, -active_camera->yaw, glm::vec3(0,1,0));
		//if (player)
		{
			//player->world_transform = char_mat;
		}
	}
	
	virtual void shutdown( kernel::Parameters& params )
	{
		delete compositor;
		compositor = 0;
	
		DESTROY(SceneRenderMethod, render_method);
		
		if (device)
		{
			vr::destroy_device(device);
		}
		vr::shutdown();
		
		if (game_interface)
		{
			game_interface->shutdown();
		}
		
		if (disconnect_engine)
		{
			disconnect_engine();
		}
		
		platform::instance()->close_dynamiclibrary(gamelib);
	}
};

IMPLEMENT_APPLICATION( ProjectChimera );