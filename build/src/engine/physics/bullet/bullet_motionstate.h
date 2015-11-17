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

#include <sdk/physics_collisionobject.h>
#include <sdk/physics_constraint.h>
#include <sdk/physics_api.h>

#include "physics/bullet/bullet_common.h"

class btGhostObject;

namespace gemini
{
	namespace physics
	{
		namespace bullet
		{
			class CustomMotionState : public btMotionState
			{
				glm::vec3 position;
				glm::quat orientation;
				btRigidBody* body;
				btGhostObject* ghost;

			public:
				CustomMotionState(const glm::vec3& origin, const glm::quat& basis);

				void set_body_and_ghost(btRigidBody* body, btGhostObject* ghost);
				btTransform compose_transform() const;

				virtual ~CustomMotionState() {}

				virtual void getWorldTransform(btTransform &world_transform) const;

				// world_transform will be the interpolated value
				virtual void setWorldTransform(const btTransform &world_transform);
			};
		} // namespace bullet
	} // namespace physics
} // namespace gemini
