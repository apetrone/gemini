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
#include <slim/xlog.h>

#include <algorithm>


#include "scenelink.h"
#include "meshnode.h"

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
				// this is renderable
				scenegraph::MeshNode* meshnode = static_cast<scenegraph::MeshNode*>(node);
				if (meshnode)
				{
					for(size_t geo = 0; geo < meshnode->mesh->total_geometry; ++geo)
					{
						assets::Geometry* geometry = &meshnode->mesh->geometry[geo];
						
						// TODO: determine render key for this geometry.
						RenderKey key = geometry->attributes /*plus some other stuff */;
						queue.insert(key, geometry);
					}
				}
			}
			return 0;
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

	void SceneLink::draw(scenegraph::Node* root)
	{
		// clear the queue and prepare for another frame
		queue->clear();
		
		// visit all nodes in the scene graph
		// add renderable nodes to the queue
		SceneVisitor visitor(*queue);
		scenegraph::visit_nodes(root, &visitor);
	
		// sort the queue
		queue->sort();
		
		// finally, draw from the queue
		queue->draw();
	}
}; // namespace renderer