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
#include <gemini/typedefs.h>
#include <slim/xlog.h>

#include "scene_graph.h"
#include "meshnode.h"

#include "physics.h"

namespace scenegraph
{
	MeshNode::MeshNode()
	{
		type = MESH;
		mesh = 0;
	}
	
	MeshNode::~MeshNode()
	{
		
	}
	
	void MeshNode::load_mesh(const char* path, bool build_physics_from_mesh)
	{
		mesh = assets::meshes()->load_from_path(path);
		if (mesh)
		{
			mesh->prepare_geometry();
		}
		
		if (build_physics_from_mesh)
		{
			physics::create_physics_for_mesh(mesh);
		}
	}
}; // namespace scenegraph