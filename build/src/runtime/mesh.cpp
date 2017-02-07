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
#include <runtime/mesh.h>

namespace gemini
{
	Mesh::Mesh(gemini::Allocator& _allocator)
		: vertices(nullptr)
		, normals(nullptr)
		, uvs(nullptr)
		, blend_indices(nullptr)
		, blend_weights(nullptr)
		, indices(nullptr)
		, bind_poses(nullptr)
		, inverse_bind_poses(nullptr)
		, geometry(_allocator)
		, sequences(_allocator)
		, sequence_index_by_name(_allocator)
		, skeleton(_allocator)
		, hitboxes(_allocator)
	{
	} // Mesh

	void mesh_init(Allocator& allocator, Mesh* mesh, uint32_t total_vertices, uint32_t total_indices, uint32_t total_bones)
	{
		size_t vertex_data_size = 0;

		assert(total_vertices > 0);
		assert(total_indices > 0);

		const size_t vec2_size = sizeof(glm::vec2) * total_vertices;
		const size_t vec3_size = sizeof(glm::vec3) * total_vertices;
		const size_t vec4_size = sizeof(glm::vec4) * total_vertices;

		// vertices
		vertex_data_size += vec3_size;

		// normals
		vertex_data_size += vec3_size;

		// uvs
		vertex_data_size += vec2_size;

		// blend indices
		vertex_data_size += vec4_size;

		// blend weights
		vertex_data_size += vec4_size;

		// setup points
		uint8_t* mem = static_cast<uint8_t*>(MEMORY2_ALLOC(allocator, vertex_data_size));
		memset(mem, 0, vertex_data_size);
		mesh->vertices = reinterpret_cast<glm::vec3*>(mem);
		mesh->normals = reinterpret_cast<glm::vec3*>(mem + vec3_size);
		mesh->uvs = reinterpret_cast<glm::vec2*>(mem + vec3_size + vec3_size);
		mesh->blend_indices = reinterpret_cast<glm::vec4*>(mem + vec3_size + vec3_size + vec2_size);
		mesh->blend_weights = reinterpret_cast<glm::vec4*>(mem + vec3_size + vec3_size + vec2_size + vec4_size);

		mesh->indices = static_cast<index_t*>(MEMORY2_ALLOC(allocator, sizeof(index_t) * total_indices));
		memset(mesh->indices, 0, sizeof(index_t) * total_indices);

		// setup skeleton data
		if (total_bones > 0)
		{
			uint8_t* data = static_cast<uint8_t*>(MEMORY2_ALLOC(allocator, sizeof(glm::mat4) * MAX_BONES * 2));
			mesh->bind_poses = reinterpret_cast<glm::mat4*>(data);
			mesh->inverse_bind_poses = reinterpret_cast<glm::mat4*>(data + (sizeof(glm::mat4) * MAX_BONES));
		}
	} // mesh_init

	void mesh_destroy(Allocator& allocator, Mesh* mesh)
	{
		MEMORY2_DEALLOC(allocator, mesh->vertices);
		MEMORY2_DEALLOC(allocator, mesh->indices);
		MEMORY2_DEALLOC(allocator, mesh->bind_poses);
	} // mesh_destroy

	void mesh_stats(Mesh* mesh, uint32_t& total_vertices, uint32_t& total_indices)
	{
		for (size_t index = 0; index < mesh->geometry.size(); ++index)
		{
			total_vertices += mesh->geometry[index].total_vertices;
			total_indices += mesh->geometry[index].total_indices;
		}
	} // mesh_stats

	Joint* mesh_find_bone_named(Mesh* mesh, const char* name)
	{
		core::StackString<128> target_name(name);

		for (Joint& j : mesh->skeleton)
		{
			if (j.name == target_name)
			{
				return &j;
			}
		}
		return 0;
	} // mesh_find_bone_named
} // namespace gemini
