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

#include <gemini/typedefs.h>
#include <gemini/mem.h>
#include <gemini/mathlib.h>
#include <vector>
#include <string>



namespace datamodel
{
	struct Mesh;
	struct Skeleton;

	typedef std::vector<struct SceneNode*, GeminiAllocator<struct SceneNode*>> SceneNodeVector;
	
	struct SceneNode
	{
		std::string name;
		std::string type;
		
		glm::vec3 scale;
		glm::quat rotation;
		glm::vec3 translation;

		SceneNode* parent;
		SceneNodeVector children;
		
		Mesh* mesh;
		Skeleton* skeleton;
		
		SceneNode();
		virtual ~SceneNode();
		void add_child(SceneNode* child);
		void remove_child(SceneNode* child);
	};
};