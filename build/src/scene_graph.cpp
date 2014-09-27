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
#include <vector>
#include <algorithm>
#include <string>

#include <gemini/typedefs.h>
#include <gemini/mem.h>

#include <slim/xlog.h>

#include "scene_graph.h"
#include <gemini/mathlib.h>

#include "core/assets/asset_mesh.h"
#include "physics.h"

// Useful to hand to a std::for_each statement in order to clean up a container.
template <class _Type>
struct DestroyPointer
{
	void operator()(_Type * p)
	{
		DESTROY(_Type, p);
	}
}; // DestroyPointer

namespace scenegraph
{
	Node::Node()
	{
		// sane defaults
		local_scale = glm::vec3(1.0f, 1.0f, 1.0f);
		scale = glm::vec3(1.0f, 1.0f, 1.0f);
		parent = 0;
		type = NODE;
		attributes = 0;
	}
	
	Node::Node(const Node& other)
	{
		name = other.name;
		type = other.type;
		attributes = other.attributes;
		
		local_scale = other.local_scale;
		local_rotation = other.local_rotation;
		local_position = other.local_position;
		
		local_to_world = other.local_to_world;
		local_to_pivot = other.local_to_pivot;
		
		scale = other.scale;
		rotation = other.rotation;
		translation = other.translation;
		
		world_transform = other.world_transform;
		
		parent = other.parent;
	}
	
	Node::~Node()
	{
		clear();
	}
	
	void Node::add_child(Node* child)
	{
		if (child->parent && child->parent != this)
		{
			child->parent->remove_child(child);
		}
		child->parent = this;
		children.push_back(child);
	}
	
	Node* Node::find_child_named(const std::string& name)
	{
		if (this->name == name)
		{
			return this;
		}
		
		for (auto& child : children)
		{
			Node* node = child->find_child_named(name);
			if (node)
			{
				return node;
			}
		}
		
		return 0;
	}
	
	void Node::remove_child(Node* node)
	{
		auto it = children.begin();
		for ( ; it != children.end(); ++it)
		{
			if ((*it) == node)
			{
				children.erase(it);
				break;
			}
		}
	}
	
	void Node::update(float delta_seconds)
	{
		NodeVector::iterator start, end;
		start = children.begin();
		end = children.end();
		
		glm::mat4 sc = glm::scale(glm::mat4(1.0), scale);
		glm::mat4 ro = glm::toMat4(rotation);
		glm::mat4 tr = glm::translate(glm::mat4(1.0), translation);
		
		glm::mat4 inv_pivot = glm::translate(glm::mat4(1.0), local_position);
		glm::mat4 pivot = glm::translate(glm::mat4(1.0), -local_position);
	
		assert(scale.x != 0 && scale.y != 0 && scale.z != 0);
		local_to_world = inv_pivot * sc * ro * pivot * tr;


		if (parent)
		{
			world_transform = parent->world_transform * local_to_world;
		}
		else
		{
			world_transform = local_to_world;
		}
		
		while (start != end)
		{
			(*start)->update(delta_seconds);
			++start;
		}
	}
	
	void Node::clear()
	{
		std::for_each(children.begin(), children.end(), DestroyPointer<Node>());
		children.clear();
	}
	
	
	void create_scene(Node* root)
	{
		
	}
	
	void recursive_visit_nodes(Node* root, Visitor* visitor)
	{
		visitor->visit(root);
		NodeVector::iterator start, end;
		start = root->children.begin();
		end = root->children.end();
		
		visitor->visit((root));
		
		while (start != end)
		{
			recursive_visit_nodes(*start, visitor);
			++start;
		}
	}
	
	void visit_nodes(Node* root, Visitor* visitor)
	{
		recursive_visit_nodes(root, visitor);
	}
	
	void destroy_scene(Node* root)
	{
		
	}
	
	static void print_tree_node(Node* root, uint32_t indent_depth)
	{
		std::string indent;
		for (uint32_t i = 0; i < indent_depth; ++i)
		{
			indent += "\t";
		}
		
		LOGV("[Node] Name=\"%s\" (%x), # Children=%i\n", root->name.c_str(), root, root->children.size());
		
		NodeVector::iterator start, end;
		start = root->children.begin();
		end = root->children.end();
		
		while(start != end)
		{
			print_tree_node((*start), (indent_depth+1));
			++start;
		}
	}
	
	void print_tree(Node* root)
	{
		print_tree_node(root, 0);
	}
	
	
	scenegraph::Node* clone_to_scene(scenegraph::Node* template_node, scenegraph::Node* root)
	{
		scenegraph::Node* newnode = template_node->clone();
		root->add_child(newnode);
		
		for (auto child : template_node->children)
		{
			clone_to_scene(child, newnode);
		}
		
		return newnode;
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
			
			node = clone_to_scene(mesh->scene_root, root);
			
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
	
}; // namespace scenegraph