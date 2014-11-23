// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone

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

#include <platform/typedefs.h>
#include <platform/mem.h>
#include <core/mathlib.h>
#include <vector>
#include <string>


namespace datamodel
{
	struct Mesh;
	struct Skeleton;

	typedef std::vector<struct Node*, GeminiAllocator<struct Node*>> NodeVector;

	
	struct Node
	{
		enum Flags
		{
			None,
			NoAnimations, // this node cannot have animations
		};
		
		uint32_t flags;
		
		std::string name;
		std::string type;
		
		glm::vec3 scale;
		glm::quat rotation;
		glm::vec3 translation;

		Node* parent;
		NodeVector children;
		
		Mesh* mesh;
		Skeleton* skeleton;
		
		Node();
		virtual ~Node();
		void add_child(Node* child);
		void remove_child(Node* child);
		Node* find_child_named(const std::string& name);
		
		bool has_animations() const { return (flags & NoAnimations) == 0; }
	};
};