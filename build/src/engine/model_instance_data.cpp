// -------------------------------------------------------------
// Copyright (C) 2017- Adam Petrone
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//      * Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.

//      * Redistributions in binary form must reproduce the above copyright notice,
//      this list of conditions and the following disclaimer in the documentation
//      and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//       SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------
#include <engine/model_instance_data.h>

#include <runtime/mesh.h>

#include <core/typedefs.h>

#include <shared/shared_constants.h>



namespace gemini
{
	ModelInstanceData::ModelInstanceData(gemini::Allocator& allocator)
		: mesh(nullptr)
		, component_index(0)
		, allocator(allocator)
	{

	}

	void ModelInstanceData::set_mesh_index(AssetHandle in_mesh_handle)
	{
		mesh_handle = in_mesh_handle;
		mesh = mesh_from_handle(mesh_handle);
	}

	AssetHandle ModelInstanceData::get_mesh_handle() const
	{
		return mesh_handle;
	}

	void ModelInstanceData::set_component_index(uint32_t in_component_index)
	{
		component_index = in_component_index;
	}

	AssetHandle ModelInstanceData::asset_index() const { return mesh_handle; }

	const Hitbox* ModelInstanceData::get_hitboxes() const
	{
		return &mesh->hitboxes[0];
	}

	void ModelInstanceData::set_animation_enabled(int32_t index, bool enabled)
	{
		assert(0); // I don't think this is used anymore.
		animation::SequenceId global_instance_index = animations[index];
		animation::AnimatedInstance* instance = animation::get_instance_by_index(global_instance_index);
		assert(instance != 0);
		if (instance)
		{
			instance->flags = animation::AnimatedInstance::Flags::Playing;
		}
	}

	float ModelInstanceData::get_animation_duration(int32_t index) const
	{
		float duration_seconds = 0;
#if 0
		animation::SequenceId instance_index = animations[index];
		animation::AnimatedInstance* instance = animation::get_instance_by_index(instance_index);
		assert(instance != 0);

		animation::Sequence* sequence = animation::get_sequence_by_index(instance->sequence_index);
		assert(sequence != 0);

		duration_seconds = sequence->duration_seconds;

#endif
		return duration_seconds;
	}

	const glm::vec3& ModelInstanceData::get_mins() const
	{
		return mesh->aabb_mins;
	}

	const glm::vec3& ModelInstanceData::get_maxs() const
	{
		return mesh->aabb_maxs;
	}

	const glm::vec3& ModelInstanceData::get_center_offset() const
	{
		return mesh->mass_center_offset;
	}

	void ModelInstanceData::set_transform_node(TransformNode* node)
	{
		transform_node = node;
	} // set_transform_node

	TransformNode* ModelInstanceData::get_transform_node() const
	{
		return transform_node;
	} // get_transform_node
} // namespace gemini