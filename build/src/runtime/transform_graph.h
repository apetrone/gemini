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
#pragma once

#include <core/array.h>
#include <core/mathlib.h>
#include <core/str.h>
#include <core/typedefs.h>


namespace gemini
{
	// This holds the interpolated frame state.
	struct TransformFrameState
	{
		glm::vec3 position[256];
		glm::quat orientation[256];
		glm::vec3 pivot_point[256];
	};

	struct Joint;
	struct ModelAttachment;

	struct TransformNode
	{
		TransformNode(gemini::Allocator& allocator);

		Array<TransformNode*> children;

		// a flat list of Nodes that this node owns.
		Array<TransformNode*> bones;

		struct TransformNode* parent;

		// Individual components before aggregation into local_matrices.
		glm::vec3 position;
		glm::quat orientation;
		glm::vec3 pivot_point;

		// These are the node local and world matrices. Updated with
		// transform_graph_transform.
		glm::mat4 local_matrix;
		glm::mat4 world_matrix;

		// Used to post-multiply local matrices; before local->world multiply
		glm::mat4 bind_pose_matrix;

		// model-space bone matrix. This is multiplied with bone parent nodes, but
		// does not multiply past the local TransformNode.
		glm::mat4 model_bone_matrix;

		gemini::string name;

		// associated entity
		uint16_t entity_index;
		uint16_t transform_index;

		// visit function for graph nodes; changes based on behavior of
		// this node.
		void(*transform_function)(TransformNode* node, glm::mat4* world_matrices);
	}; // TransformNode

	TransformNode* transform_graph_create_node(gemini::Allocator& allocator, const char* node_name);
	TransformNode* transform_graph_create_hierarchy(gemini::Allocator& allocator, const FixedArray<gemini::Joint>& skeleton, const Array<gemini::ModelAttachment*>& attachments, const char* node_name);
	void transform_graph_destroy_node(gemini::Allocator& allocator, TransformNode* node);
	void transform_graph_set_parent(TransformNode* child, TransformNode* parent);

	// Populate local matrices for entity-backed transform nodes.
	void transform_graph_copy_frame_state(TransformNode* node, TransformFrameState* state);

	void transform_graph_transform(TransformNode* node, glm::mat4* world_matrices);

	void transform_graph_print(TransformNode* root, uint32_t indent = 0);
} // namespace gemini