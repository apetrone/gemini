// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone
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

#include <core/mathlib.h>
#include <runtime/asset_handle.h>

namespace gemini
{
	struct Hitbox;

	class IModelInstanceData
	{
	public:
		virtual ~IModelInstanceData() {};

		virtual AssetHandle asset_index() const = 0;
		//virtual glm::mat4& get_local_transform() = 0;
		//virtual void set_local_transform(const glm::mat4& transform) = 0;

		virtual const Hitbox* get_hitboxes() const = 0;

		virtual void set_animation_enabled(int32_t index, bool enabled) = 0;

		// add an animation to this model
		//virtual int32_t add_animation(const char* name) = 0;

		virtual float get_animation_duration(int32_t index) const = 0;
		//virtual uint32_t get_total_bones(int32_t index) const = 0;

		//virtual int32_t find_bone_named(const char* bone) = 0;

		// returns the bone's current pose in local coordinates
		//virtual void get_local_bone_pose(int32_t animation_index, int32_t bone_index, glm::vec3& position, glm::quat& rotation) = 0;

		// returns the bone's current pose in model coordinates
		//virtual void get_model_bone_pose(int32_t animation_index, int32_t bone_index, glm::vec3& position, glm::quat& rotation) = 0;

		virtual const glm::vec3& get_mins() const = 0;
		virtual const glm::vec3& get_maxs() const = 0;
		virtual const glm::vec3& get_center_offset() const = 0;
	};

	class IModelInterface
	{
	public:
		virtual ~IModelInterface() {};


//		virtual int32_t get_model_index(const char* model_path) = 0;

		virtual int32_t create_instance_data(uint16_t entity_index, const char* model_path) = 0;
		virtual void destroy_instance_data(int32_t index) = 0;

		virtual IModelInstanceData* get_instance_data(int32_t index) = 0;
	};
} // namespace gemini
