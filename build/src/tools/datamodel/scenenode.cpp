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

#include "datamodel/mesh.h"
#include "datamodel/skeleton.h"
#include "datamodel/scenenode.h"

namespace datamodel
{
	SceneNode::SceneNode()
	{
		parent = nullptr;
		mesh = nullptr;
		skeleton = nullptr;
	}
	
	SceneNode::~SceneNode()
	{
//		SceneNodeVector::iterator it = children.begin();
//		for( ; it != children.end(); ++it)
//		{
//			SceneNode* node = (*it);
//			DESTROY(SceneNode, node);
//		}
		
		for (auto& child : children)
		{
			DESTROY(SceneNode, child);
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
	
	void SceneNode::add_child(SceneNode* child)
	{
		if (child->parent)
		{
			child->parent->remove_child(child);
		}
		
		child->parent = this;
		children.push_back(child);
	}
	
	void SceneNode::remove_child(SceneNode* child)
	{
		// find the child and detach from the old parent
		for (SceneNodeVector::iterator it = children.begin(); it != children.end(); ++it)
		{
			if (child == (*it))
			{
				children.erase(it);
				break;
			}
		}
	}
	
	SceneNode* SceneNode::find_child_named(const std::string& name)
	{
		if (this->name == name)
		{
			return this;
		}

		for (auto& child : children)
		{
			SceneNode* node = child->find_child_named(name);
			if (node)
			{
				return node;
			}
		}
		
		return 0;
	}
};