// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//		* Redistributions of source code must retain the above copyright notice,
//		this list of conditions and the following disclaimer.

//		* Redistributions in binary form must reproduce the above copyright notice,
//		this list of conditions and the following disclaimer in the documentation
//		and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//		 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------
#include <vector>
#include <algorithm>
#include <string>

#include <platform/mem.h>

#include <core/typedefs.h>
#include <core/logging.h>
#include <core/mathlib.h>

#include "scene_graph.h"


//#include "core/assets/asset_mesh.h"
//#include "physics.h"

// Useful to hand to a std::for_each statement in order to clean up a container.
template <class _Type>
struct DestroyPointer
{
	void operator()(_Type * p)
	{
		MEMORY_DELETE(p, core::memory::global_allocator());
	}
}; // DestroyPointer

namespace gemini
{
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

		Node* Node::find_child_named(const String& name)
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

		}

		void Node::update_transforms()
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
	//		local_to_world = inv_pivot * sc * ro * pivot * tr;
			local_to_world = tr * pivot * ro * sc * inv_pivot;
	//		local_to_world = tr * ro * sc;

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
				(*start)->update_transforms();
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
			String indent;
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
	} // namespace scenegraph
} // namespace gemini
