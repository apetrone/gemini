// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM,OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
// -------------------------------------------------------------
#include "kernel.h"

#include <platform/platform.h>

#include <stdio.h>
#include <core/logging.h>
#include <core/mathlib.h>
#include <renderer/debugdraw.h>
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


#include <core/filesystem.h>
#include <core/logging.h>

#include <sdk/entity_api.h>
#include <sdk/iengineentity.h>
#include <sdk/model_api.h>
#include <sdk/engine_api.h>
#include <sdk/game_api.h>
#include <sdk/experimental_api.h>
#include <sdk/shared.h>

#include <platform/mem.h>

using namespace gemini;
using namespace gemini::physics;

#define LOCK_CAMERA_TO_CHARACTER 1

// even if a VR device is attached, this will NOT render to it
// this allows debugging in some other mechanism to check sensor data.
uint8_t RENDER_TO_VR = 1;

uint8_t REQUIRE_RIFT = 1;

const uint16_t MAX_ENTITIES = 2048;


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
	scenelink.queue_entities(cb, entity_list, MAX_ENTITIES);
	scenelink.sort();
	scenelink.draw(cb);
}

void render_scene_from_entity(gemini::IEngineEntity** entity_list, const glm::mat4& modelview, const glm::mat4& projection, renderer::SceneLink& scenelink)
{
	// setup constant buffer
	glm::vec3 light_position;
//	light_position = camera.pos + -camera.view + glm::vec3(0.0f, 1.0f, 0.0f);
	light_position = glm::vec3(0, 2.0f, 0);
	
	glm::vec3 viewer_dir;
	glm::vec3 viewer_pos;
	
	renderer::ConstantBuffer cb;
	cb.modelview_matrix = &modelview;
	cb.projection_matrix = &projection;
	cb.viewer_direction = &viewer_dir;
	cb.viewer_position = &viewer_pos;
	cb.light_position = &light_position;
	
	// use the entity list to render
	scenelink.clear();
	scenelink.queue_entities(cb, entity_list, MAX_ENTITIES);
	scenelink.sort();
	scenelink.draw(cb);
}

class SceneRenderMethod
{
public:
	virtual ~SceneRenderMethod() {}
	virtual void render_frame(gemini::IEngineEntity** entity_list, Camera& camera, const kernel::Params& params ) = 0;
	virtual void render_view(gemini::IEngineEntity** entity_list, const kernel::Params& params, const glm::vec3& origin, const glm::vec2& view_angles) = 0;
};


class VRRenderMethod : public SceneRenderMethod
{
	vr::HeadMountedDevice* device;
	
	renderer::SceneLink& scenelink;

public:
	VRRenderMethod(vr::HeadMountedDevice* in_device, renderer::SceneLink& in_link) : device(in_device), scenelink(in_link) {}
	
	virtual void render_frame(gemini::IEngineEntity** entity_list, Camera& camera, const kernel::Params& params )
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
	
	virtual void render_view(gemini::IEngineEntity** entity_list, const kernel::Params& params, const glm::vec3& origin, const glm::vec2& view_angles)
	{
		
	}
};


class DefaultRenderMethod : public SceneRenderMethod
{
	renderer::SceneLink& scenelink;
public:
	DefaultRenderMethod(renderer::SceneLink& in_link) : scenelink(in_link) {};

	virtual void render_frame(gemini::IEngineEntity** entity_list, Camera& camera, const kernel::Params& params )
	{
		RenderStream rs;
		rs.add_cullmode(renderer::CullMode::CULLMODE_BACK);
//		rs.add_state(renderer::STATE_BACKFACE_CULLING, 0);
		rs.add_state(renderer::STATE_DEPTH_TEST, 1);
		rs.add_viewport( 0, 0, params.render_width, params.render_height );
		rs.add_clearcolor( 0.0, 0.0, 0.0, 1.0f );
		rs.add_clear( renderer::CLEAR_COLOR_BUFFER | renderer::CLEAR_DEPTH_BUFFER );
		rs.run_commands();
		
		render_scene_from_camera(entity_list, camera, scenelink);
		
		// draw debug graphics
		{
			debugdraw::render(camera.matCam, camera.matProj, 0, 0, params.render_width, params.render_height);
		}
	}

	virtual void render_view(gemini::IEngineEntity** entity_list, const kernel::Params& params, const glm::vec3& origin, const glm::vec2& view_angles)
	{
	
		RenderStream rs;
		rs.add_cullmode(renderer::CullMode::CULLMODE_BACK);
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
	
		{
			debugdraw::render(camera.matCam, camera.matProj, 0, 0, params.render_width, params.render_height);
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
	virtual gemini::core::logging::ILog* log() { return gemini::core::log::instance(); }
	
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
		render_method->render_view(em->get_entity_list(), kernel::instance()->parameters(), origin, view_angles);
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

public:
	ProjectChimera()
	{
		device = 0;
		render_method = 0;
		active_camera = &main_camera;
		draw_physics_debug = true;

		game_interface = 0;
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
				kernel::instance()->set_active(false);
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
			}
		}
	}
	
