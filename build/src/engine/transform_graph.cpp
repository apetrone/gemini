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


namespace gemini
{
	TransformNode::TransformNode(gemini::Allocator& allocator)
		: children(allocator)
		, parent(nullptr)
		, data_index(0)
	{
	}

	TransformNode* transform_graph_create_node(gemini::Allocator& allocator, const char* node_name)
	{
		TransformNode* node = MEMORY2_NEW(allocator, TransformNode)(allocator);
		node->name = string_create(allocator, node_name);
		return node;
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
			// Remove child from child->parent.
			for (size_t index = 0; index < parent->children.size(); ++index)
			{
				if (parent->children[index] == child)
				{
					// TODO: Array Slice!
					break;
				}
			}
		}

		parent->children.push_back(child);
		child->parent = parent;
	}

	void transform_graph_transform(TransformNode* root, glm::mat4* world_matrices, const glm::mat4* local_matrices, size_t total_matrices)
	{
		TransformNode* parent = root->parent;
		if (parent)
		{
			world_matrices[root->data_index] = world_matrices[parent->data_index] * local_matrices[root->data_index];
			//root->world_matrix = parent->world_matrix * root->local_matrix;
		}
		else
		{
			world_matrices[root->data_index] = local_matrices[root->data_index];
			//root->world_matrix = root->local_matrix;
		}

		for (size_t index = 0; index < root->children.size(); ++index)
		{
			transform_graph_transform(root->children[index], world_matrices, local_matrices, total_matrices);
		}
	}

	void transform_graph_extract(TransformNode* root, EntityRenderState* entity_state)
	{
	}
} // namespace gemini
