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
#include <stdio.h>

#include <slim/xlog.h>
#include <gemini/mathlib.h>
#include <gemini/util/threadsafequeue.h>

#include "kernel.h"
#include "debugdraw.h"
#include "input.h"
#include "renderer/renderer.h"
#include "renderer/renderstream.h"
#include "renderer/scenelink.h"
#include "camera.h"
#include "assets/asset_mesh.h"
#include "assets/asset_material.h"
#include "font.h"
#include "assets/asset_font.h"
#include "scene_graph.h"
#include "skeletalnode.h"
#include "physics.h"
#include "gemgl.h"

#include <json/json.h>

struct BaseVar
{
	BaseVar* next;
	std::string name;

	virtual void load(Json::Value& value) = 0;
	virtual std::string value_string() = 0;
	
	static void render_values(int x, int y);
};

BaseVar* tail = 0;

void BaseVar::render_values(int x, int y)
{
	BaseVar* current = tail;
	while (current)
	{
		debugdraw::text(x, y, xstr_format("[VAR] %s = %s", current->name.c_str(), current->value_string().c_str()), Color(255, 255, 255, 255));
	
		y += 12;
		current = current->next;
	}
}

BaseVar* find_by_name(const std::string& name)
{
	BaseVar* current = tail;
	
	while( current )
	{
		if (name == current->name)
		{
			return current;
		}
		current = current->next;
	}
	
	return 0;
}

template <class Type>
struct JsonLoader
{
	static void load(Json::Value& json_value, Type* value);
};

template <>
struct JsonLoader<float>
{
	static void load(Json::Value& json_value, float* value)
	{
		if (json_value.isDouble())
		{
			*value = json_value.asFloat();
		}
	}
};

template <>
struct JsonLoader<int>
{
	static void load(Json::Value& json_value, int* value)
	{
		if (json_value.isInt())
		{
			*value = json_value.asInt();
		}
	}
};

template <class Type>
struct Var : public BaseVar
{
	Type value;

	Var(const char* varname)
	{
		name = varname;
		next = tail;
		tail = this;
	}


	virtual void load(Json::Value& json_value)
	{
		JsonLoader<Type>::load(json_value, &value);
	}
	
	virtual std::string value_string()
	{
		std::string val = std::to_string(value);
		return val;
	}
};


void put_new_json(const std::string& json_document)
{
	Json::Value root;
	Json::Reader reader;
	
	bool success = reader.parse(json_document, root);
	if (success)
	{
		// apply this to variables
		Json::ValueIterator it = root.begin();
		
		size_t total_values_loaded = 0;
		size_t missing_values = 0;
		for( ; it != root.end(); ++it)
		{
			Json::Value key = it.key();
			Json::Value value = (*it);

			const std::string& name = key.asString();
			BaseVar* v = find_by_name(name);
			if (v)
			{
				v->load(value);
				++total_values_loaded;
			}
			else
			{
				++missing_values;
			}
		}
		
		LOGV("loaded (%u/%u) values\n", total_values_loaded, (total_values_loaded+missing_values));
	}
}


Var<float> meter("meter");
Var<int> test("value_test");

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
			assert(bytes < 1024);

			
