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
#include "physics.h"
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
	
	renderer::RenderSettings render_settings;
	
	Settings()
	{
		// setup sane defaults
		physics_tick_rate = 60;
		debugdraw_max_primitives = 2048;
		enable_asset_reloading = 0;
		vr_render = false;
		vr_require_headset = false;
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
	
	const Json::Value& renderer = root["renderer"];
	if (!renderer.isNull())
	{
		// load renderer settings
		result = load_render_config(renderer, &cfg->render_settings);
	}
	
	return result;
}



#include <SDL.h>





#include <nom/nom.hpp>
#include <nom/compositor.hpp>
#include <nom/graph.hpp>
#include <nom/button.hpp>

// this is required at the moment because our render method needs it!
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
			if (!mesh->skeleton.empty())
			{
				bone_transforms = new glm::mat4[mesh->animation.total_bones];
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
		
		
		virtual void get_animation_pose(int32_t index, glm::vec3* positions, glm::quat* rotations, float t)
		{
			if (mesh->skeleton.empty())
			{
				return;
			}
			
			mesh->animation.get_pose(positions, rotations, t);
		}
		
		virtual void set_pose(glm::vec3* positions, glm::quat* rotations)
		{
			if (mesh->skeleton.empty())
			{
				return;
			}
			
			if (mesh->animation.total_bones == 0)
			{
				return;
			}
			
			// You've hit the upper bounds for skeletal bones for a single
			// model. Congrats.
			assert(mesh->skeleton.size() < MAX_BONES);
			
			// recalculate
			for (size_t index = 0; index < mesh->animation.total_bones; ++index)
			{
				assets::Joint* joint = &mesh->skeleton[index];
				glm::mat4& global_pose = bone_transforms[index];
				glm::mat4& saved_pose = mesh->animation.transforms[index];
				
				glm::mat4 local_scale;
				glm::mat4 local_rotation = glm::toMat4(rotations[index]);
				glm::mat4 local_transform = glm::translate(glm::mat4(1.0f), positions[index]);
				glm::mat4 local_pose = local_transform * local_rotation * local_scale;
				//				local_to_world = tr * pivot * ro * sc * inv_pivot;
				
				if (joint->parent_index > -1)
				{
					const glm::mat4& parent_transform = mesh->animation.transforms[joint->parent_index];
					saved_pose = parent_transform * local_pose;
				}
				else
				{
					saved_pose = local_pose;
				}
				
				global_pose = saved_pose * joint->inverse_bind_matrix;
			}
		}
		
		virtual int32_t get_animation_index(const char* name)
		{
			return animation::find_sequence(name);
		}
		
		virtual int32_t add_animation(const char* name)
		{
			animation::SequenceId id = animation::load_sequence(name);
			animations.push_back(id);
			return id;
		}
		
		virtual int32_t get_total_animations() const
		{
			//			return animations.size();
			return 1;
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
		
		image::Image image;
		image.type = image::TEX_CUBE;
		image.width = 128;
		image.height = 128;
		
		cubemap_texture = driver->texture_create(image);
		
		
		
		
		cubemap = driver->render_target_create(image.width, image.height);
		driver->render_target_set_attachment(cubemap, renderer::RenderTarget::COLOR, 0, cubemap_texture);
	}
	
	void shutdown()
	{
		renderer::IRenderDriver* driver = renderer::driver();
		driver->render_target_destroy(cubemap);
		
		
		driver->texture_destroy(cubemap_texture);
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
					kernel::instance()->show_mouse(false);
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
	
	// SDL related items
	SDL_Window* window;
	SDL_GLContext context;
	SDL_Rect* display_rects;
	uint8_t total_displays;
	uint8_t total_controllers;
	typedef CustomPlatformAllocator<std::pair<const unsigned int, input::Button> > ButtonKeyMapAllocator;
	typedef std::map<unsigned int, input::Button, std::less<unsigned int>, ButtonKeyMapAllocator> SDLToButtonKeyMap;
	SDLToButtonKeyMap key_map;
	input::MouseButton mouse_map[input::MOUSE_COUNT];
	SDL_GameController* controllers[input::MAX_JOYSTICKS];
		
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
	bool sdl_startup()
	{
		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC) == -1)
		{
			// failure!
			fprintf(stdout, "failure to init SDL\n");
		}

		total_displays = SDL_GetNumVideoDisplays();
		fprintf(stdout, "Found %i total displays.\n", total_displays);
		
		display_rects = CREATE_ARRAY(SDL_Rect, total_displays);
		for (int index = 0; index < total_displays; ++index)
		{
			SDL_DisplayMode current;
			SDL_GetCurrentDisplayMode(index, &current);
			fprintf(stdout, "%i) width: %i, height: %i, refresh_rate: %iHz\n", index, current.w, current.h, current.refresh_rate);
			
			// cache display bounds
			SDL_GetDisplayBounds(index, &display_rects[index]);
		}
		
		
		if (kernel::parameters().use_vsync)
		{
			SDL_GL_SetSwapInterval(1);
		}
		
#if defined(PLATFORM_MOBILE)
#else
		kernel::parameters().device_flags |= kernel::DeviceDesktop;
#endif
		
		//
		// setup input mappings

		// populate the keyboard map

		key_map[SDLK_a] = KEY_A;
		key_map[SDLK_b] = KEY_B;
		key_map[SDLK_c] = KEY_C;
		key_map[SDLK_d] = KEY_D;
		key_map[SDLK_e] = KEY_E;
		key_map[SDLK_f] = KEY_F;
		key_map[SDLK_g] = KEY_G;
		key_map[SDLK_h] = KEY_H;
		key_map[SDLK_i] = KEY_I;
		key_map[SDLK_j] = KEY_J;
		key_map[SDLK_k] = KEY_K;
		key_map[SDLK_l] = KEY_L;
		key_map[SDLK_m] = KEY_M;
		key_map[SDLK_n] = KEY_N;
		key_map[SDLK_o] = KEY_O;
		key_map[SDLK_p] = KEY_P;
		key_map[SDLK_q] = KEY_Q;
		key_map[SDLK_r] = KEY_R;
		key_map[SDLK_s] = KEY_S;
		key_map[SDLK_t] = KEY_T;
		key_map[SDLK_u] = KEY_U;
		key_map[SDLK_v] = KEY_V;
		key_map[SDLK_w] = KEY_W;
		key_map[SDLK_y] = KEY_Y;
		key_map[SDLK_x] = KEY_X;
		key_map[SDLK_z] = KEY_Z;
		key_map[SDLK_MENU] = KEY_MENU;
		key_map[SDLK_SEMICOLON] = KEY_SEMICOLON;
		key_map[SDLK_SLASH] = KEY_SLASH;
		key_map[SDLK_BACKSLASH] = KEY_BACKSLASH;
		key_map[SDLK_EQUALS] = KEY_EQUALS;
		key_map[SDLK_MINUS] = KEY_MINUS;
		key_map[SDLK_LEFTBRACKET] = KEY_LBRACKET;
		key_map[SDLK_RIGHTBRACKET] = KEY_RBRACKET;
		key_map[SDLK_COMMA] = KEY_COMMA;
		key_map[SDLK_PERIOD] = KEY_PERIOD;
		key_map[SDLK_QUOTE] = KEY_QUOTE;
		key_map[SDLK_ESCAPE] = KEY_ESCAPE;
		key_map[SDLK_SPACE] = KEY_SPACE;
		key_map[SDLK_RETURN] = KEY_RETURN;
		key_map[SDLK_BACKSPACE] = KEY_BACKSPACE;
		key_map[SDLK_TAB] = KEY_TAB;
		key_map[SDLK_PAGEUP] = KEY_PAGEUP;
		key_map[SDLK_PAGEDOWN] = KEY_PAGEDN;
		key_map[SDLK_END] = KEY_END;
		key_map[SDLK_HOME] = KEY_HOME;
		key_map[SDLK_INSERT] = KEY_INSERT;
		key_map[SDLK_DELETE] = KEY_DELETE;
		key_map[SDLK_PAUSE] = KEY_PAUSE;
		key_map[SDLK_LSHIFT] = KEY_LSHIFT;
		key_map[SDLK_RSHIFT] = KEY_RSHIFT;
		key_map[SDLK_LCTRL] = KEY_LCONTROL;
		key_map[SDLK_RCTRL] = KEY_RCONTROL;
		key_map[SDLK_LALT] = KEY_LALT;
		key_map[SDLK_RALT] = KEY_RALT;
		key_map[SDLK_NUMLOCKCLEAR] = KEY_NUMLOCK;
		key_map[SDLK_CAPSLOCK] = KEY_CAPSLOCK;
		key_map[SDLK_LGUI] = KEY_LGUI;
		key_map[SDLK_0] = KEY_0;
		key_map[SDLK_1] = KEY_1;
		key_map[SDLK_2] = KEY_2;
		key_map[SDLK_3] = KEY_3;
		key_map[SDLK_4] = KEY_4;
		key_map[SDLK_5] = KEY_5;
		key_map[SDLK_6] = KEY_6;
		key_map[SDLK_7] = KEY_7;
		key_map[SDLK_8] = KEY_8;
		key_map[SDLK_9] = KEY_9;
		key_map[SDLK_F1] = KEY_F1;
		key_map[SDLK_F2] = KEY_F2;
		key_map[SDLK_F3] = KEY_F3;
		key_map[SDLK_F4] = KEY_F4;
		key_map[SDLK_F5] = KEY_F5;
		key_map[SDLK_F6] = KEY_F6;
		key_map[SDLK_F7] = KEY_F7;
		key_map[SDLK_F8] = KEY_F8;
		key_map[SDLK_F9] = KEY_F9;
		key_map[SDLK_F10] = KEY_F10;
		key_map[SDLK_F11] = KEY_F11;
		key_map[SDLK_F12] = KEY_F12;
		key_map[SDLK_F13] = KEY_F13;
		key_map[SDLK_F14] = KEY_F14;
		key_map[SDLK_F15] = KEY_F15;
		key_map[SDLK_LEFT] = KEY_LEFT;
		key_map[SDLK_RIGHT] = KEY_RIGHT;
		key_map[SDLK_UP] = KEY_UP;
		key_map[SDLK_DOWN] = KEY_DOWN;
		key_map[SDLK_KP_0] = KEY_NUMPAD0;
		key_map[SDLK_KP_1] = KEY_NUMPAD1;
		key_map[SDLK_KP_2] = KEY_NUMPAD2;
		key_map[SDLK_KP_3] = KEY_NUMPAD3;
		key_map[SDLK_KP_4] = KEY_NUMPAD4;
		key_map[SDLK_KP_5] = KEY_NUMPAD5;
		key_map[SDLK_KP_6] = KEY_NUMPAD6;
		key_map[SDLK_KP_7] = KEY_NUMPAD7;
		key_map[SDLK_KP_8] = KEY_NUMPAD8;
		key_map[SDLK_KP_9] = KEY_NUMPAD9;
		key_map[SDLK_KP_PLUS] = KEY_NUMPAD_PLUS;
		key_map[SDLK_KP_MINUS] = KEY_NUMPAD_MINUS;
		key_map[SDLK_KP_PLUSMINUS] = KEY_NUMPAD_PLUSMINUS;
		key_map[SDLK_KP_MULTIPLY] = KEY_NUMPAD_MULTIPLY;
		key_map[SDLK_KP_DIVIDE] = KEY_NUMPAD_DIVIDE;
		
		// populate the mouse map
		mouse_map[SDL_BUTTON_LEFT] = MOUSE_LEFT;
		mouse_map[SDL_BUTTON_RIGHT] = MOUSE_RIGHT;
		mouse_map[SDL_BUTTON_MIDDLE] = MOUSE_MIDDLE;
		mouse_map[SDL_BUTTON_X1] = MOUSE_MOUSE4;
		mouse_map[SDL_BUTTON_X2] = MOUSE_MOUSE5;

		return true;
	} // sdl_startup
	
	
	void sdl_shutdown()
	{
		DESTROY_ARRAY(SDL_Rect, display_rects, total_displays);
		
		// close all controllers
		for (uint8_t i = 0; i < total_controllers; ++i)
		{
			input::JoystickInput& js = input::state()->joystick(i);
			input::state()->disconnect_joystick(i);
			
			SDL_GameController* controller = controllers[i];
			if (controller)
			{
				SDL_GameControllerClose(controller);
				controllers[i] = 0;
			}
		}
		
		key_map.clear();
		
		
		SDL_GL_DeleteContext(context);
		SDL_DestroyWindow(window);
		SDL_Quit();
	} // sdl_shutdown
	
	void create_window()
	{
		int window_width, window_height;
		int render_width, render_height;
		
		if ( is_active() )
		{
			assert( kernel::parameters().window_width != 0 || kernel::parameters().window_height != 0 );
			
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
			SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
			
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
			SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
			
			uint32_t window_flags = 0;
			window_flags |= SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
			
			if (kernel::parameters().use_fullscreen)
			{
				window_flags |= SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS;
			}
			else
			{
				window_flags |= SDL_WINDOW_RESIZABLE;
			}
			
			window = SDL_CreateWindow(
											 kernel::parameters().window_title, 0, 0,
											 kernel::parameters().window_width, kernel::parameters().window_height,
											 window_flags);
			
			if (!window)
			{
				LOGE("Failed to create SDL window: %s\n", SDL_GetError());
			}
			
			// move the window to the correct display
			SDL_SetWindowPosition(window, display_rects[kernel::parameters().target_display].x, display_rects[kernel::parameters().target_display].y);
			
			context = SDL_GL_CreateContext(window);
			if (!context)
			{
				LOGE("Failed to create SDL GL context: %s\n", SDL_GetError());
			}
			
			// try to set our window size; it might still be smaller than requested.
			SDL_SetWindowSize(window, kernel::parameters().window_width, kernel::parameters().window_height);
			
			// center our window
			SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
			
			// fetch the window size and renderable size
			SDL_GetWindowSize(window, &window_width, &window_height);
			SDL_GL_GetDrawableSize(window, &render_width, &render_height);
			
			// hide the mouse cursor
			show_mouse(false);
			
			kernel::parameters().window_width = window_width;
			kernel::parameters().window_height = window_height;
			kernel::parameters().render_width = render_width;
			kernel::parameters().render_height = render_height;
			
			if ( render_width > window_width && render_height > window_height )
			{
				LOGV( "Retina display detected. Render Resolution is (%i x %i)\n", render_width, render_height );
				kernel::parameters().device_flags |= kernel::DeviceSupportsRetinaDisplay;
			}
			else
			{
				LOGV( "window resolution %i x %i\n", window_width, window_height );
				LOGV( "render resolution %i x %i\n", render_width, render_height );
			}
		}
	} // create_window

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
			kernel::instance()->warp_mouse(params.window_width/2, params.window_height/2);
		}
	}
	
