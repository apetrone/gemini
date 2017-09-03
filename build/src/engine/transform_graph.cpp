// -------------------------------------------------------------
// Copyright (C) 2017- Adam Petrone
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
#include "typedefs.h"

#include <engine/transform_graph.h>
#include <runtime/mesh.h>


namespace gemini
{
	static uint16_t transform_index = USHRT_MAX;

	TransformNode::TransformNode(gemini::Allocator& allocator)
		: children(allocator)
		, bones(allocator)
		, parent(nullptr)
		, entity_index(USHRT_MAX)
		, transform_index(USHRT_MAX)
	{
	}

	TransformNode* transform_graph_create_node(gemini::Allocator& allocator, const char* node_name)
	{
		TransformNode* node = MEMORY2_NEW(allocator, TransformNode)(allocator);
		node->name = string_create(allocator, node_name);
		node->transform_index = transform_index++;
		return node;
	}

	TransformNode* transform_graph_create_hierarchy(gemini::Allocator& allocator, FixedArray<gemini::Joint>& skeleton, const char* node_name)
	{
		TransformNode* animated_node = MEMORY2_NEW(allocator, TransformNode)(allocator);
		animated_node->name = string_create(allocator, node_name);
		animated_node->transform_index = transform_index++;
		animated_node->bones.resize(skeleton.size());

		const size_t total_joints = skeleton.size();
		for (size_t index = 0; index < total_joints; ++index)
		{
			const Joint& joint = skeleton[index];
			TransformNode* child = MEMORY2_NEW(allocator, TransformNode)(allocator);
			child->name = string_create(allocator, joint.name());
			child->transform_index = transform_index++;

			TransformNode* parent = animated_node;
			if (joint.parent_index != -1)
			{
				parent = animated_node->bones[joint.parent_index];
			}
			else
			{
				parent = animated_node;
			}

			animated_node->bones[joint.index] = child;
			transform_graph_set_parent(child, parent);
		}

		return animated_node;
	}

	void transform_graph_destroy_node(gemini::Allocator& allocator, TransformNode* node)
	{
		for (size_t index = 0; index < node->children.size(); ++index)
		{
			transform_graph_destroy_node(allocator, node->children[index]);
		}

		string_destroy(allocator, node->name);
		MEMORY2_DELETE(allocator, node);
	}

	void transform_graph_set_parent(TransformNode* child, TransformNode* parent)
	{
		if (child->parent)
		{
			child->parent->children.erase(child);

			//// Remove child from child->parent.
			//for (size_t index = 0; index < parent->children.size(); ++index)
			//{
			//	if (parent->children[index] == child)
			//	{
			//		// TODO: Array Slice!
			//
			//		break;
			//	}
			//}
		}

		parent->children.push_back(child);
		child->parent = parent;
	}

	void transform_graph_copy_frame_state(TransformNode* node, TransformFrameState* state)
	{
		if (node->entity_index != USHRT_MAX)
		{
			// This transform node represents an entity.
			node->local_matrix = state->local_matrices[node->entity_index];
		}

		for (size_t index = 0; index < node->children.size(); ++index)
		{
			transform_graph_copy_frame_state(node->children[index], state);
		}
	}

	//void transform_graph_transform_root(TransformNode* root, glm::mat4* world_matrices, const glm::mat4* local_matrices, size_t total_matrices)
	//{
	//	// We skip the root node -- because ultimately it lines up with
	//	// the world entity and shouldn't impose any transforms on the children.

	//	for (size_t index = 0; index < root->children.size(); ++index)
	//	{
	//		transform_graph_transform(root->children[index], world_matrices, local_matrices, total_matrices);
	//	}
	//}

	void transform_graph_transform(TransformNode* node, glm::mat4* world_matrices)
	{
		TransformNode* parent = node->parent;
		if (parent)
		{
			node->world_matrix = parent->world_matrix * node->local_matrix;
		}
		else
		{
			node->world_matrix = node->local_matrix;
		}

		if (node->transform_index != USHRT_MAX)
		{
			assert(256 > node->transform_index);
			world_matrices[node->transform_index] = node->world_matrix;
		}

		for (size_t index = 0; index < node->children.size(); ++index)
		{
			transform_graph_transform(node->children[index], world_matrices);
		}
	}
} // namespace gemini
