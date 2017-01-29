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
#pragma once

#include <sdk/model_api.h>
#include <core/mathlib.h>

#include <renderer/debug_draw.h>

#include <runtime/animation.h>
#include <runtime/assets.h>
#include <runtime/keyframechannel.h>


namespace gemini
{
	struct Mesh;

	// Each entity that has a model associated with it
	// will have a model instance data allocated.
	class ModelInstanceData : public IModelInstanceData
	{
		AssetHandle mesh_handle;
		Mesh* mesh;
		glm::mat4 transform;

		// parent-local bone transforms
		glm::mat4* local_bone_transforms;

		// model bone transforms
		glm::mat4* model_bone_transforms;

		glm::mat4* inverse_bind_transforms;

		Channel<glm::vec3> scale_channel;
		Channel<glm::quat> rotation_channel;
		Channel<glm::vec3> translation_channel;

		glm::vec3 scale;
		glm::quat rotation;
		glm::vec3 translation;

		std::vector<animation::SequenceId> animations;

		gemini::Allocator& allocator;
	public:

		ModelInstanceData(gemini::Allocator& allocator);

		void set_mesh_index(AssetHandle in_mesh_handle);
		void create_bones();
		void destroy_bones();
		virtual AssetHandle asset_index() const;
		virtual glm::mat4& get_local_transform();
		virtual void set_local_transform(const glm::mat4& _transform);
		virtual glm::mat4* get_model_bone_transforms(uint32_t geometry_index) const;
		virtual const Hitbox* get_hitboxes() const;
		virtual glm::mat4* get_inverse_bind_transforms(uint32_t geometry_index) const;
		virtual uint32_t get_total_transforms() const;
		virtual void set_animation_enabled(int32_t index, bool enabled);
		virtual void get_animation_pose(int32_t index, glm::vec3* positions, glm::quat* rotations);
		virtual void set_pose(glm::vec3* positions, glm::quat* rotations);
		virtual int32_t get_animation_index(const char* name);
		virtual int32_t add_animation(const char* name);
		virtual int32_t get_total_animations() const;
		virtual void reset_channels(int32_t index);
		virtual float get_animation_duration(int32_t index) const;
		virtual uint32_t get_total_bones(int32_t /*index*/) const;
		virtual int32_t find_bone_named(const char* bone);
		virtual void get_local_bone_pose(int32_t /*animation_index*/, int32_t bone_index, glm::vec3& position, glm::quat& rotation);
		virtual void get_model_bone_pose(int32_t /*animation_index*/, int32_t bone_index, glm::vec3& position, glm::quat& rotation);
		virtual const glm::vec3& get_mins() const;
		virtual const glm::vec3& get_maxs() const;
		virtual const glm::vec3& get_center_offset() const;
	}; // ModelInstanceData

} // namespace gemini