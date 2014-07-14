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

#include "mathlib.h"

namespace scenegraph
{
	typedef std::vector<struct Node*, GeminiAllocator<Node*>> NodeVector;
	struct Node
	{
		// decomposed pieces
		glm::vec3 translation;
		glm::quat rotation;
		glm::vec3 scale;
		
		// the local to world transform
		glm::mat4 local_to_world;
		
		// the final world-transform for this model
		glm::mat4 world_transform;
		
		StackString<128> name;
		
		NodeVector children;
		
		Node();
		virtual ~Node() {}
		
		void add_child(Node* child);
		void remove_child(Node* child);
		void update(float delta_seconds);
	};
	
	
	
	void create_scene(Node* root);
	void visit(Node* root, void* visitor);
	void destroy_scene(Node* root);
}; // namespace scenegraph