	virtual void event( kernel::MouseEvent & event )
	{
		switch(event.subtype)
		{
			case kernel::MouseMoved:
			{
				if (active_camera)
				{
					active_camera->move_view(event.dx, event.dy);
				}
				//camera.update_view();
				break;
			}
			
			case kernel::WindowLostFocus:
			case kernel::WindowGainFocus:
			case kernel::WindowResized:
			case kernel::MouseButton:
//			{
//				if (event.is_down && active_camera)
//				{
//					// try to raycast?
//					glm::vec3 start, direction;
//					physics::CollisionObject* char_object = (physics::CollisionObject*)character->getGhostObject()->getUserPointer();
//					physics::raycast(char_object, active_camera->pos, active_camera->view, 4096.0f);
//				}
//				break;
//			}
			case kernel::MouseWheelMoved:
			case kernel::TouchBegin:
			case kernel::TouchMoved:
			case kernel::TouchEnd:
			default: break;
		}
//		switch( event.subtype )
//		{
//			case kernel::MouseMoved:
//			{
//				if ( input::state()->mouse().is_down( input::MOUSE_LEFT ) )
//				{
//					int lastx, lasty;
//					input::state()->mouse().last_mouse_position( lastx, lasty );
//					
//					camera.move_view( event.mx-lastx, event.my-lasty );
//				}
//                break;
//			}
//			default: break;
//		}
	}
	
	virtual void event( kernel::SystemEvent & event )
	{
		if (event.subtype == kernel::WindowLostFocus)
		{
			kernel::instance()->capture_mouse(false);
		}
		else if (event.subtype == kernel::WindowGainFocus)
		{
			kernel::instance()->capture_mouse(true);
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

	virtual kernel::ApplicationResult config( kernel::Params & params )
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

	virtual kernel::ApplicationResult startup( kernel::Params & params )
	{
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
			
		// capture the mouse
		kernel::instance()->capture_mouse( true );

		// load the game library
		StackString<MAX_PATH_SIZE> game_library_path = ::core::filesystem::content_directory();
		game_library_path.append(PATH_SEPARATOR_STRING).append("bin").append(PATH_SEPARATOR_STRING).append("game.dylib");
		
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
			
			
			game_interface->level_load();
		}

		return kernel::Application_Success;
	}

	virtual void step( kernel::Params & params )
	{
		if (game_interface)
		{
			game_interface->physics_update(params.step_interval_seconds);
//			background_source = audio::play(background, 1);
		}

		if (draw_physics_debug)
		{
			physics::debug_draw();
		}

		debugdraw::axes(glm::mat4(1.0), 1.0f);
		int x = 10;
		int y = params.render_height - 50 - params.titlebar_height;
		if (active_camera)
		{
			debugdraw::text(x, y, core::str::format("active_camera->pos = %.2g %.2g %.2g", active_camera->pos.x, active_camera->pos.y, active_camera->pos.z), Color(255, 255, 255));
			debugdraw::text(x, y+12, core::str::format("eye_position = %.2g %.2g %.2g", active_camera->eye_position.x, active_camera->eye_position.y, active_camera->eye_position.z), Color(255, 0, 255));
			debugdraw::text(x, y+24, core::str::format("active_camera->view = %.2g %.2g %.2g", active_camera->view.x, active_camera->view.y, active_camera->view.z), Color(128, 128, 255));
			debugdraw::text(x, y+36, core::str::format("active_camera->right = %.2g %.2g %.2g", active_camera->side.x, active_camera->side.y, active_camera->side.z), Color(255, 0, 0));
		}
		debugdraw::text(x, y+48, core::str::format("frame delta = %2.2fms\n", params.framedelta_filtered_msec), Color(255, 255, 255));
		debugdraw::text(x, y+60, core::str::format("# allocations = %i, total %i Kbytes\n", platform::memory::allocator().total_allocations(), platform::memory::allocator().total_bytes()/1024), Color(64, 102, 192));
	}

	virtual void tick( kernel::Params & params )
	{
		// run server frame
		if (game_interface)
		{
			game_interface->server_frame();
						
			game_interface->client_frame();
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
	
	virtual void shutdown( kernel::Params & params )
	{
		DESTROY(SceneRenderMethod, render_method);
		
		if (device)
		{
			vr::destroy_device(device);
		}
		vr::shutdown();
		
		
//		if (destroy_gamerules)
//		{
//			destroy_gamerules(gamerules);
//		}
		
		
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