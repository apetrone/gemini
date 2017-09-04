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

#include <core/logging.h>

#include <runtime/mesh.h>
#include <runtime/transform_graph.h>


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
	} // transform_graph_create_node

	TransformNode* transform_graph_create_hierarchy(gemini::Allocator& allocator, const FixedArray<gemini::Joint>& skeleton, const Array<gemini::ModelAttachment*>& attachments, const char* node_name)
	{
		const size_t total_joints = skeleton.size();

		TransformNode* animated_node = MEMORY2_NEW(allocator, TransformNode)(allocator);
		animated_node->name = string_create(allocator, node_name);
		animated_node->transform_index = transform_index++;
		animated_node->bones.resize(total_joints);

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

		// These should be additional named nodes inserted as children
		// to the skeleton.
		for (size_t index = 0; index < attachments.size(); ++index)
		{
			ModelAttachment* attachment = attachments[index];
			TransformNode* parent_node = animated_node->bones[attachment->bone_index];

			TransformNode* attachment_node = MEMORY2_NEW(allocator, TransformNode)(allocator);
			attachment_node->name = string_create(allocator, attachment->name.c_str());
			attachment_node->position = attachment->local_translation_offset;
			attachment_node->orientation = attachment->local_orientation_offset;
			transform_graph_set_parent(attachment_node, parent_node);
		}

		return animated_node;
	} // transform_graph_create_hierarchy

	void transform_graph_destroy_node(gemini::Allocator& allocator, TransformNode* node)
	{
		for (size_t index = 0; index < node->children.size(); ++index)
		{
			transform_graph_destroy_node(allocator, node->children[index]);
		}

		string_destroy(allocator, node->name);
		MEMORY2_DELETE(allocator, node);
	} // transform_graph_destroy_node

	void transform_graph_set_parent(TransformNode* child, TransformNode* parent)
	{
		if (child->parent == parent)
		{
			return;
		}

		if (child->parent)
		{
			child->parent->children.erase(child);
		}

		parent->children.push_back(child);
		child->parent = parent;
	} // transform_graph_set_parent

	void transform_graph_copy_frame_state(TransformNode* node, TransformFrameState* state)
	{
		if (node->entity_index != USHRT_MAX)
		{
			// This transform node represents an entity.
			node->position = state->position[node->entity_index];
			node->orientation = state->orientation[node->entity_index];
			node->pivot_point = state->pivot_point[node->entity_index];
		}

		for (size_t index = 0; index < node->children.size(); ++index)
		{
			transform_graph_copy_frame_state(node->children[index], state);
		}
	} // transform_graph_copy_frame_state

	void transform_graph_transform(TransformNode* node, glm::mat4* world_matrices)
	{
		glm::mat4 rotation = glm::toMat4(node->orientation);
		glm::mat4 translation = glm::translate(glm::mat4(1.0f), node->position);
		glm::mat4 to_pivot = glm::translate(glm::mat4(1.0f), -node->pivot_point);
		glm::mat4 from_pivot = glm::translate(glm::mat4(1.0f), node->pivot_point);
		node->local_matrix = node->bind_pose_matrix * translation * from_pivot * rotation * to_pivot;

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
			world_matrices[node->transform_index] = node->world_matrix;
		}

		for (size_t index = 0; index < node->children.size(); ++index)
		{
			transform_graph_transform(node->children[index], world_matrices);
		}
	} // transform_graph_transform
} // namespace gemini
