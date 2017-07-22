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
		, local_bone_transforms(0)
		, model_bone_transforms(0)
		, component_index(0)
		//, inverse_bind_transforms(0)
		//, scale_channel(scale)
		//, rotation_channel(rotation)
		//, translation_channel(translation)
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
	//glm::mat4& ModelInstanceData::get_local_transform() { return transform; }
	//void ModelInstanceData::set_local_transform(const glm::mat4& _transform) { transform = _transform; }

	const Hitbox* ModelInstanceData::get_hitboxes() const
	{
		if (!model_bone_transforms)
		{
			return nullptr;
		}

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

#if 0
	void ModelInstanceData::set_pose(glm::vec3* positions, glm::quat* rotations)
	{
		// THIS IS DEPRECATED. Replace with new animation code.
		assert(0);

		if (mesh->skeleton.empty())
		{
			return;
		}

		// You've hit the upper bounds for skeletal bones for a single
		// model. Congrats.
		assert(mesh->skeleton.size() < MAX_BONES);

		Hitbox* hitboxes = &mesh->hitboxes[0];

		size_t geometry_index = 0;
		// we must update the transforms for each geometry instance
		for (const ::renderer::Geometry* geo : mesh->geometry)
		{
			// If you hit this assert, one mesh in this model didn't have
			// blend weights.
			assert(!geo->bind_poses.empty());

			size_t transform_index;

			for (size_t index = 0; index < mesh->skeleton.size(); ++index)
			{
				transform_index = (geometry_index * mesh->skeleton.size()) + index;
				Joint* joint = &mesh->skeleton[index];

				glm::mat4& local_bone_pose = local_bone_transforms[transform_index];
				glm::mat4& model_pose = model_bone_transforms[transform_index];

				glm::mat4 parent_pose;
				glm::mat4& inverse_bind_pose = inverse_bind_transforms[transform_index];

				glm::mat4 local_rotation = glm::toMat4(rotations[index]);
				glm::mat4 local_transform = glm::translate(glm::mat4(1.0f), positions[index]);

				const glm::mat4 local_pose = local_transform * local_rotation;
				if (joint->parent_index > -1)
				{
					parent_pose = model_bone_transforms[joint->parent_index];
				}

				// this will be cached in local transforms
				local_bone_pose = geo->bind_poses[index] * local_pose;

				// this will be used for skinning in the vertex shader
				model_pose = parent_pose * local_bone_pose;

				// set the inverse_bind_pose
				inverse_bind_pose = geo->inverse_bind_poses[index];

				glm::mat4 local_bbox_xf = glm::toMat4(glm::angleAxis(glm::radians(15.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
				Hitbox* hitbox = (hitboxes + index);
				//glm::vec3 pos(0.0f, 0.0f, 0.0f);
				//glm::vec3 dims(0.5f, 0.5f, 0.5f);
				//pos = mathlib::transform_point(local_bone_pose, pos);
				//debugdraw::box(-dims + pos, dims + pos, gemini::Color(0.0f, 1.0f, 1.0f));
				debugdraw::axes(glm::mat4(hitbox->rotation) * model_pose, 1.0f, 0.0f);
			}

			++geometry_index;
		}
	}
#endif

	int32_t ModelInstanceData::get_animation_index(const char* name)
	{
		size_t index = 0;
#if 0
		for (const animation::SequenceId& id : animations)
		{
			animation::AnimatedInstance* instance = animation::get_instance_by_index(id);
			animation::Sequence* sequence = animation::get_sequence_by_index(instance->sequence_index);
			if (0 == core::str::case_insensitive_compare(name, sequence->name(), 0))
			{
				return index;
				break;
			}
			++index;
		}
#endif
		return -1;
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

	//uint32_t ModelInstanceData::get_total_bones(int32_t /*index*/) const
	//{
	//	assert(mesh != nullptr);
	//	return mesh->skeleton.size();
	//}

	//int32_t ModelInstanceData::find_bone_named(const char* bone)
	//{
	//	for (const Joint& joint : mesh->skeleton)
	//	{
	//		if (joint.name == bone)
	//		{
	//			return joint.index;
	//		}
	//	}

	//	return -1;
	//}

#if 0

	void ModelInstanceData::get_local_bone_pose(int32_t /*animation_index*/, int32_t bone_index, glm::vec3& position, glm::quat& rotation)
	{
		assert(bone_index != -1);

		const glm::mat4 model_matrix = local_bone_transforms[bone_index];
		rotation = glm::toQuat(model_matrix);
		position = glm::vec3(glm::column(model_matrix, 3));
	}

	void ModelInstanceData::get_model_bone_pose(int32_t /*animation_index*/, int32_t bone_index, glm::vec3& position, glm::quat& rotation)
	{
		assert(bone_index != -1);

		const glm::mat4& model_matrix = model_bone_transforms[bone_index];
		rotation = glm::toQuat(model_matrix);
		position = glm::vec3(glm::column(model_matrix, 3));
	}

#endif

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
} // namespace gemini