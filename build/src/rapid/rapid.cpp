// -------------------------------------------------------------
// Copyright (C) 2016- Adam Petrone
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
#include <runtime/geometry.h>

#include <rapid/rapid.h>
//#include <core/logging.h>

namespace gemini
{
	void rapid_compute_pose(glm::mat4& world_pose, glm::mat4* world_poses, glm::quat* local_rotations, glm::mat4* joint_offsets, size_t index)
	{
		glm::quat parent_quat;
		glm::mat4 parent_pose;
		if (index > 0)
		{
			parent_quat = local_rotations[index - 1];
			parent_pose = world_poses[(index - 1)];
		}

		glm::mat4 local_pose = glm::toMat4(glm::inverse(parent_quat) * local_rotations[index]);
		world_pose = parent_pose * joint_offsets[index] * local_pose;
	}
} // namespace gemini


extern "C"
{
	void populate_interface(gemini::RapidInterface& interface)
	{
		interface.compute_pose = gemini::rapid_compute_pose;
	}
}



#if 0
glm::mat4 local_pose = joint_offsets[index] * glm::toMat4(glm::inverse(local_rotations[index]);
glm::mat4 parent_pose;
glm::mat4 inverse_parent_rotation;
if (index > 0)
{
	parent_pose = world_poses[(index - 1)];

	glm::mat4 inv_offset = glm::inverse(joint_offsets[index - 1]);
	inverse_parent_rotation = inv_offset * glm::inverse(parent_pose * joint_offsets[index - 1]);
}

glm::mat4& world_pose = world_poses[index];

// convert to
//local_pose = to_bind_pose[index] * local_pose;
//glm::mat4 model_pose = parent_pose * local_pose;
//world_pose = inverse_bind_pose[index] * model_pose;

//world_pose = parent_pose * local_pose * inverse_parent_rotation;
#endif