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
#include <gemini/mathlib.h>
#include "debugdraw.h"
#include "input.h"
#include "renderer/renderer.h"
#include "renderer/renderstream.h"

#include "camera.h"

#include "assets/asset_mesh.h"
#include "assets/asset_material.h"

#include "btBulletDynamicsCommon.h"

#include "physics.h"

#include "font.h"
#include "assets/asset_font.h"
#include "entity.h"
#include "script.h"
#include "scene_graph.h"

#include "skeletalnode.h"

#include <gemini/mathlib.h>

#include "renderer/scenelink.h"
#include "vr.h"

#define LOCK_CAMERA_TO_CHARACTER 0

// even if a VR device is attached, this will NOT render to it
// this allows debugging in some other mechanism to check sensor data.
uint8_t RENDER_TO_VR = 0;

using namespace physics;

void render_scene_from_camera(scenegraph::Node* root, Camera& camera)
{
	// setup constant buffer
	glm::vec3 light_position(0.5f, 2.5f, -12);
	renderer::ConstantBuffer cb;
	cb.modelview_matrix = &camera.matCam;
	cb.projection_matrix = &camera.matProj;
	cb.viewer_direction = &camera.view;
	cb.viewer_position = &camera.eye_position;
	cb.light_position = &light_position;
	
	// draw scene graph
	renderer::SceneLink scenelink;
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

public:
	VRRenderMethod(vr::HeadMountedDevice* in_device) : device(in_device) {}
	
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
			glm::vec3 translation = camera_position + eye_pose.offset + eye_pose.translation;

			glm::mat4 tr = glm::translate(glm::mat4(1.0f), translation);
			glm::mat4 ro = glm::toMat4(rotation);
			camera.matCam = glm::inverse(tr * ro);
			camera.matProj = proj[eye_index];
			
			rs.add_viewport(x, y, width, height);
			
			// render nodes
			rs.run_commands();
			rs.rewind();

			render_scene_from_camera(root, camera);
			
			// draw debug graphics
//			debugdraw::render(camera.matCam, camera.matProj, x, y, width, height);
		}

		camera.pos = old_camera_position;
				
		device->end_frame(driver);
	}
};

class DefaultRenderMethod : public SceneRenderMethod
{
public:
	virtual void render_frame( scenegraph::Node* root, Camera& camera, const kernel::Params& params )
	{
		RenderStream rs;
		rs.add_viewport( 0, 0, params.render_width, params.render_height );
		rs.add_clearcolor( 0.1, 0.1, 0.1, 1.0f );
		rs.add_clear( renderer::CLEAR_COLOR_BUFFER | renderer::CLEAR_DEPTH_BUFFER );
		rs.run_commands();
		
		render_scene_from_camera(root, camera);
		
		// draw debug graphics
		{
			debugdraw::render(camera.matCam, camera.matProj, 0, 0, params.render_width, params.render_height);
		}
	}
};


class ProjectChimera : public kernel::IApplication,
public kernel::IEventListener<kernel::KeyboardEvent>,
public kernel::IEventListener<kernel::MouseEvent>,
public kernel::IEventListener<kernel::SystemEvent>
{

public:
	DECLARE_APPLICATION(ProjectChimera);
	Camera camera;
	physics::CharacterController* character;
	scenegraph::Node* root;
	vr::HeadMountedDevice* device;

	SceneRenderMethod* render_method;

	ProjectChimera()
	{
		character = 0;
		root = 0;
		camera.type = Camera::FIRST_PERSON;
		device = 0;
		render_method = 0;
		camera.move_speed = 0.1f;
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
		}
	}
	
	virtual void event( kernel::MouseEvent & event )
	{
		switch(event.subtype)
		{
			case kernel::MouseMoved:
			{
				camera.move_view(event.dx, event.dy);
				//camera.update_view();
				break;
			}
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
		
	}

	virtual kernel::ApplicationResult config( kernel::Params & params )
	{
		params.window_title = "project_chimera";
		params.window_width = 1280;
		params.window_height = 720;

		vr::startup();
		
		// if there's a rift connected
		//if (vr::total_devices() > 0)
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
					params.use_fullscreen = 1;
					params.use_vsync = false; // disable vsync? does this reduce jutter when in extended mode?
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
			render_method = CREATE(VRRenderMethod, device);
		}
		else
		{
			render_method = CREATE(DefaultRenderMethod);
		}
	
		// create character
		character = physics::create_character_controller(btVector3(0, 2, 0), false);
		character->reset();
		
		// setup character
		camera.perspective(camera_fov, params.render_width, params.render_height, 0.01f, 8192.0f);
		camera.set_absolute_position(glm::vec3(0, 0, 5));
		camera.update_view();
			
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

		return kernel::Application_Success;
	}

	virtual void step( kernel::Params & params )
	{
		// grab state here?
		physics::MovementCommand command;
		command.time = 0;
		command.left = input::state()->keyboard().is_down(input::KEY_A);
		command.right = input::state()->keyboard().is_down(input::KEY_D);
		command.forward = input::state()->keyboard().is_down(input::KEY_W);
		command.back = input::state()->keyboard().is_down(input::KEY_S);
		
#if LOCK_CAMERA_TO_CHARACTER
		physics::player_move(character, camera, command);
#else
		// if you want to move JUST the camera instead...
		camera.move_left(input::state()->keyboard().is_down(input::KEY_A));
		camera.move_right(input::state()->keyboard().is_down(input::KEY_D));
		camera.move_forward(input::state()->keyboard().is_down(input::KEY_W));
		camera.move_backward(input::state()->keyboard().is_down(input::KEY_S));
		camera.update_view();
#endif

		// rotate physics body based on camera yaw
		btTransform worldTrans = character->getGhostObject()->getWorldTransform();
		btQuaternion rotation(btVector3(0,1,0), mathlib::degrees_to_radians(-camera.yaw));
		worldTrans.setRotation(rotation);
		character->getGhostObject()->setWorldTransform(worldTrans);
		
#if LOCK_CAMERA_TO_CHARACTER
		physics::copy_ghost_to_camera(character->getGhostObject(), camera);
#endif
		
		
		entity_step();

		camera.update_view();

		//physics::debug_draw();
		
//		debugdraw::axes(glm::mat4(1.0), 1.0f);
		debugdraw::text(10, 0, xstr_format("camera.pos = %.2g %.2g %.2g", camera.pos.x, camera.pos.y, camera.pos.z), Color(255, 255, 255));
		debugdraw::text(10, 12, xstr_format("eye_position = %.2g %.2g %.2g", camera.eye_position.x, camera.eye_position.y, camera.eye_position.z), Color(255, 0, 255));
		debugdraw::text(10, 24, xstr_format("camera.view = %.2g %.2g %.2g", camera.view.x, camera.view.y, camera.view.z), Color(128, 128, 255));
		debugdraw::text(10, 36, xstr_format("camera.right = %.2g %.2g %.2g", camera.side.x, camera.side.y, camera.side.z), Color(255, 0, 0));
		debugdraw::text(10, 48, xstr_format("frame_delta = %g", params.framedelta_raw_msec), Color(255, 255, 255));

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
//		char_mat = glm::translate(camera.pos - glm::vec3(0,1.82,0));
//		char_mat = glm::rotate(char_mat, -camera.yaw, glm::vec3(0,1,0));
		//if (player)
		{
			//player->world_transform = char_mat;
		}

		render_method->render_frame(root, camera, params);
	}
	
	virtual void shutdown( kernel::Params & params )
	{
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