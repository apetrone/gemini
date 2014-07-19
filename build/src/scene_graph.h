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

#include <gemini/util/stackstring.h>

#include "mathlib.h"
#include "keyframechannel.h"


namespace scenegraph
{
	typedef std::vector< struct Node*, GeminiAllocator<Node*> > NodeVector;
	
	enum NodeType
	{
		SCENEROOT, 		// the scene root
		TRANSFORM, 		// generic transform
		MESH,			// geometry/mesh node
	};
	
	struct Node
	{
		// decomposed pieces
		glm::vec3 local_position;
		glm::quat local_rotation;
		glm::vec3 local_scale;
		
		// the local to world transform
		glm::mat4 local_to_world;
		
		// local to pivot point vector
		glm::vec3 local_to_pivot;
		
		// the final world-transform for this model
		glm::mat4 world_transform;
		
		StackString<128> name;
		
		NodeVector children;
		Node* parent;
		
		NodeType type;
	
		Node();
		virtual ~Node();
		
		void add_child(Node* child);
		void remove_child(Node* child);
		virtual void update(float delta_seconds);
		NodeType get_type() const { return type; }
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
}; // namespace scenegraph


