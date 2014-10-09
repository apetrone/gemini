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

using namespace physics;

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

	
	renderer::SceneLink scenelink;
	

	ProjectChimera()
	{
		character = 0;
		root = 0;
		camera.type = Camera::FIRST_PERSON;
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
			}
			else if (event.key == input::KEY_J)
			{
				LOGV("check controllers\n");
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
		return kernel::Application_Success;
	}

	virtual kernel::ApplicationResult startup( kernel::Params & params )
	{
		character = physics::create_character_controller(btVector3(0, 2, 0), false);

		root = CREATE(scenegraph::Node);
		root->name = "scene_root";

		//camera.target_lookatOffset = glm::vec3(0, 0, 1);
		
		camera.perspective(50.0f, params.render_width, params.render_height, 0.1f, 8192.0f);
		// This is appropriate for drawing 3D models, but not sprites
		//camera.set_absolute_position( glm::vec3(8, 5, 8.0f) );
		camera.yaw = -45;
		camera.pitch = 30;
		camera.update_view();
		
		character->reset();
		
		// capture the mouse
		kernel::instance()->capture_mouse( true );
		
//		scenegraph::Node* ground = add_mesh_to_root(root, "models/power_grid", true);
		
//		scenegraph::Node* generator = add_mesh_to_root(root, "models/generator_core", true);
//		if (generator)
//		{
//			generator->translation = glm::vec3(4, 0, -10);
//		}

		entity_startup();
	
		entity_set_scene_root(root);
	
		script::execute_file("scripts/project_chimera.nut");
		
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
		physics::player_move(character, camera, command);
		
		// if you want to move JUST the camera instead...
//		camera.move_left(input::state()->keyboard().is_down(input::KEY_A));
//		camera.move_right(input::state()->keyboard().is_down(input::KEY_D));
//		camera.move_forward(input::state()->keyboard().is_down(input::KEY_W));
//		camera.move_backward(input::state()->keyboard().is_down(input::KEY_S));
//		camera.update_view();
		


		// rotate physics body
		btTransform worldTrans = character->getGhostObject()->getWorldTransform();
		btQuaternion rotation(btVector3(0,1,0), mathlib::degrees_to_radians(-camera.yaw));
		
		worldTrans.setRotation(rotation);
		character->getGhostObject()->setWorldTransform(worldTrans);
		
//		if (player)
//		{
			physics::copy_ghost_to_camera(character->getGhostObject(), camera);
//		}
		
		
		
		
		entity_step();

		
		//camera.pos += glm::vec3(0, 2.5, 5);
		camera.update_view();

//		physics::debug_draw();
		debugdraw::axes(glm::mat4(1.0), 1.0f);
		debugdraw::text(10, 0, xstr_format("camera.pos = %.2g %.2g %.2g", camera.pos.x, camera.pos.y, camera.pos.z), Color(255, 255, 255));
		debugdraw::text(10, 12, xstr_format("eye_position = %.2g %.2g %.2g", camera.eye_position.x, camera.eye_position.y, camera.eye_position.z), Color(255, 0, 255));
		debugdraw::text(10, 24, xstr_format("camera.view = %.2g %.2g %.2g", camera.view.x, camera.view.y, camera.view.z), Color(128, 128, 255));
		debugdraw::text(10, 36, xstr_format("camera.right = %.2g %.2g %.2g", camera.side.x, camera.side.y, camera.side.z), Color(255, 0, 0));
		debugdraw::text(10, 48, xstr_format("frame_delta = %g", params.framedelta_raw_msec), Color(255, 255, 255));

		root->update(params.step_interval_seconds);
	}

	virtual void tick( kernel::Params & params )
	{
		entity_tick();
	
		RenderStream rs;
		rs.add_viewport( 0, 0, params.render_width, params.render_height );
		rs.add_clearcolor( 0.1, 0.1, 0.1, 1.0f );
		rs.add_clear( renderer::CLEAR_COLOR_BUFFER | renderer::CLEAR_DEPTH_BUFFER );
	
		glm::mat4 char_mat = glm::mat4(1.0);
		
		// TODO: this should use the actual player height instead of
		// hard coding the value.
//		char_mat = glm::translate(camera.pos - glm::vec3(0,1.82,0));
//		char_mat = glm::rotate(char_mat, -camera.yaw, glm::vec3(0,1,0));
		//if (player)
		{
			//player->world_transform = char_mat;
		}

		//rs.add_blendfunc( renderer::BLEND_SRC_ALPHA, renderer::BLEND_ONE_MINUS_SRC_ALPHA );
		//rs.add_state( renderer::STATE_BLEND, 1 );
		//rs.add_state( renderer::STATE_DEPTH_TEST, 0 );
		
		rs.run_commands();

		glm::vec3 light_position(0.5f, 2.5f, -12);
		renderer::ConstantBuffer cb;

		cb.modelview_matrix = &camera.matCam;
		cb.projection_matrix = &camera.matProj;
		cb.viewer_direction = &camera.view;
		cb.viewer_position = &camera.eye_position;
		cb.light_position = &light_position;
		
		scenelink.draw(root, cb);
		
		{
			glm::mat4 modelview;
			debugdraw::render(modelview, camera.matCamProj, params.render_width, params.render_height);
		}
	}
	
	virtual void shutdown( kernel::Params & params )
	{
		DESTROY(Node, root);
		
//		entity_shutdown();
	}
};

IMPLEMENT_APPLICATION( ProjectChimera );