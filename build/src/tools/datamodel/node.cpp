// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone
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

#include "datamodel/mesh.h"
#include "datamodel/skeleton.h"
#include "datamodel/node.h"

namespace gemini
{
	namespace datamodel
	{
		Node::Node()
		{
			parent = nullptr;
			mesh = nullptr;
			skeleton = nullptr;
			flags = 0;
		}
		
		Node::~Node()
		{
	//		NodeVector::iterator it = children.begin();
	//		for( ; it != children.end(); ++it)
	//		{
	//			Node* node = (*it);
	//			DESTROY(Node, node);
	//		}
			
			for (auto& child : children)
			{
				DESTROY(Node, child);
			}
			
			children.clear();
			
			if (mesh)
			{
				DESTROY(Mesh, mesh);
			}
			
			if (skeleton)
			{
				DESTROY(Skeleton, skeleton);
			}
		}
		
		void Node::add_child(Node* child)
		{
			if (child->parent)
			{
				child->parent->remove_child(child);
			}
			
			child->parent = this;
			children.push_back(child);
		}
		
		void Node::remove_child(Node* child)
		{
			// find the child and detach from the old parent
			for (NodeVector::iterator it = children.begin(); it != children.end(); ++it)
			{
				if (child == (*it))
				{
					children.erase(it);
					break;
				}
			}
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
	} // namespace datamodel
} // namespace gemini