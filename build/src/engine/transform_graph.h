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

#include <renderer/scene_renderer.h>


namespace gemini
{
	// This holds the interpolated frame state.
	struct TransformFrameState
	{
		glm::mat4 local_matrices[256];
		int16_t parent_index[256];
	};

	struct Joint;

	struct TransformNode
	{
		TransformNode(gemini::Allocator& allocator);

		Array<TransformNode*> children;

		struct TransformNode* parent;
		glm::mat4 local_matrix;
		glm::mat4 world_matrix;

		gemini::string name;

		// associated entity
		uint16_t entity_index;
		uint16_t transform_index;
	};


	TransformNode* transform_graph_create_node(gemini::Allocator& allocator, const char* node_name);
	TransformNode* transform_graph_create_hierarchy(gemini::Allocator& allocator, FixedArray<gemini::Joint>& skeleton, const char* node_name);
	void transform_graph_destroy_node(gemini::Allocator& allocator, TransformNode* node);
	void transform_graph_set_parent(TransformNode* child, TransformNode* parent);

	// Populate local matrices for entity-backed transform nodes.
	void transform_graph_copy_frame_state(TransformNode* node, TransformFrameState* state);

	// start with the root node
	// transform local matrices to world matrices
	//void transform_graph_transform_root(TransformNode* root, glm::mat4* world_matrices, const glm::mat4* local_matrices, size_t total_matrices);
	void transform_graph_transform(TransformNode* node, glm::mat4* world_matrices);
} // namespace gemini