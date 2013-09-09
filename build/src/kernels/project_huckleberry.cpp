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
#include "input.hpp"
#include <stdio.h>
#include <slim/xlog.h>
#include "filesystem.hpp"
#include "assets/asset_mesh.hpp"
#include "camera.hpp"
#include "gemgl.hpp"
#include "debugdraw.hpp"

#include "renderer.hpp"
#include "renderstream.hpp"
#include "render_utilities.hpp"

struct SceneNode
{
	assets::Mesh * mesh;
	glm::mat4 transform;
	
	
	SceneNode()
	{
		mesh = 0;
	}
}; // SceneNode


typedef std::vector<SceneNode*> SceneNodeVector;

struct Scene
{
	SceneNodeVector nodes;
	RenderStream rs;
	
	SceneNode * add_node()
	{
		SceneNode * node = CREATE(SceneNode);
		nodes.push_back( node );
		return node;
	}
	
	Scene()
	{
		
	}
	
	~Scene()
	{
		purge();
	}
	
	void purge()
	{
		for( SceneNodeVector::iterator it = nodes.begin(); it != nodes.end(); ++it )
		{
			SceneNode * node = (*it);
			DESTROY( SceneNode, node );
		}
		nodes.clear();
	} // purge
	
	void render( Camera & camera )
	{
		rs.rewind();
		renderer::GeneralParameters gp;
		
		glm::mat4 object_matrix;
		gp.global_params = 0;
		gp.camera_position = &camera.pos;
		gp.modelview_matrix = &camera.matCam;
		gp.projection_project = &camera.matProj;
		gp.object_matrix = &object_matrix;
		
		rs.add_viewport(0, 0, 800, 600);
		rs.add_clearcolor(0.15f, 0.15f, 0.15f, 1.0f);
		rs.add_clear( renderer::CLEAR_COLOR_BUFFER | renderer::CLEAR_DEPTH_BUFFER );
	
		
//		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
//		glDisable( GL_POLYGON_OFFSET_LINE );
		for( SceneNodeVector::iterator it = nodes.begin(); it != nodes.end(); ++it )
		{
			SceneNode * node = (*it);
			assert( node->mesh != 0 );
			
			render_utilities::stream_geometry( rs, &node->mesh->geometry[0], gp );
		}
		
		
		rs.run_commands();
		
#if 0
		glEnable( GL_POLYGON_OFFSET_LINE );
		glPolygonOffset( -1, -1 );
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		rs.rewind();
		for( SceneNodeVector::iterator it = nodes.begin(); it != nodes.end(); ++it )
		{
			SceneNode * node = (*it);
			assert( node->mesh != 0 );
			
			render_utilities::stream_geometry( rs, &node->mesh->geometry[0], gp );
		}
		
		rs.run_commands();
#endif
		

		

	} // render
}; // Scene





class ProjectHuckleberry : public kernel::IApplication,
public kernel::IEventListener<kernel::KeyboardEvent>,
public kernel::IEventListener<kernel::MouseEvent>,
public kernel::IEventListener<kernel::SystemEvent>
{
public:
	DECLARE_APPLICATION( ProjectHuckleberry );
	
	assets::Mesh * mesh;
	Scene scene;
	Camera camera;
	
	virtual void event( kernel::KeyboardEvent & event )
	{
		if (event.is_down)
		{
			if (event.key == input::KEY_ESCAPE)
			{
				kernel::instance()->set_active(false);
			}
		}
	}
	
	virtual void event( kernel::MouseEvent & event )
	{
		switch( event.subtype )
		{
			case kernel::MouseMoved:
			{
				if ( input::state()->mouse().is_down( input::MOUSE_LEFT ) )
				{
					int lastx, lasty;
					input::state()->mouse().last_mouse_position( lastx, lasty );
					
					camera.move_view( event.mx-lastx, event.my-lasty );
				}
                break;
			}
			default: break;
		}
	}
	
	virtual void event( kernel::SystemEvent & event )
	{
		
	}
	
	virtual kernel::ApplicationResult config( kernel::Params & params )
	{
		params.window_width = 800;
		params.window_height = 600;
		params.window_title = "ProjectHuckleberry";
		return kernel::Application_Success;
	}
	
	virtual kernel::ApplicationResult startup( kernel::Params & params )
	{
		mesh = assets::meshes()->load_from_path("models/room2");
		mesh->prepare_geometry();
		
		SceneNode * s0 = scene.add_node();
		s0->mesh = mesh;
		s0->transform = glm::mat4(1.0f);
		
		debugdraw::startup(1024);
		
		camera.set_absolute_position( glm::vec3(0, 1, 10.0f) );

		return kernel::Application_Success;
	}
	
	virtual void step( kernel::Params & params )
	{
		float dt = params.framedelta_filtered_msec * .001;
		debugdraw::update( params.framedelta_filtered_msec );
		
		camera.move_speed = 10.0f;
		
		if ( input::state()->keyboard().is_down(input::KEY_W) )
		{
			camera.move_forward( dt );
		}
		else if ( input::state()->keyboard().is_down(input::KEY_S) )
		{
			camera.move_backward( dt );
		}
		
		if ( input::state()->keyboard().is_down(input::KEY_A) )
		{
			camera.move_left( dt );
		}
		else if ( input::state()->keyboard().is_down(input::KEY_D) )
		{
			camera.move_right( dt );
		}
	}
	
	virtual void tick( kernel::Params & params )
	{
		
		camera.perspective( 60.0f, params.render_width, params.render_height, 0.1f, 128.0f );

		scene.render( camera );
		
		debugdraw::text( 25, 50, "Testing", Color(255, 255, 255) );

		debugdraw::axes( glm::mat4(1.0), 25.0f );
		
		debugdraw::render( camera.matCam, camera.matProj, params.render_width, params.render_height );
	}
	
	virtual void shutdown( kernel::Params & params )
	{
		debugdraw::shutdown();
	}
};

IMPLEMENT_APPLICATION( ProjectHuckleberry );