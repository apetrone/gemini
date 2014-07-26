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
#include <algorithm>

#include <slim/xlog.h>

#include "scenelink.h"
#include "meshnode.h"

#include "renderstream.h"

namespace renderer
{
	class SceneVisitor : public scenegraph::Visitor
	{
		size_t total_nodes;
		RenderQueue& queue;

	public:
		SceneVisitor(RenderQueue& _queue): queue(_queue) {}
	
		virtual int visit(scenegraph::Node* node)
		{
			++total_nodes;
			if (node->type == scenegraph::MESH)
			{
				scenegraph::MeshNode* meshnode = static_cast<scenegraph::MeshNode*>(node);
				if (meshnode)
				{
					for (auto child : meshnode->children)
					{
						scenegraph::RenderNode* rn = static_cast<scenegraph::RenderNode*>(child);

						RenderKey key = compute_render_key(rn->geometry);
						RenderBlock block(key, rn->geometry);
						block.object_matrix = &meshnode->world_transform;
						block.material_id = rn->material_id;
						block.shader = rn->shader;
//						block.shader_id = rn->shader_id;

						queue.insert(block);
					}
				}
			}
			return 0;
		}
		
	private:
		RenderKey compute_render_key(RenderObject* object)
		{
			return object->attributes;
		}
	};
	
	
	SceneLink::SceneLink()
	{
		queue = CREATE(RenderQueue);
	}
	SceneLink::~SceneLink()
	{
		DESTROY(RenderQueue, queue);
	}

	void SceneLink::draw(scenegraph::Node* root, const glm::mat4& modelview_matrix, const glm::mat4& projection_matrix)
	{
		// clear the queue and prepare for another frame
		queue->clear();
		
		// visit all nodes in the scene graph
		// add renderable nodes to the queue
		SceneVisitor visitor(*queue);
		scenegraph::visit_nodes(root, &visitor);
	
		// sort the queue
		queue->sort();
		
		
		RenderStream rs;
		ConstantBuffer cb;
		cb.modelview_matrix = &modelview_matrix;
		cb.projection_matrix = &projection_matrix;
		
		// finally, draw from the queue
		for(auto& block : queue->render_list)
		{
			render_utilities::queue_geometry(rs, block, cb);
		}
		
		rs.run_commands();
	}
}; // namespace renderer