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
#include <stdio.h>
#include <slim/xlog.h>
#include <core/mathlib.h>
#include <renderer/debugdraw.h>
#include "input.h"
#include <renderer/renderer.h>
#include <renderer/renderstream.h>

#include "camera.h"

#include "assets/asset_mesh.h"
#include "assets/asset_material.h"

#include "btBulletDynamicsCommon.h"

#include "physics.h"

#include <renderer/font.h>
#include "assets/asset_font.h"
#include "entity.h"
#include "script.h"
#include "scene_graph.h"

#include "skeletalnode.h"

#include <core/mathlib.h>

#include "scenelink.h"
#include "vr.h"

#include <renderer/constantbuffer.h>

#include "audio.h"

#define LOCK_CAMERA_TO_CHARACTER 1

// even if a VR device is attached, this will NOT render to it
// this allows debugging in some other mechanism to check sensor data.
uint8_t RENDER_TO_VR = 1;

uint8_t REQUIRE_RIFT = 1;

using namespace physics;



void render_scene_from_camera(scenegraph::Node* root, Camera& camera, renderer::SceneLink& scenelink)
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

	// draw scene graph
	scenelink.draw(root, cb);
}

class SceneRenderMethod
{
public:
	virtual ~SceneRenderMethod() {}
	virtual void render_frame( scenegraph::Node* root, Camera& camera, const kernel::Params& params ) = 0;
};


class VRRenderMethod : public SceneRenderMethod
{
	vr::HeadMountedDevice* device;
	
	renderer::SceneLink& scenelink;

public:
	VRRenderMethod(vr::HeadMountedDevice* in_device, renderer::SceneLink& in_link) : device(in_device), scenelink(in_link) {}
	
	virtual void render_frame( scenegraph::Node* root, Camera& camera, const kernel::Params& params )
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

			render_scene_from_camera(root, camera, scenelink);
			
			
			camera.matCam = old_matcam;
			// draw debug graphics
//			debugdraw::render(camera.matCam, camera.matProj, x, y, width, height);
		}

		camera.pos = old_camera_position;
				
		device->end_frame(driver);
	}
};

class DefaultRenderMethod : public SceneRenderMethod
{
	renderer::SceneLink& scenelink;
public:
	DefaultRenderMethod(renderer::SceneLink& in_link) : scenelink(in_link) {};

