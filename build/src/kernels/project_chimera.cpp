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
#include "mathlib.h"
#include "debugdraw.h"
#include "input.h"
#include "renderer.h"
#include "renderstream.h"

#include "camera.h"

#include "assets/asset_mesh.h"

#include "btBulletDynamicsCommon.h"

#include "physics.h"

#include "font.h"
#include "assets/asset_font.h"
#include "entity.h"
#include "script.h"
#include "scene_graph.h"

#include "meshnode.h"
#include "skeletalnode.h"

using namespace physics;



class ProjectChimera : public kernel::IApplication,
public kernel::IEventListener<kernel::KeyboardEvent>,
public kernel::IEventListener<kernel::MouseEvent>,
public kernel::IEventListener<kernel::SystemEvent>,
public scenegraph::Visitor
{

public:
	DECLARE_APPLICATION( ProjectChimera );
	assets::Mesh * plane_mesh;
	assets::Mesh * char_mesh;
	Camera camera;
	physics::CharacterController* character;
	assets::Shader* animation;
	scenegraph::Node* root;
	
	
	// members of the mesh visitor
	RenderStream* renderstream;
	renderer::GeneralParameters* generalparams;
	
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
			else if (event.key == input::KEY_SPACE)
			{
				root->update(kernel::instance()->parameters().step_interval_seconds);
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
		// TODO: add this stuff to a render queue for sorting, etc.
		if (node->get_type() == scenegraph::MESH)
		{
			scenegraph::MeshNode* meshnode = static_cast<scenegraph::MeshNode*>(node);
			assets::Mesh* mesh = meshnode->mesh;

			// TODO: update world transform?
			node->world_transform = glm::mat4(1.0);
			node->world_transform = glm::translate(node->local_to_world, node->local_position);
			
			// compose a matrix
			this->generalparams->object_matrix = &node->world_transform;

			for( unsigned short i = 0; i < mesh->total_geometry; ++i )
			{
				render_utilities::stream_geometry(*this->renderstream, &mesh->geometry[i], *this->generalparams );
			}
		}
		
		
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
		
		root = CREATE(scenegraph::Node);
		root->name = "scene_root";
		
		scenegraph::MeshNode* mn = 0;
//		mn = CREATE(scenegraph::MeshNode);
//		mn->load_mesh("models/house", true);
//		root->add_child(mn);
		
		mn = CREATE(scenegraph::MeshNode);
		mn->load_mesh("models/ground", true);
		root->add_child(mn);

		
		scenegraph::SkeletalNode* sn = CREATE(scenegraph::SkeletalNode);
		sn->load_mesh("models/test", true);
		sn->local_position = glm::vec3(0,1,0);
		root->add_child(sn);

		sn->setup_skeleton();
		

		char_mesh = assets::meshes()->load_from_path("models/agent_cooper");
		if (char_mesh)
		{
			char_mesh->prepare_geometry();
		}
		

		

		debugdraw::startup(1024);

		camera.target_lookatOffset = glm::vec3(0, 0, 5);
		
		camera.perspective( 50.0f, params.render_width, params.render_height, 0.1f, 128.0f );
		// This is appropriate for drawing 3D models, but not sprites
		//camera.set_absolute_position( glm::vec3(8, 5, 8.0f) );
		//camera.yaw = -45;
		//camera.pitch = 30;
		camera.update_view();
		
		character->reset();
		
		// capture the mouse
		kernel::instance()->capture_mouse( true );
		
		animation = CREATE(assets::Shader);
		
		animation->set_frag_data_location("out_color");
		animation->alloc_uniforms(4);
		animation->uniforms[0].set_key("projection_matrix");
		animation->uniforms[1].set_key("modelview_matrix");
		animation->uniforms[2].set_key("object_matrix");
		animation->uniforms[3].set_key("diffusemap");
		
		animation->alloc_attributes(3);
		animation->attributes[0].set_key("in_position"); animation->attributes[0].second = 0;
		animation->attributes[1].set_key("in_normal"); animation->attributes[1].second = 1;
		animation->attributes[2].set_key("in_uv"); animation->attributes[2].second = 2;

		
		assets::load_shader("shaders/animation", animation);


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
		
		physics::copy_ghost_to_camera(character->getGhostObject(), camera);
		
		
		
		
		entity_step();

		
		//camera.pos += glm::vec3(0, 2.5, 5);
		camera.update_view();

		//physics::debug_draw();
		
		
		debugdraw::text(10, 0, xstr_format("camera.pos = %.2g %.2g %.2g", camera.pos.x, camera.pos.y, camera.pos.z), Color(255, 255, 255));
		debugdraw::text(10, 12, xstr_format("eye_position = %.2g %.2g %.2g", camera.eye_position.x, camera.eye_position.y, camera.eye_position.z), Color(255, 0, 255));
		debugdraw::text(10, 24, xstr_format("camera.view = %.2g %.2g %.2g", camera.view.x, camera.view.y, camera.view.z), Color(128, 128, 255));
		debugdraw::text(10, 36, xstr_format("camera.right = %.2g %.2g %.2g", camera.side.x, camera.side.y, camera.side.z), Color(255, 0, 0));
		
#if 0
		for(size_t boneid = 0; boneid < plane_mesh->total_bones; ++boneid)
		{
			debugdraw::axes(plane_mesh->bones[boneid].inverse_bind_matrix, 1.0f);
//			debugdraw::sphere(plane_mesh->bones[boneid].world_position, Color(255,0,0), 0.25f);
//			plane_mesh->bones
		}
#endif

		root->update(params.step_interval_seconds);

		debugdraw::update(params.step_interval_seconds);
	}

	virtual void tick( kernel::Params & params )
	{
		entity_tick();
	
	
	
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


		
		this->renderstream = &rs;
		this->generalparams = &gp;
		
		// render nodes
		scenegraph::visit_nodes(root, this);

		
		glm::mat4 char_mat = glm::mat4(1.0);
		
		// TODO: this should use the actual player height instead of
		// hard coding the value.
		char_mat = glm::translate(camera.pos - glm::vec3(0,1.82,0));
		char_mat = glm::rotate(char_mat, -camera.yaw, glm::vec3(0,1,0));
		gp.object_matrix = &char_mat;

			
		for( unsigned short i = 0; i < char_mesh->total_geometry; ++i )
		{
			render_utilities::stream_geometry( rs, &char_mesh->geometry[i], gp, animation );
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
		DESTROY(Node, root);
		
		entity_shutdown();
	
		DESTROY(Shader, animation);
		
		physics::shutdown();
		
		debugdraw::shutdown();
	}
};

IMPLEMENT_APPLICATION( ProjectChimera );