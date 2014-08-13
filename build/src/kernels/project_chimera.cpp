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

#include "meshnode.h"
#include "skeletalnode.h"

#include "renderer/scenelink.h"

using namespace physics;

//#define SCENE_GRAPH_MANUAL 1

#define USE_MESH_NODE_RENDERING 0

class ProjectChimera : public kernel::IApplication,
public kernel::IEventListener<kernel::KeyboardEvent>,
public kernel::IEventListener<kernel::MouseEvent>,
public kernel::IEventListener<kernel::SystemEvent>,
public scenegraph::Visitor
{

public:
	DECLARE_APPLICATION( ProjectChimera );
	Camera camera;
	physics::CharacterController* character;
	assets::Shader* animation;
	scenegraph::Node* root;
	
	size_t total_scene_nodes_visited;
	scenegraph::SkeletalNode* player;
	
	renderer::SceneLink scenelink;
	

	ProjectChimera()
	{
//		camera.type = Camera::TARGET;
		animation = 0;
		player = 0;
		character = 0;
		root = 0;

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

	virtual int visit(scenegraph::Node* node)
	{
		++total_scene_nodes_visited;
		

		glm::mat4 object_to_local = glm::translate(glm::mat4(1.0), node->local_position);
		node->world_transform = object_to_local * node->local_to_world;
		
#if GEMINI_ZUP_TO_YUP_CONVERSION
		if (node->type == scenegraph::MESH || node->type == scenegraph::SKELETON)
		{
			scenegraph::MeshNode* meshnode = static_cast<scenegraph::MeshNode*>(node);
			node->world_transform = meshnode->mesh->node_transform * node->world_transform;
		}
#else
#error No conversion to Y-up! Missing asset_mesh.h include.
#endif
		
		
		return 0;
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

		
		animation = CREATE(assets::Shader);
		
		animation->set_frag_data_location("out_color");
		animation->alloc_uniforms(5);
		animation->uniforms[0].set_key("projection_matrix");
		animation->uniforms[1].set_key("modelview_matrix");
		animation->uniforms[2].set_key("object_matrix");
		animation->uniforms[3].set_key("diffusemap");
		animation->uniforms[4].set_key("node_transforms");
		
		animation->alloc_attributes(6);
		animation->attributes[0].set_key("in_position"); animation->attributes[0].second = 0;
		animation->attributes[1].set_key("in_normal"); animation->attributes[1].second = 1;
		animation->attributes[2].set_key("in_color"); animation->attributes[2].second = 2;
		animation->attributes[3].set_key("in_uv"); animation->attributes[3].second = 3;
		animation->attributes[4].set_key("in_blendindices"); animation->attributes[4].second = 4;
		animation->attributes[5].set_key("in_blendweights"); animation->attributes[5].second = 5;
		
		assets::load_shader("shaders/animation", animation);
		
		
//		animation->show_attributes();
		
		root = CREATE(scenegraph::Node);
		root->name = "scene_root";
		


		scenegraph::MeshNode* skydome = 0;
		skydome = CREATE(scenegraph::MeshNode);
		assets::Material* colormat = assets::materials()->load_from_path("materials/skydome");
		skydome->load_mesh("models/skydome", false, 0);
		// make it extend slightly below ground level
		skydome->local_position = glm::vec3(0, -50, 0);
		root->add_child(skydome);

//		scenegraph::SkeletalNode* sn = 0;
//		sn = CREATE(scenegraph::SkeletalNode);
//		sn->load_mesh("models/test2", false);
//		sn->setup_skeleton();
//		sn->local_position = glm::vec3(0, 1, 0);
//		root->add_child(sn);

//		scenegraph::MeshNode* test5 = 0;
//		test5 = CREATE(scenegraph::MeshNode);
//		test5->load_mesh("models/delorean", false);
//		root->add_child(test5);
		
		scenegraph::MeshNode* ground = 0;
		ground = CREATE(scenegraph::MeshNode);
		ground->load_mesh("models/vapor", true);
		root->add_child(ground);
//		ground->visible = false;

		player = CREATE(scenegraph::SkeletalNode);
		player->load_mesh("models/test_yup", false);
		player->setup_skeleton();
		root->add_child(player);
//		player->visible = false;
		
//		scenegraph::MeshNode* test = CREATE(scenegraph::MeshNode);
//		test->load_mesh("models/teapot");
//		root->add_child(test);
//		
//		scenegraph::MeshNode* room = CREATE(scenegraph::MeshNode);
//		room->load_mesh("models/room2", true);
//		room->local_position = glm::vec3(0,0,0);
//		root->add_child(room);


		
		
		debugdraw::startup(1024);

		camera.target_lookatOffset = glm::vec3(0, 0, 1);
		
		camera.perspective( 50.0f, params.render_width, params.render_height, 0.1f, 8192.0f );
		// This is appropriate for drawing 3D models, but not sprites
		//camera.set_absolute_position( glm::vec3(8, 5, 8.0f) );
		//camera.yaw = -45;
		//camera.pitch = 30;
		camera.update_view();
		
		character->reset();
		
		// capture the mouse
		kernel::instance()->capture_mouse( true );
		


		entity_startup();
	
		script::execute_file("scripts/project_chimera.nut");
		
		entity_post_script_load();

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
		
		if (player)
		{
			physics::copy_ghost_to_camera(character->getGhostObject(), camera);
		}
		
		
		
		
		entity_step();

		
		//camera.pos += glm::vec3(0, 2.5, 5);
		camera.update_view();

		physics::debug_draw();
		
//		debugdraw::axes(glm::mat4(1.0), 2.0f);
		debugdraw::text(10, 0, xstr_format("camera.pos = %.2g %.2g %.2g", camera.pos.x, camera.pos.y, camera.pos.z), Color(255, 255, 255));
		debugdraw::text(10, 12, xstr_format("eye_position = %.2g %.2g %.2g", camera.eye_position.x, camera.eye_position.y, camera.eye_position.z), Color(255, 0, 255));
		debugdraw::text(10, 24, xstr_format("camera.view = %.2g %.2g %.2g", camera.view.x, camera.view.y, camera.view.z), Color(128, 128, 255));
		debugdraw::text(10, 36, xstr_format("camera.right = %.2g %.2g %.2g", camera.side.x, camera.side.y, camera.side.z), Color(255, 0, 0));
		debugdraw::text(10, 48, xstr_format("frame_delta = %g", params.framedelta_raw_msec), Color(255, 255, 255));
		debugdraw::text(10, 60, xstr_format("scene graph nodes = %i", total_scene_nodes_visited), Color(128, 128, 255));
		
#if 0
		for(size_t boneid = 0; boneid < plane_mesh->total_bones; ++boneid)
		{
			debugdraw::axes(plane_mesh->bones[boneid].inverse_bind_matrix, 1.0f);
//			debugdraw::sphere(plane_mesh->bones[boneid].world_position, Color(255,0,0), 0.25f);
//			plane_mesh->bones
		}
#endif

#ifndef SCENE_GRAPH_MANUAL
		root->update(params.step_interval_seconds);
#endif

		debugdraw::update(params.step_interval_seconds);
	}

	virtual void tick( kernel::Params & params )
	{
		entity_tick();
	
		RenderStream rs;
		rs.add_viewport( 0, 0, params.render_width, params.render_height );
		rs.add_clearcolor( 0.1, 0.1, 0.1, 1.0f );
		rs.add_clear( renderer::CLEAR_COLOR_BUFFER | renderer::CLEAR_DEPTH_BUFFER );

		// render nodes
		total_scene_nodes_visited = 0;
		scenegraph::visit_nodes(root, this);

		
		glm::mat4 char_mat = glm::mat4(1.0);
		
		// TODO: this should use the actual player height instead of
		// hard coding the value.
		char_mat = glm::translate(camera.pos - glm::vec3(0,1.82,0));
		char_mat = glm::rotate(char_mat, -camera.yaw, glm::vec3(0,1,0));
		if (player)
		{
			//player->world_transform = char_mat;
		}

		//rs.add_blendfunc( renderer::BLEND_SRC_ALPHA, renderer::BLEND_ONE_MINUS_SRC_ALPHA );
		//rs.add_state( renderer::STATE_BLEND, 1 );
		//rs.add_state( renderer::STATE_DEPTH_TEST, 0 );
		
		rs.run_commands();
		
		scenelink.draw(root, camera.matCam, camera.matProj);
		
		{
			glm::mat4 modelview;
			debugdraw::render(modelview, camera.matCamProj, params.render_width, params.render_height);
		}
	}
	
	virtual void shutdown( kernel::Params & params )
	{
		DESTROY(Node, root);
		
		entity_shutdown();
	
		DESTROY(Shader, animation);
		
		physics::shutdown();
		
		debugdraw::shutdown();
	}
};

IMPLEMENT_APPLICATION( ProjectChimera );