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


using namespace physics;



class TestBullet2 : public kernel::IApplication,
public kernel::IEventListener<kernel::KeyboardEvent>,
public kernel::IEventListener<kernel::MouseEvent>,
public kernel::IEventListener<kernel::SystemEvent>
{

public:
	DECLARE_APPLICATION( TestBullet2 );
	assets::Mesh * plane_mesh;
	Camera camera;
	physics::CharacterController* character;
	
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
		params.window_title = "bullet2";
		params.window_width = 800;
		params.window_height = 600;
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

		debugdraw::startup(1024);

		return kernel::Application_Success;
	}

	virtual void step( kernel::Params & params )
	{
		physics::step( params.step_interval_seconds );
		
		
		physics::copy_ghost_to_camera(character->getGhostObject(), camera);
		camera.update_view();
		
		debugdraw::update(params.step_interval_seconds);
	}

	virtual void tick( kernel::Params & params )
	{
		camera.perspective( 60.0f, params.render_width, params.render_height, 0.1f, 128.0f );
		// This is appropriate for drawing 3D models, but not sprites
		camera.set_absolute_position( glm::vec3(8, 5, 8.0f) );
		camera.yaw = -45;
		camera.pitch = 30;
		camera.update_view();
	
		RenderStream rs;
		renderer::GeneralParameters gp;

		physics::copy_ghost_to_camera(character->getGhostObject(), camera);
		camera.update_view();

		gp.camera_position = &camera.pos;
		gp.modelview_matrix = &camera.matCam;
		gp.projection_project = &camera.matProj;
		
		glm::mat4 ident;
		gp.object_matrix = &ident;
		
		rs.add_clear( renderer::CLEAR_COLOR_BUFFER | renderer::CLEAR_DEPTH_BUFFER );
		rs.add_viewport( 0, 0, params.render_width, params.render_height );
		rs.add_clearcolor( 0.1, 0.1, 0.1, 1.0f );
	
		for( unsigned short i = 0; i < plane_mesh->total_geometry; ++i )
		{
			render_utilities::stream_geometry( rs, &plane_mesh->geometry[i], gp );
		}

		
		rs.run_commands();
		
		
		{
			glm::mat4 modelview;
			glm::mat4 projection = glm::ortho( 0.0f, (float)params.render_width, (float)params.render_height, 0.0f, 0.0f, 128.0f );
			
			debugdraw::render( modelview, projection, params.render_width, params.render_height );
		}
		
	}
	
	virtual void shutdown( kernel::Params & params )
	{
		physics::shutdown();
		
		debugdraw::shutdown();
	}
};

IMPLEMENT_APPLICATION( TestBullet2 );