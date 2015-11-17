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

namespace gemini
{
	const unsigned int MAX_BONES = 32;

	class IModelInstanceData
	{
	public:
		virtual ~IModelInstanceData() {};

		virtual unsigned int asset_index() const = 0;
		virtual glm::mat4& get_local_transform() = 0;
		virtual void set_local_transform(const glm::mat4& transform) = 0;
//		virtual void get_geometry_data(unsigned int index, GeometryInstanceData& geometry_data) const = 0;

		virtual glm::mat4* get_bone_transforms(uint32_t geometry_index) const = 0;
		virtual glm::mat4* get_debug_bone_transforms() = 0;
		virtual uint32_t get_total_transforms() const = 0;

		virtual void set_animation_enabled(int32_t index, bool enabled) = 0;

		// get an animations pose
		virtual void get_animation_pose(int32_t index, glm::vec3* positions, glm::quat* rotations) = 0;

		// set the pose for this model instance
		virtual void set_pose(glm::vec3* positions, glm::quat* rotations) = 0;

		// returns the index of an animation by name
		// -1 if the animation could not be found.
		virtual int32_t get_animation_index(const char* name) = 0;

		// add an animation to this model
		virtual int32_t add_animation(const char* name) = 0;


		virtual int32_t get_total_animations() const = 0;


		virtual void reset_channels(int32_t index) = 0;
		virtual float get_animation_duration(int32_t index) const = 0;
		virtual uint32_t get_total_bones(int32_t index) const = 0;
	};

	class IModelInterface
	{
	public:
		virtual ~IModelInterface() {};


//		virtual int32_t get_model_index(const char* model_path) = 0;

		virtual int32_t create_instance_data(const char* model_path) = 0;
		virtual void destroy_instance_data(int32_t index) = 0;

		virtual IModelInstanceData* get_instance_data(int32_t index) = 0;
	};
} // namespace gemini
