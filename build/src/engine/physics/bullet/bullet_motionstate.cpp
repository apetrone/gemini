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

#include "physics/bullet/bullet_motionstate.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h"

namespace gemini
{
	namespace physics
	{
		namespace bullet
		{
			CustomMotionState::CustomMotionState(const glm::vec3& origin, const glm::quat& basis) :
				position(origin),
				orientation(basis),
				body(0),
				ghost(0)
			{
				center_mass_offset.setIdentity();
			}

			void CustomMotionState::set_body_and_ghost(btRigidBody* body, btGhostObject* ghost)
			{
				this->body = body;
				this->ghost = ghost;

				// set the ghost's transform when we setup this motion state
				// so the first physics update doesn't leave the ghost at the origin.
				if (ghost)
				{
					ghost->setWorldTransform(compose_transform());
				}
			}

			btTransform CustomMotionState::compose_transform() const
			{
				return bullet::position_and_orientation_to_transform(position, orientation);
			} // compose_transform

			void CustomMotionState::getWorldTransform(btTransform &world_transform) const
			{
				world_transform = center_mass_offset.inverse() * compose_transform();
			} // getWorldTransform

			void CustomMotionState::setWorldTransform(const btTransform &world_transform)
			{
				btTransform trans_matrix = center_mass_offset * world_transform;
				position_and_orientation_from_transform(position, orientation, trans_matrix);

				// sync up the ghost object when this rigid body moves
				if (ghost)
				{
					ghost->setWorldTransform(world_transform);
				}
			} // setWorldTransform

			void CustomMotionState::set_center_mass_offset(const btVector3& mass_offset)
			{
				center_mass_offset.setIdentity();
				center_mass_offset.setOrigin(mass_offset);
			} // set_center_mass_offset

			const btVector3& CustomMotionState::get_center_mass_offset() const
			{
				return center_mass_offset.getOrigin();
			} // get_center_mass_offset

		} // namespace bullet
	} // namespace physics
} // namespace gemini
