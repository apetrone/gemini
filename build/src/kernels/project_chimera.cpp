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
#include "kernel.hpp"
#include <stdio.h>
#include <slim/xlog.h>
#include "mathlib.h"
#include "debugdraw.hpp"
#include "input.hpp"
#include "renderer.hpp"
#include "renderstream.hpp"

#include "camera.hpp"

#include "assets/asset_mesh.hpp"

#include "btBulletDynamicsCommon.h"

#include "physics.hpp"

#include "font.hpp"
#include "assets/asset_font.hpp"


using namespace physics;



class ProjectChimera : public kernel::IApplication,
public kernel::IEventListener<kernel::KeyboardEvent>,
public kernel::IEventListener<kernel::MouseEvent>,
public kernel::IEventListener<kernel::SystemEvent>
{

public:
	DECLARE_APPLICATION( ProjectChimera );
	assets::Mesh * plane_mesh;
	assets::Mesh * char_mesh;
	Camera camera;
	physics::CharacterController* character;

	ProjectChimera()
	{
		camera.type = Camera::TARGET;
	}
	
	virtual void event( kernel::KeyboardEvent & event )
	{
		if (event.is_down)
		{
			if (event.key == input::KEY_ESCAPE)
			{
				kernel::instance()->set_active(false);
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
		physics::startup();
		
		character = physics::create_character_controller(btVector3(0, 5, 0), false);
		
		// load in the plane mesh
		plane_mesh = assets::meshes()->load_from_path("models/construct");
		if (plane_mesh)
		{
			plane_mesh->prepare_geometry();
		}
		
		physics::create_physics_for_mesh(plane_mesh);
		
		
		
		char_mesh = assets::meshes()->load_from_path("models/character_box");
		if (char_mesh)
		{
			char_mesh->prepare_geometry();
		}

		debugdraw::startup(1024);

		camera.target_lookatOffset = glm::vec3(0, 0, 5);
		
		camera.perspective( 60.0f, params.render_width, params.render_height, 0.1f, 128.0f );
		// This is appropriate for drawing 3D models, but not sprites
		//camera.set_absolute_position( glm::vec3(8, 5, 8.0f) );
		//camera.yaw = -45;
		//camera.pitch = 30;
		camera.update_view();
		
		character->reset();
		
		// capture the mouse
		kernel::instance()->capture_mouse( true );

		return kernel::Application_Success;
	}

	virtual void step( kernel::Params & params )
	{
		physics::step( params.step_interval_seconds );
		
		// grab state here?
		physics::MovementCommand command;
		command.time = 0;
		command.left = input::state()->keyboard().is_down(input::KEY_A);
		command.right = input::state()->keyboard().is_down(input::KEY_D);
		command.forward = input::state()->keyboard().is_down(input::KEY_W);
		command.back = input::state()->keyboard().is_down(input::KEY_S);
		
		physics::player_move(character, camera, command);
		
		


		// rotate physics body
		btTransform worldTrans = character->getGhostObject()->getWorldTransform();
		btQuaternion rotation(btVector3(0,1,0), DegToRad(-camera.yaw));
		
		worldTrans.setRotation(rotation);
		character->getGhostObject()->setWorldTransform(worldTrans);
		
		physics::copy_ghost_to_camera(character->getGhostObject(), camera);
		
		
		

		
		//camera.pos += glm::vec3(0, 2.5, 5);
		camera.update_view();

		physics::debug_draw();
		
		
		debugdraw::text(10, 0, xstr_format("camera.pos = %.2g %.2g %.2g", camera.pos.x, camera.pos.y, camera.pos.z), Color(255, 255, 255));
		debugdraw::text(10, 12, xstr_format("eye_position = %.2g %.2g %.2g", camera.eye_position.x, camera.eye_position.y, camera.eye_position.z), Color(255, 0, 255));
		debugdraw::text(10, 24, xstr_format("camera.view = %.2g %.2g %.2g", camera.view.x, camera.view.y, camera.view.z), Color(128, 128, 255));
		debugdraw::text(10, 36, xstr_format("camera.right = %.2g %.2g %.2g", camera.side.x, camera.side.y, camera.side.z), Color(255, 0, 0));
		

		debugdraw::update(params.step_interval_seconds);
	}

	virtual void tick( kernel::Params & params )
	{
		RenderStream rs;
		renderer::GeneralParameters gp;

		gp.camera_position = &camera.pos;
		gp.modelview_matrix = &camera.matCam;
		gp.projection_project = &camera.matProj;
		
		glm::mat4 ident;
		gp.object_matrix = &ident;

		rs.add_viewport( 0, 0, params.render_width, params.render_height );
		rs.add_clearcolor( 0.1, 0.1, 0.1, 1.0f );
		rs.add_clear( renderer::CLEAR_COLOR_BUFFER | renderer::CLEAR_DEPTH_BUFFER );

		
		for( unsigned short i = 0; i < plane_mesh->total_geometry; ++i )
		{
			render_utilities::stream_geometry( rs, &plane_mesh->geometry[i], gp );
		}
		


		
		glm::mat4 char_mat = glm::mat4(1.0);
		
		// TODO: this should use the actual player height instead of
		// hard coding the value.
		char_mat = glm::translate(camera.pos - glm::vec3(0,1.82,0));
		char_mat = glm::rotate(char_mat, -camera.yaw, glm::vec3(0,1,0));
		gp.object_matrix = &char_mat;
		
			
		for( unsigned short i = 0; i < char_mesh->total_geometry; ++i )
		{
			render_utilities::stream_geometry( rs, &char_mesh->geometry[i], gp );
		}

		//rs.add_blendfunc( renderer::BLEND_SRC_ALPHA, renderer::BLEND_ONE_MINUS_SRC_ALPHA );
		//rs.add_state( renderer::STATE_BLEND, 1 );
		//rs.add_state( renderer::STATE_DEPTH_TEST, 0 );
		
		rs.run_commands();
		
		{
			glm::mat4 modelview;
			debugdraw::render(modelview, camera.matCamProj, params.render_width, params.render_height);
		}


	}
	
	virtual void shutdown( kernel::Params & params )
	{
		physics::shutdown();
		
		debugdraw::shutdown();
	}
};

IMPLEMENT_APPLICATION( ProjectChimera );