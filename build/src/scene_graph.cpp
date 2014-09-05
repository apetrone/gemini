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
#include <gemini/util/stackstring.h>

#include <slim/xlog.h>

#include "scene_graph.h"
#include <gemini/mathlib.h>

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
		parent = 0;
		type = NODE;
	}
	
	Node::Node(const Node& other)
	{
		name = other.name;
		type = other.type;
		
		local_scale = other.local_scale;
		local_rotation = other.local_rotation;
		local_position = other.local_position;
		
		local_to_world = other.local_to_world;
		local_to_pivot = other.local_to_pivot;
		
		world_transform = other.world_transform;
		
		parent = other.parent;
	}
	
	Node::~Node()
	{
		clear();
	}
	
	void Node::add_child(Node* child)
	{
		children.push_back(child);
	}
	
	void Node::remove_child(Node* child)
	{
		// TODO: implement
	}
	
	void Node::update(float delta_seconds)
	{
		NodeVector::iterator start, end;
		start = children.begin();
		end = children.end();
		
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
		
		LOGV("[Node] Name=\"%s\" (%x), # Children=%i\n", root->name(), root, root->children.size());
		
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
	
}; // namespace scenegraph