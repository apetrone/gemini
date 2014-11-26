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
#pragma once

#include <vector>

#include <core/mathlib.h>
#include "keyframechannel.h"

namespace scenegraph
{
	typedef std::vector< struct Node*, GeminiAllocator<Node*> > NodeVector;
	
	enum NodeType
	{
		SCENEROOT 		= 0, 	// the scene root
		NODE			= 1, 	// generic node
		STATIC_MESH		= 2,	// geometry/mesh node
		RENDER			= 4,	// is a render node
		SKELETON		= 8,	// render a skeleton
		ANIMATED		= 16,	// has animatable position/rotation/scale
	};
	
	struct Node
	{
		typedef uint16_t AttributeType;
	
		// decomposed pieces
		glm::vec3 local_scale;
		glm::quat local_rotation;
		glm::vec3 local_position;
		
		// the local to world transform
		glm::mat4 local_to_world;
		
		// local to pivot point vector
		glm::vec3 local_to_pivot;
		
		// the final world-transform for this model
		glm::mat4 world_transform;

		glm::vec3 scale;
		glm::quat rotation;
		glm::vec3 translation;

		String name;
		
		NodeVector children;
		Node* parent;
		
		NodeType type;
		AttributeType attributes;

		Node();
		Node(const Node& other);
		virtual ~Node();
		
		void add_child(Node* child);
		void remove_child(Node* child);
		Node* find_child_named(const String& name);
		virtual void update(float delta_seconds);
		void update_transforms();
		void clear();
		NodeType get_type() const { return type; }
		
		bool has_attributes(AttributeType flags) { return (flags & attributes); }
		virtual Node* clone() { return CREATE(Node, *this); }
	};

	struct Visitor
	{
		virtual ~Visitor() {}
		virtual int32_t visit(Node* node) = 0;
	};

	
	void create_scene(Node* root);
	void visit_nodes(Node* root, Visitor* visitor);
	void destroy_scene(Node* root);
	void print_tree(Node* root);
	
	// TODO: move these?
	scenegraph::Node* clone_to_scene(scenegraph::Node* template_node, scenegraph::Node* root);
}; // namespace scenegraph


