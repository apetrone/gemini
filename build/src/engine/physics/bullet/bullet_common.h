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

//#include "physics_collisionobject.h"
//#include <core/mathlib.h>


#include <btBulletDynamicsCommon.h>

#include <core/mathlib.h>


class btDiscreteDynamicsWorld;

namespace gemini
{
	namespace physics
	{
		class Constraint;



		inline const glm::vec3 toglm(const btVector3& input)
		{
			return glm::vec3(input.x(), input.y(), input.z());
		}

		inline const btVector3 fromglm(const glm::vec3& input)
		{
			return btVector3(input.x, input.y, input.z);
		}


		namespace bullet
		{

			class BulletConstraint;

			btDiscreteDynamicsWorld* get_world();
			void set_world(btDiscreteDynamicsWorld* world);


			void add_constraint(BulletConstraint* constraint);
			void remove_constraint(BulletConstraint* constraint);

			void startup(gemini::Allocator& allocator);
			void shutdown();
			void step(float fixed_step_seconds);
			void debug_draw();

			btTransform position_and_orientation_to_transform(const glm::vec3& position, const glm::quat& orientation);
			void position_and_orientation_from_transform(glm::vec3& out_position, glm::quat& out_orientation, const btTransform& world_transform);
		}
	} // namespace physics
} // namespace gemini
