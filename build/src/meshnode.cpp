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
	RenderNode::RenderNode()
	{
		type = RENDER;
	}
	
	MeshNode::MeshNode()
	{
		type = MESH;
		mesh = 0;
		visible = true;
	}
	
	MeshNode::~MeshNode()
	{
		
	}
	
	void MeshNode::load_mesh(const char* path, bool build_physics_from_mesh, assets::Material* material, assets::Shader* shader)
	{
		assert(shader != 0);
		
		// load mesh through the asset system
		mesh = assets::meshes()->load_from_path(path);
		if (mesh)
		{
			// prepare geometry (this uploads data to the gpu)
			mesh->prepare_geometry();
			
			// using this node as the parent node; we create render nodes as children
			
			for (size_t id = 0; id < mesh->geometry.size(); ++id)
			{

				assets::Geometry* geometry = &mesh->geometry[id];
				assets::Material* geometry_material = material;
				
				scenegraph::RenderNode* rn = 0;
				rn = CREATE(scenegraph::RenderNode);
				rn->geometry = geometry;
				rn->material_id = geometry->material_id;
				if (!geometry_material)
				{
					geometry_material = assets::materials()->find_with_id(geometry->material_id);
				}

//				LOGV("shader: (asset_id = %i), (id = %i)\n", shader->Id(), shader->id);
//				shader->show_uniforms();
//				shader->show_attributes();
				rn->material = geometry_material;
				rn->shader = shader;

//				rn->shader_id = shader->Id();


		
				add_child(rn);
			}
			
			if (build_physics_from_mesh)
			{
				physics::create_physics_for_mesh(mesh);
			}
		}
		else
		{
			LOGW("Unable to load model: %s\n", path);
		}
		

	}
}; // namespace scenegraph