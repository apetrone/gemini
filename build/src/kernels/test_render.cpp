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

#include "font.h"
#include "assets/asset_font.h"

#include "scene_graph.h"

#include "meshnode.h"

#include "renderer/scenelink.h"

#ifdef USE_WEBSERVER
	#include "civetweb.h"
	#include "CivetServer.h"

	class TestHandler : public CivetHandler
	{
		virtual bool handlePut(CivetServer* server, struct mg_connection* conn)
		{
			const struct mg_request_info* request = mg_get_request_info(conn);
			std::string buf;
			
			buf.resize(1024);
			int bytes = mg_read(conn, &buf[0], 1024);
//			LOGV("read %i bytes, %s\n", bytes, buf.c_str());

			// On Error, we can return "400 Bad Request",
			// but we'll have to provide a response body to describe why.

			std::string output = "204 No Content";
			mg_write(conn, &output[0], output.length());

			return true;
		}
		
		virtual bool handleDelete(CivetServer* server, struct mg_connection* conn)
		{
			return true;
		}
	};

	static int log_message(const struct mg_connection *conn, const char *message)
	{
		(void) conn;
		printf("%s\n", message);
		return 0;
	}
#endif

//#define SCENE_GRAPH_MANUAL 1

#define USE_MESH_NODE_RENDERING 0

class TestRender : public kernel::IApplication,
public kernel::IEventListener<kernel::KeyboardEvent>,
public kernel::IEventListener<kernel::MouseEvent>,
public kernel::IEventListener<kernel::SystemEvent>,
public scenegraph::Visitor
{

public:
	DECLARE_APPLICATION( TestRender );
	Camera camera;
	scenegraph::Node* root;
	
	size_t total_scene_nodes_visited;
	renderer::SceneLink scenelink;
	
#ifdef USE_WEBSERVER
	CivetServer* server;
#endif
	
	TestRender()
	{
//		camera.type = Camera::TARGET;
		root = 0;
		
#ifdef USE_WEBSERVER
		server = 0;
#endif
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
#ifdef SCENE_GRAPH_MANUAL
				root->update(kernel::instance()->parameters().step_interval_seconds);
#endif
			}
			else if (event.key == input::KEY_J)
			{
				LOGV("check controllers\n");
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

	virtual int visit(scenegraph::Node* node)
	{
		++total_scene_nodes_visited;
		

		// TODO: update world transform?
		glm::mat4 object_to_local = glm::translate(glm::mat4(1.0), node->local_position);
			
		node->world_transform = object_to_local * node->local_to_world;
		
		
		return 0;
	}

	virtual kernel::ApplicationResult config( kernel::Params & params )
	{
		params.window_title = "test_render";
		params.window_width = 1280;
		params.window_height = 720;
		return kernel::Application_Success;
	}

	virtual kernel::ApplicationResult startup( kernel::Params & params )
	{
#ifdef USE_WEBSERVER
		struct mg_callbacks cb;
		memset(&cb, 0, sizeof(mg_callbacks));
		cb.log_message = log_message;
		
		const char* options[] = {
			"listening_ports", "1983",
			"request_timeout_ms", "10",
			0
		};
		
		server = CREATE(CivetServer, options, &cb);
		server->addHandler("/json", new TestHandler());
#endif
		

		root = CREATE(scenegraph::Node);
		root->name = "scene_root";
		
		scenegraph::MeshNode* skydome = 0;
		skydome = CREATE(scenegraph::MeshNode);
		assets::Material* colormat = assets::materials()->load_from_path("materials/skydome");
		skydome->load_mesh("models/skydome", false, 0);
		// make it extend slightly below ground level
		skydome->local_position = glm::vec3(0, -50, 0);
		root->add_child(skydome);

		
		scenegraph::MeshNode* ground = 0;
		ground = CREATE(scenegraph::MeshNode);
		ground->load_mesh("models/vapor", false);
		root->add_child(ground);
//		ground->visible = false;

				
		debugdraw::startup(1024);

		camera.target_lookatOffset = glm::vec3(0, 0, 1);
		
		camera.perspective( 50.0f, params.render_width, params.render_height, 0.1f, 8192.0f );
		
		// This is appropriate for drawing 3D models, but not sprites
		camera.set_absolute_position( glm::vec3(8, 5, 8.0f) );
		camera.yaw = -45;
		camera.pitch = 30;
		camera.update_view();

		return kernel::Application_Success;
	}

	virtual void step( kernel::Params & params )
	{
		float dt_seconds = params.framedelta_filtered_msec * .001f;
		camera.move_left(input::state()->keyboard().is_down(input::KEY_A) * dt_seconds);
		camera.move_right(input::state()->keyboard().is_down(input::KEY_D) * dt_seconds);
		camera.move_forward(input::state()->keyboard().is_down(input::KEY_W) * dt_seconds);
		camera.move_backward(input::state()->keyboard().is_down(input::KEY_S) * dt_seconds);
		
		camera.update_view();


		debugdraw::text(10, 0, xstr_format("camera.pos = %.2g %.2g %.2g", camera.pos.x, camera.pos.y, camera.pos.z), Color(255, 255, 255));
		debugdraw::text(10, 12, xstr_format("eye_position = %.2g %.2g %.2g", camera.eye_position.x, camera.eye_position.y, camera.eye_position.z), Color(255, 0, 255));
		debugdraw::text(10, 24, xstr_format("camera.view = %.2g %.2g %.2g", camera.view.x, camera.view.y, camera.view.z), Color(128, 128, 255));
		debugdraw::text(10, 36, xstr_format("camera.right = %.2g %.2g %.2g", camera.side.x, camera.side.y, camera.side.z), Color(255, 0, 0));
		debugdraw::text(10, 48, xstr_format("frame_delta = %g", params.framedelta_raw_msec), Color(255, 255, 255));
		debugdraw::text(10, 60, xstr_format("scene graph nodes = %i", total_scene_nodes_visited), Color(128, 128, 255));


#ifndef SCENE_GRAPH_MANUAL
		root->update(params.step_interval_seconds);
#endif

		debugdraw::update(params.step_interval_seconds);
	}

	virtual void tick( kernel::Params & params )
	{
		RenderStream rs;
		rs.add_viewport( 0, 0, params.render_width, params.render_height );
		rs.add_clearcolor( 0.1, 0.1, 0.1, 1.0f );
		rs.add_clear( renderer::CLEAR_COLOR_BUFFER | renderer::CLEAR_DEPTH_BUFFER );

		// render nodes
		total_scene_nodes_visited = 0;
		scenegraph::visit_nodes(root, this);

		rs.run_commands();
		
		scenelink.draw(root, camera.matCam, camera.matProj);
		
		{
			glm::mat4 modelview;
			debugdraw::render(modelview, camera.matCamProj, params.render_width, params.render_height);
		}
	}
	
	virtual void shutdown( kernel::Params & params )
	{
#ifdef USE_WEBSERVER
		DESTROY(CivetServer, server);
#endif
	
		DESTROY(Node, root);
		debugdraw::shutdown();
	}
};

IMPLEMENT_APPLICATION( TestRender );