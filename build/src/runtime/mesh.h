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

#include <runtime/animation.h>
#include <runtime/asset_handle.h>
#include <runtime/geometry.h>

#include <core/fixedarray.h>
#include <core/mathlib.h>
#include <core/stackstring.h>
#include <core/typedefs.h>


namespace gemini
{
	const int MAX_VERTEX_WEIGHTS = 4;

	struct Joint
	{
		// -1 == no parent
		int32_t parent_index;
		int32_t index;
		core::StackString<128> name;

		Joint()
		{
			parent_index = -1;
			index = -1;
		}
	};


	struct GeometryDefinition
	{
		uint16_t vertex_offset;
		uint16_t total_vertices;
		uint16_t index_offset;
		uint16_t total_indices;

		AssetHandle material_handle;
		AssetHandle shader_handle;
	}; // GeometryDefinition


	struct Mesh
	{
		Mesh(gemini::Allocator& allocator);

		glm::vec3* vertices;
		glm::vec3* normals;
		glm::vec2* uvs;
		glm::vec4* blend_indices;
		glm::vec4* blend_weights;

		glm::mat4* bind_poses;
		glm::mat4* inverse_bind_poses;

		uint16_t* indices;

		// geometry definitions for this mesh
		FixedArray<GeometryDefinition> geometry;

		// list of animation sequences associated with this mesh
		FixedArray<animation::SequenceId> sequences;

		// offset to the center of mass
		glm::vec3 mass_center_offset;
		glm::vec3 aabb_mins;
		glm::vec3 aabb_maxs;

		// bind pose skeleton
		FixedArray<Joint> skeleton;
		FixedArray<Hitbox> hitboxes;
	}; // Mesh

	void mesh_init(Allocator& allocator, Mesh* mesh, uint32_t total_vertices, uint32_t total_indices, uint32_t total_bones);
	void mesh_destroy(Allocator& allocator, Mesh* mesh);
	void mesh_stats(Mesh* mesh, uint32_t& total_vertices, uint32_t& total_indices);
	Joint* mesh_find_bone_named(Mesh* mesh, const char* name);

} // namespace gemini