public:
	EngineKernel() :
		active(true),
		window(0),
		context(0),
		display_rects(0),
		total_displays(0),
		total_controllers(0),
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
		game_path = "/Users/apetrone/Documents/games/vrpowergrid";
		device = 0;
		
		memset(mouse_map, 0, sizeof(input::MouseButton)*input::MOUSE_COUNT);
		memset(controllers, 0, sizeof(SDL_GameController*)*input::MAX_JOYSTICKS);
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
	
	
	
	
	
	
	
	
	
	
	
	void sdl_setup_joysticks()
	{
		// add game controller db
		size_t length = 0;
		char* buffer = core::filesystem::file_to_buffer("conf/gamecontrollerdb.conf", 0, &length);
		int result = SDL_GameControllerAddMapping(buffer);
		DEALLOC(buffer);
		
		// If you hit this assert, there was an error laoding the gamecontrollerdb.
		// Otherwise, it was either added (result == 1) or updated (result == 0).
		assert(result != -1);
		
		
		fprintf(stdout, "Gather joystick infos...\n");
		fprintf(stdout, "Num Haptics: %i\n", SDL_NumHaptics());
		fprintf(stdout, "Num Joysticks: %i\n", SDL_NumJoysticks());
		
		
		assert(SDL_NumJoysticks() < input::MAX_JOYSTICKS);
		total_controllers = SDL_NumJoysticks();
		for (uint8_t i = 0; i < total_controllers; ++i)
		{
			input::JoystickInput& js = input::state()->joystick(i);
			input::state()->connect_joystick(i);
			js.reset();
			
			controllers[i] = SDL_GameControllerOpen(i);
			if (SDL_IsGameController(i))
			{
				fprintf(stdout, "Found compatible controller: \"%s\"\n", SDL_GameControllerNameForIndex(i));
				//			fprintf(stdout, "Mapped as: \"%s\"\n", SDL_GameControllerMapping(state->controllers[i]));
				
				SDL_Joystick * joystick = SDL_GameControllerGetJoystick(controllers[i]);
				SDL_JoystickID joystickID = SDL_JoystickInstanceID( joystick );
				if (SDL_JoystickIsHaptic(joystick))
				{
					js.flags |= input::JoystickInput::HapticSupport;
					fprintf(stdout, "Joystick is haptic!\n");
					//			http://blog.5pmcasual.com/game-controller-api-in-sdl2.html
					SDL_Haptic * haptic = SDL_HapticOpenFromJoystick( joystick );
					if (haptic)
					{
						SDL_HapticRumbleInit(haptic);
						//				SDL_HapticRumblePlay(haptic, 1.0, 2000);
						
						//				SDL_Delay(2000);
						SDL_HapticClose(haptic);
					}
					else
					{
						fprintf(stdout, "error opening haptic for joystickID: %i\n", joystickID);
					}
				}
			}
			else
			{
				fprintf(stderr, "GameController at index %i, is not a compatible controller.\n", i);
			}
		}
	}
	
	
	void setup_gui(const kernel::Parameters& params)
	{
		
		compositor = new gui::Compositor(params.render_width, params.render_height);
		_compositor = compositor;
		
		gui_renderer = CREATE(GUIRenderer);
		compositor->set_renderer(gui_renderer);
		
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
		const char FONT_SHADER[] = "shaders/fontshader";
		const char DEBUG_FONT[] = "fonts/debug";
		const char DEBUG_SHADER[] = "shaders/debug";
		kernel::Parameters& params = kernel::parameters();
	
		// initialize timer
		last_time = platform::instance()->get_time_microseconds();
	
	
		sdl_startup();
	
	
	
	
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
		
		
		sdl_setup_joysticks();
		
		// load engine settings
		// load boot config
		Settings config;
		load_config(config);
		
		params.step_interval_seconds = (1.0f/(float)config.physics_tick_rate);
		
		// TODO: we should load these from a config; for now just set them.
		params.window_width = 1280;
		params.window_height = 720;
		params.window_title = "gemini";
		
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
		
		// TODO: post application config; create the window
		create_window();

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
		
		kernel::instance()->show_mouse(in_gui);
		
		
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
	
	
	void controller_axis_event(SDL_ControllerDeviceEvent& device, SDL_ControllerAxisEvent& axis)
	{
		LOGV("Axis Motion: %i, %i, %i, %i\n", device.which, axis.which, axis.axis, axis.value);
	}
	
	void controller_button_event(SDL_ControllerDeviceEvent& device, SDL_ControllerButtonEvent& button)
	{
		bool is_down = (button.state == SDL_PRESSED);
		LOGV("Button %s: %i, %i, %i\n", (is_down ? "Yes" : "No"), device.which, button.button, button.state);
	}
	
	void pre_tick()
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			input::Button button;
			
			// dispatch event!
			switch(event.type)
			{
				case SDL_QUIT:
					kernel::instance()->set_active(false);
					break;
					
				case SDL_TEXTINPUT:
				{
//					LOGV("TODO: add unicode support from SDL: %s\n", event.text.text);
					break;
				}
					
				case SDL_KEYUP:
				case SDL_KEYDOWN:
				{
					button = key_map[event.key.keysym.sym];
					
					if (event.key.repeat)
					{
						break;
					}
					
					//printf( "\t-> key: %i (%s)\n", e->key, xwl_key_to_string(e->key) );
					kernel::KeyboardEvent ev;
					ev.is_down = (event.type == SDL_KEYDOWN);
					ev.key = button;
					input::state()->keyboard().inject_key_event(button, ev.is_down);
					kernel::event_dispatch(ev);
					break;
				}
					
				case SDL_MOUSEBUTTONUP:
				case SDL_MOUSEBUTTONDOWN:
				{
					kernel::MouseEvent ev;
					ev.subtype = kernel::MouseButton;
					ev.button = mouse_map[event.button.button];
					ev.is_down = (event.type == SDL_MOUSEBUTTONDOWN);
					input::state()->mouse().inject_mouse_button((input::MouseButton)ev.button, ev.is_down);
					kernel::event_dispatch(ev);
					break;
				}
					
				case SDL_MOUSEMOTION:
				{
					kernel::MouseEvent ev;
					ev.subtype = kernel::MouseMoved;
					ev.dx = event.motion.xrel;
					ev.dy = event.motion.yrel;
					ev.mx = event.motion.x;
					ev.my = event.motion.y;
					input::state()->mouse().inject_mouse_move(ev.mx, ev.my);
					kernel::event_dispatch(ev);
					break;
				}
					
				case SDL_MOUSEWHEEL:
				{
					kernel::MouseEvent ev;
					ev.subtype = kernel::MouseWheelMoved;
					ev.wheel_direction = event.wheel.y;
					input::state()->mouse().inject_mouse_wheel(ev.wheel_direction);
					kernel::event_dispatch(ev);
					break;
				}
					
				case SDL_CONTROLLERAXISMOTION:
				{
					input::JoystickInput& joystick = input::state()->joystick(event.cdevice.which);
					input::AxisState& axis = joystick.axes[event.caxis.axis];
					axis.value = event.caxis.value;
					axis.normalized_value = (event.caxis.value/(float)SHRT_MAX);
					
					// check for values outside the deadzone
					if (event.caxis.value > 3200 || event.caxis.value < -3200)
					{
						kernel::GameControllerEvent ev;
						ev.gamepad_id = event.cdevice.which;
						ev.subtype = kernel::JoystickAxisMoved;
						ev.joystick_id = event.caxis.axis;
						ev.joystick_value = event.caxis.value;
						kernel::event_dispatch(ev);
					}
					else
					{
						axis.value = 0;
						axis.normalized_value = 0;
					}
					break;
				}
					
				case SDL_CONTROLLERBUTTONDOWN:
				{
					controller_button_event(event.cdevice, event.cbutton);
					
					kernel::GameControllerEvent ev;
					ev.gamepad_id = event.cdevice.which;
					ev.subtype = kernel::JoystickButton;
					ev.is_down = true;
					ev.button = event.cbutton.button;
					kernel::event_dispatch(ev);
					break;
				}
					
				case SDL_CONTROLLERBUTTONUP:
				{
					controller_button_event(event.cdevice, event.cbutton);
					
					kernel::GameControllerEvent ev;
					ev.gamepad_id = event.cdevice.which;
					ev.subtype = kernel::JoystickButton;
					ev.is_down = false;
					ev.button = event.cbutton.button;
					kernel::event_dispatch(ev);
					break;
				}
					
				case SDL_CONTROLLERDEVICEADDED:
				{
					// event 'which' member
					// describes an index into the list of active devices; NOT joystick id.
					LOGV("Device Added: %i\n", event.cdevice.which);
					
					input::JoystickInput& js = input::state()->joystick(event.cdevice.which);
					js.reset();
					input::state()->connect_joystick(event.cdevice.which);
					
					
					controllers[event.cdevice.which] = SDL_GameControllerOpen(event.cdevice.which);
					SDL_Joystick * joystick = SDL_GameControllerGetJoystick(controllers[event.cdevice.which]);
					
					
					kernel::GameControllerEvent ev;
					ev.gamepad_id = event.cdevice.which;
					ev.subtype = kernel::JoystickConnected;
					kernel::event_dispatch(ev);
					break;
				}
					
				case SDL_CONTROLLERDEVICEREMOVED:
				{
					LOGV("Device Removed: %i\n", event.cdevice.which);
					
					input::state()->disconnect_joystick(event.cdevice.which);
					
					SDL_GameControllerClose(controllers[event.cdevice.which]);
					controllers[event.cdevice.which] = 0;
					
					
					kernel::GameControllerEvent ev;
					ev.gamepad_id = event.cdevice.which;
					ev.subtype = kernel::JoystickDisconnected;
					kernel::event_dispatch(ev);
					break;
				}
					
					// handle window events
				case SDL_WINDOWEVENT:
				{
					switch (event.window.event)
					{
						case SDL_WINDOWEVENT_FOCUS_LOST:
						{
							kernel::Event<kernel::System> event;
							event.subtype = kernel::WindowLostFocus;
							kernel::event_dispatch(event);
							break;
						}
							
						case SDL_WINDOWEVENT_FOCUS_GAINED:
						{
							kernel::Event<kernel::System> event;
							event.subtype = kernel::WindowGainFocus;
							kernel::event_dispatch(event);
							break;
						}
					}
				}
			}
		}
	} // pre_tick
	
	virtual void tick()
	{
		update();
	
		audio::update();
		input::update();
		animation::update(kernel::parameters().framedelta_filtered_seconds);
		pre_tick();
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
			kernel::instance()->get_mouse_position(mouse[0], mouse[1]);
			
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
			SDL_GL_SwapWindow(window);
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
	
		sdl_shutdown();
		
		DESTROY(IEngineInterface, engine_interface);
		DESTROY(SceneLink, scenelink);
	}
	
	virtual void capture_mouse(bool capture)
	{
		SDL_bool is_enabled = capture ? SDL_TRUE : SDL_FALSE;
		SDL_SetRelativeMouseMode(is_enabled);
	}
	
	virtual void warp_mouse(int x, int y)
	{
		SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
		SDL_WarpMouseInWindow(window, x, y);
		SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);
	}
	
	virtual void get_mouse_position(int& x, int& y)
	{
		SDL_GetMouseState(&x, &y);
	}
	
	virtual void show_mouse(bool show)
	{
		SDL_ShowCursor((show ? SDL_TRUE : SDL_FALSE));
	}
};



PLATFORM_MAIN
{
	int return_code;
	return_code = platform::run_application(new EngineKernel());
	return return_code;
}