//			LOGV("read %i bytes, %s\n", bytes, buf.c_str());

			put_new_json(buf);
			
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


	class ReloadHandler : public CivetHandler
	{
		ThreadSafeQueue<std::string>& queue;
		
	public:
	
		ReloadHandler(ThreadSafeQueue<std::string>& command_queue) : queue(command_queue)
		{}

		virtual bool handlePut(CivetServer* server, struct mg_connection* conn)
		{
			const struct mg_request_info* request = mg_get_request_info(conn);
			std::string buf;
			
			buf.resize(1024);
			int bytes = mg_read(conn, &buf[0], 1024);
			assert(bytes < 1024);
			
			// send a response that we received it.
			std::string output = "204 No Content";
			mg_write(conn, &output[0], output.length());
			

			LOGV("reload: %s\n", buf.c_str());
			
			Json::Reader reader;
			Json::Value root;

			bool success = reader.parse(&buf[0], &buf[0] + buf.size(), root);
			if (!success)
			{
				LOGW("ignored reload request: failed to parse json\n");
				LOGW("json parsing failed: %s\n", reader.getFormatedErrorMessages().c_str());
			}
			else
			{
				if (!root["resource"].isNull())
				{
					StackString<1024> filename = root["resource"].asString().c_str();
					StackString<1024> dirname = filename.dirname();
					LOGV("dir: %s, file: %s\n", dirname(), filename.basename().remove_extension()());
					queue.enqueue(filename.remove_extension()());
				}
			}

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
	
	glm::vec3 light_position;
	float current_time;
	scenegraph::Node* ground;
	bool advance_time;

	renderer::RenderTarget* rt;
	
#ifdef USE_WEBSERVER
	CivetServer* server;
#endif

	
	ThreadSafeQueue<std::string> reload_queue;
	
	void process_reload_queue()
	{
		// process queue
		size_t queued_items = reload_queue.size();
		if (queued_items > 0)
		{
			for(size_t i = 0; i < queued_items; ++i)
			{
				std::string item = reload_queue.dequeue();
				// for now, assume everything is a shader.
//				LOGV("processing: %s\n", item.c_str());
				StackString<512> relative_path = item.c_str();
				
				// get the basename to lookup asset library.
				std::string dirname = relative_path.dirname()();
				
				// TODO: replace this with a better mechanism
				if (dirname == "shaders")
				{
					assets::shaders()->load_from_path(item.c_str(), assets::AssetParameters(), true);
				}
				else if (dirname == "models")
				{
					// clear the scene
					root->clear();

					// force mesh reload
					assets::Mesh* mesh = assets::meshes()->load_from_path(item.c_str(), assets::AssetParameters(), true);
					
					//add_mesh_to_root(root, item.c_str(), false);
				}
				else if (dirname == "materials")
				{
					assets::materials()->load_from_path(item.c_str(), assets::AssetParameters(), true);
				}
				else
				{
					LOGW("Reload is not supported for assets in \"%s\"\n", dirname.c_str());
				}
			}
		}
	}
	
	TestRender()
	{
//		camera.type = Camera::TARGET;
		root = 0;
		current_time = 0;
		ground = 0;
		advance_time = 1;
		rt = 0;
		
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
				advance_time = !advance_time;
			}
			else if (event.key == input::KEY_R)
			{
				LOGV("reloading assets...\n");
				assets::materials()->load_from_path("materials/checker", assets::AssetParameters(), true);
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

		glm::mat4 tr = glm::translate(glm::mat4(1.0), node->local_position) *
			glm::toMat4(node->local_rotation) *
			glm::scale(glm::mat4(1.0), node->local_scale);
		
		if (node->parent)
		{
			node->world_transform = node->parent->world_transform * tr * node->local_to_world;
		}
		else
		{
			node->world_transform = tr * node->local_to_world;
		}

//		LOGV("visit: %s\n", node->name());
		
		return 0;
	}

	virtual kernel::ApplicationResult config( kernel::Params & params )
	{
		params.window_title = "test_render";
		params.window_width = 1280;
		params.window_height = 720;
		return kernel::Application_Success;
	}

	void clone_to_scene(scenegraph::Node* template_node, scenegraph::Node* root)
	{
		scenegraph::Node* newnode = template_node->clone();
		root->add_child(newnode);
		
		for (auto child : template_node->children)
		{
			clone_to_scene(child, newnode);
		}
	}

	scenegraph::Node* add_mesh_to_root(scenegraph::Node* root, const char* path, bool build_physics_from_mesh)
	{
		assets::Shader* character = assets::shaders()->load_from_path("shaders/character");
		assets::Shader* world = assets::shaders()->load_from_path("shaders/world");
		
		scenegraph::Node* node = 0;
		
		// load mesh through the asset system
		assets::Mesh* mesh = assets::meshes()->load_from_path(path);
		if (mesh)
		{
			mesh->prepare_geometry();
		
			clone_to_scene(mesh->scene_root, root);
		
//			assets::Shader* shader = nullptr;
//
//			
//			scenegraph::MeshNode* mesh_node = nullptr;
//			scenegraph::SkeletalNode* skel_node = nullptr;
//			
//			if (!mesh->bones.empty())
//			{
//				LOGV("\"%s\" created as a skeletal node\n", path);
//				node = skel_node = CREATE(scenegraph::SkeletalNode);
//				skel_node->mesh = mesh;
//				shader = character;
//			}
//			else
//			{
//				LOGV("\"%s\" created as a mesh node\n", path);
//				node = mesh_node = CREATE(scenegraph::MeshNode);
//				mesh_node->mesh = mesh;
//				shader = world;
//			}
//			
//			assert(shader != nullptr);

			if (build_physics_from_mesh)
			{
				physics::create_physics_for_mesh(mesh);
			}
//
//			if (skel_node)
//			{
//				skel_node->setup_skeleton();
//			}

		}
		else
		{
			LOGW("Unable to load model: %s\n", path);
		}

		return node;
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
		server->addHandler("/reload", new ReloadHandler(reload_queue));
#endif
		
		root = CREATE(scenegraph::Node);
		root->name = "scene_root";
		
//		scenegraph::MeshNode* skydome = 0;
//		skydome = CREATE(scenegraph::MeshNode);
//		assets::Material* colormat = assets::materials()->load_from_path("materials/skydome");
//		skydome->load_mesh("models/skydome", false, 0);
//		// make it extend slightly below ground level
//		skydome->local_position = glm::vec3(0, -50, 0);
//		root->add_child(skydome);

//		add_mesh_to_root(root, "models/future", false);
		scenegraph::Node* ground = add_mesh_to_root(root, "models/plane", false);

//		scenegraph::SkeletalNode* sn = CREATE(scenegraph::SkeletalNode);
//		sn->load_mesh("models/test_yup", false, 0, character);
//		sn->setup_skeleton();
		//sn->local_position = glm::vec3(0, 1, 0);
//		root->add_child(sn);



		

		
		
//		assets::Texture* tex = assets::textures()->allocate_asset();
//		tex->image.width = 512;
//		tex->image.height = 512;
//		tex->image.channels = 3;
//		image::generate_checker_pattern(tex->image, Color(255, 0, 0), Color(0, 255, 0));
//		assets::textures()->take_ownership("render_texture", tex);
//
//		tex->texture = renderer::driver()->texture_create(tex->image);
//		rt = renderer::driver()->render_target_create(tex->image.width, tex->image.height);
//		
//		renderer::driver()->render_target_set_attachment(rt, renderer::RenderTarget::COLOR, 0, tex->texture);

		
		
//		assets::Material* mat = assets::materials()->allocate_asset();
//		assets::Material::Parameter diffusemap;
//		diffusemap.name = "diffusemap";
//		diffusemap.texture_unit = 0;
//		diffusemap.type = assets::MP_SAMPLER_2D;
//		diffusemap.intValue = tex->Id();
//		mat->add_parameter(diffusemap);
//		assets::materials()->take_ownership("render_target", mat);
//
//		LOGV("created rtt material; asset: %i\n", mat->asset_id);

//		assets::Material* def = assets::materials()->get_default();

//		ground = CREATE(scenegraph::MeshNode);
//		ground->load_mesh("models/future", false, 0, world);
//		root->add_child(ground);
//		ground->visible = false;

		debugdraw::startup(1024);

		camera.target_lookatOffset = glm::vec3(0, 0, 1);
		camera.perspective( 50.0f, params.render_width, params.render_height, 0.1f, 32768.0f );
		
		// This is appropriate for drawing 3D models, but not sprites
		camera.set_absolute_position( glm::vec3(1, 1, 1.0f) );
		camera.yaw = -45;
		camera.pitch = 30;
		camera.update_view();

		return kernel::Application_Success;
	}

	virtual void step( kernel::Params & params )
	{
		camera.move_left(input::state()->keyboard().is_down(input::KEY_A));
		camera.move_right(input::state()->keyboard().is_down(input::KEY_D));
		camera.move_forward(input::state()->keyboard().is_down(input::KEY_W));
		camera.move_backward(input::state()->keyboard().is_down(input::KEY_S));
		
		camera.update_view();


		if (advance_time)
		{
			current_time += params.step_interval_seconds;
		}
		
		const float dist = 9.0f;
		float quotient = (current_time * (1.0f/2.0f));
		light_position = glm::vec3(dist*cos(quotient), 3.0f, dist*sin(quotient));

#ifndef SCENE_GRAPH_MANUAL
		if (advance_time)
			root->update(params.step_interval_seconds);
#endif

		debugdraw::sphere(light_position, Color(255, 255, 255, 255), 0.5f, 0.0f);
		debugdraw::axes(glm::mat4(1.0), 1.0f);
		
		debugdraw::text(10, 0, xstr_format("camera.pos = %.2g %.2g %.2g", camera.pos.x, camera.pos.y, camera.pos.z), Color(255, 255, 255));
		debugdraw::text(10, 12, xstr_format("eye_position = %.2g %.2g %.2g", camera.eye_position.x, camera.eye_position.y, camera.eye_position.z), Color(255, 0, 255));
		debugdraw::text(10, 24, xstr_format("camera.view = %.2g %.2g %.2g", camera.view.x, camera.view.y, camera.view.z), Color(128, 128, 255));
		debugdraw::text(10, 36, xstr_format("camera.right = %.2g %.2g %.2g", camera.side.x, camera.side.y, camera.side.z), Color(255, 0, 0));
		debugdraw::text(10, 48, xstr_format("frame_delta = %g", params.framedelta_raw_msec), Color(255, 255, 255));
		debugdraw::text(10, 60, xstr_format("scene graph nodes = %i", total_scene_nodes_visited), Color(128, 128, 255));
		debugdraw::update(params.step_interval_seconds);
	}

	virtual void tick( kernel::Params & params )
	{
		process_reload_queue();

//		renderer::driver()->render_target_activate(rt);
//		{
//			RenderStream s;
//			s.add_viewport( 0, 0, rt->width, rt->height );
//			s.add_clearcolor( 1.0, 0.0, 0.0, 1.0f );
//			s.add_clear( renderer::CLEAR_COLOR_BUFFER );
//			s.run_commands();
//		}
//		renderer::driver()->render_target_deactivate(rt);

		
		renderer::ConstantBuffer cb;
		cb.modelview_matrix = &camera.matCam;
		cb.projection_matrix = &camera.matProj;
		cb.viewer_direction = &camera.view;
		cb.viewer_position = &camera.eye_position;
		cb.light_position = &light_position;
	
		RenderStream rs;
		rs.add_viewport( 0, 0, params.render_width, params.render_height );
		rs.add_clearcolor( 0.1, 0.1, 0.1, 1.0f );
		rs.add_clear( renderer::CLEAR_COLOR_BUFFER | renderer::CLEAR_DEPTH_BUFFER );
		
		// render nodes
		total_scene_nodes_visited = 0;
		scenegraph::visit_nodes(root, this);
		rs.run_commands();
		scenelink.draw(root, cb);
		

		

		
		BaseVar::render_values(10, 72);
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
		renderer::driver()->render_target_destroy(rt);
		DESTROY(Node, root);
		debugdraw::shutdown();
	}
};

IMPLEMENT_APPLICATION( TestRender );