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
#include "skeletalnode.h"

#include <renderer/renderstream.h>


#include <assets/asset_shader.h>
#include <assets/asset_material.h>

namespace renderer
{
	class SceneVisitor : public scenegraph::Visitor
	{
		size_t total_nodes;
		RenderQueue& queue;

	public:
		SceneVisitor(RenderQueue& _queue): queue(_queue) {}

		void queue_render_node(scenegraph::RenderNode* render_node, glm::mat4* transforms = 0, uint8_t total_transforms = 0)
		{
			RenderKey key = compute_render_key(render_node->geometry);
			RenderBlock block(key, render_node->geometry);
			block.object_matrix = &render_node->world_transform;
			block.material_id = render_node->material_id;
			block.shader_id = render_node->shader_id;
			
			block.node_transforms = transforms;
			block.total_transforms = total_transforms;
			
			queue.insert(block);
		}
		
		void queue_mesh(scenegraph::Node* node)
		{
			scenegraph::RenderNode* render_node = static_cast<scenegraph::RenderNode*>(node);
			if (!render_node)
			{
				return;
			}
			
			queue_render_node(render_node);
		}
		
		void queue_skeleton(scenegraph::Node* node)
		{
			// TODO: implement!
			
			// there will be a render node child: this contains the mesh as normal.
			// there will be AnimatedNode(s) corresponding to the bones.
			scenegraph::SkeletalNode* skeleton = static_cast<scenegraph::SkeletalNode*>(node);
			
			// If you hit this assert, a non-animated model was loaded via SkeletonNode.
			assert(!skeleton->final_transforms.empty());
			scenegraph::RenderNode* rn = static_cast<scenegraph::RenderNode*>(node->children[0]);
			if (rn && rn->get_type() == scenegraph::RENDER)
			{
				queue_render_node(rn, &skeleton->final_transforms[0], skeleton->final_transforms.size());
			}
		}
		
		virtual int visit(scenegraph::Node* node)
		{
			++total_nodes;
			if (node->type == scenegraph::STATIC_MESH)
			{
				queue_mesh(node);
			}
			else if (node->type == scenegraph::SKELETON)
			{
				queue_skeleton(node);
			}
			else
			{
//				LOGW("node->type of %i is unhandled. Will not render!\n", node->type);
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

	void SceneLink::draw(scenegraph::Node* root, ConstantBuffer& constant_buffer)
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
		
		// finally, draw from the queue
		for(auto& block : queue->render_list)
		{
			assets::Shader* shader = assets::shaders()->find_with_id(block.shader_id);
			assets::Material* material = assets::materials()->find_with_id(block.material_id);
			
			//render_utilities::queue_geometry(rs, block, constant_buffer, 0, 0);
			

			
			rs.add_shader(shader->program);
			
			rs.add_uniform_matrix4(shader->program->get_uniform_location("modelview_matrix"), constant_buffer.modelview_matrix);
			rs.add_uniform_matrix4(shader->program->get_uniform_location("projection_matrix"), constant_buffer.projection_matrix);
			rs.add_uniform_matrix4(shader->program->get_uniform_location("object_matrix"), block.object_matrix);
			
//			if (block.total_transforms > 0)
//			{
//				rs.add_uniform_matrix4(shader->get_uniform_location("node_transforms"), block.node_transforms, block.total_transforms);
//			}
			

			if (constant_buffer.viewer_direction)
			{
				rs.add_uniform3f(shader->program->get_uniform_location("viewer_direction"), constant_buffer.viewer_direction);
			}
			
			if (constant_buffer.viewer_position)
			{
				rs.add_uniform3f(shader->program->get_uniform_location("viewer_position"), constant_buffer.viewer_position);
			}
			
			if (constant_buffer.light_position)
			{
				rs.add_uniform3f(shader->program->get_uniform_location("light_position"), constant_buffer.light_position);
			}
			
			rs.add_material(material, shader->program);
			
			rs.add_draw_call(block.object->vertexbuffer);
		}
		
		rs.run_commands();
	}
}; // namespace renderer