	virtual void render_frame( scenegraph::Node* root, Camera& camera, const kernel::Params& params )
	{
		RenderStream rs;
		rs.add_cullmode(renderer::CullMode::CULLMODE_BACK);
//		rs.add_state(renderer::STATE_BACKFACE_CULLING, 0);
		rs.add_state(renderer::STATE_DEPTH_TEST, 1);
		rs.add_viewport( 0, 0, params.render_width, params.render_height );
		rs.add_clearcolor( 0.0, 0.0, 0.0, 1.0f );
		rs.add_clear( renderer::CLEAR_COLOR_BUFFER | renderer::CLEAR_DEPTH_BUFFER );
		rs.run_commands();
		
		render_scene_from_camera(root, camera, scenelink);
		
		// draw debug graphics
		{
			debugdraw::render(camera.matCam, camera.matProj, 0, 0, params.render_width, params.render_height);
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
	physics::CharacterController* player_controller;
	physics::KinematicCharacter* character;
	scenegraph::Node* root;
	vr::HeadMountedDevice* device;

	renderer::SceneLink scenelink;
	SceneRenderMethod* render_method;
	
	
	audio::SoundHandle background;
	audio::SoundSource background_source;
	
	bool draw_physics_debug;

	ProjectChimera()
	{
		player_controller = 0;
		character = 0;
		root = 0;
		device = 0;
		render_method = 0;
		active_camera = 0;
		draw_physics_debug = false;
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
		
		
//		background = audio::create_sound("sounds/wind_loop");
//		background_source = audio::play(background, -1);
		
		// create character
		character = physics::create_character_controller(btVector3(0, 2, 0), false);
		character->clear_state();
		
		player_controller = CREATE(CharacterController);
		player_controller->character = character;
			
		// capture the mouse
		kernel::instance()->capture_mouse( true );

		// entity startup
		entity_startup();
		
		// create scene graph root
		root = CREATE(scenegraph::Node);
		root->name = "scene_root";
		entity_set_scene_root(root);
	
		// run a script
		script::execute_file("scripts/project_chimera.nut");
		
		// perform post-script load duties
		entity_post_script_load();

		
		// setup camera
		Sqrat::RootTable roottable( script::get_vm() );
		Sqrat::Object gamerules = roottable.GetSlot( _SC("gamerules") );
		if ( !gamerules.IsNull() )
		{
			GameRules * gr = gamerules.Cast<GameRules*>();
			if ( gr )
			{
				active_camera = gr->get_active_camera();
				if (active_camera)
				{
					active_camera->type = Camera::FIRST_PERSON;
					active_camera->move_speed = 0.1f;
					active_camera->perspective(camera_fov, params.render_width, params.render_height, 0.01f, 8192.0f);
					active_camera->set_absolute_position(glm::vec3(0, 0, 5));
					active_camera->update_view();
				}
				else
				{
					LOGE("Error! No camera attached to gamerules!\n");
				}
			}
			else
			{
				LOGE("Error! Gamerules is invalid!\n");
			}
		}


		return kernel::Application_Success;
	}

	virtual void step( kernel::Params & params )
	{
		// grab state here?
		physics::MovementCommand command;
		command.time = 0;

		// handle input
		player_controller->get_input_command(command);
		



		Sqrat::RootTable roottable( script::get_vm() );
		Sqrat::Object gamerules = roottable.GetSlot( _SC("gamerules") );
		if ( !gamerules.IsNull() )
		{
			GameRules * gr = gamerules.Cast<GameRules*>();
			if ( gr )
			{
				active_camera = gr->get_active_camera();
				player_controller->camera = active_camera;
			}
		}
		
		// apply input
		player_controller->apply_command(command);
		
		if (active_camera)
		{
		
#if LOCK_CAMERA_TO_CHARACTER
		//physics::player_move(character, *active_camera, command);
#else
		// if you want to move JUST the camera instead...
		active_camera->move_left(input::state()->keyboard().is_down(input::KEY_A));
		active_camera->move_right(input::state()->keyboard().is_down(input::KEY_D));
		active_camera->move_forward(input::state()->keyboard().is_down(input::KEY_W));
		active_camera->move_backward(input::state()->keyboard().is_down(input::KEY_S));
		active_camera->update_view();
#endif
	
		
		}
		
		if (active_camera)
		{
			entity_step();
			
			float joystick_sensitivity = 20;
			active_camera->move_view(
				joystick_sensitivity*input::state()->joystick(0).axes[2].normalized_value,
				joystick_sensitivity*input::state()->joystick(0).axes[3].normalized_value
			);
			
			active_camera->update_view();
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
			debugdraw::text(x, y, xstr_format("active_camera->pos = %.2g %.2g %.2g", active_camera->pos.x, active_camera->pos.y, active_camera->pos.z), Color(255, 255, 255));
			debugdraw::text(x, y+12, xstr_format("eye_position = %.2g %.2g %.2g", active_camera->eye_position.x, active_camera->eye_position.y, active_camera->eye_position.z), Color(255, 0, 255));
			debugdraw::text(x, y+24, xstr_format("active_camera->view = %.2g %.2g %.2g", active_camera->view.x, active_camera->view.y, active_camera->view.z), Color(128, 128, 255));
			debugdraw::text(x, y+36, xstr_format("active_camera->right = %.2g %.2g %.2g", active_camera->side.x, active_camera->side.y, active_camera->side.z), Color(255, 0, 0));
		}
		debugdraw::text(x, y+48, xstr_format("frame delta = %2.2fms\n", params.framedelta_filtered_msec), Color(255, 255, 255));
		debugdraw::text(x, y+60, xstr_format("# allocations = %i, total %i Kbytes\n", memory::allocator().total_allocations(), memory::allocator().total_bytes()/1024), Color(64, 102, 192));

		root->update(params.step_interval_seconds);
	}

	virtual void tick( kernel::Params & params )
	{
		// need to tick entities
		entity_tick();
		
		// then update the scene graph
		root->update_transforms();
	
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

		if (active_camera)
		{
			assert(active_camera != nullptr);
			render_method->render_frame(root, *active_camera, params);
		}
	}
	
	virtual void shutdown( kernel::Params & params )
	{
		DESTROY(CharacterController, player_controller);
	
		// cleanup entities
		// perhaps just destroy the gamerules and the entities will follow?
		script::shutdown();

		entity_shutdown();

		DESTROY(Node, root);

		DESTROY(SceneRenderMethod, render_method);
		
		if (device)
		{
			vr::destroy_device(device);
		}
		vr::shutdown();
	}
};

IMPLEMENT_APPLICATION( ProjectChimera );