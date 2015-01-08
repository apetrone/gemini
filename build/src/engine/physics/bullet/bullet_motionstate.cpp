// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM,OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
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
			}
			
			void CustomMotionState::getWorldTransform(btTransform &world_transform) const
			{
				world_transform = compose_transform();
			}
			
			void CustomMotionState::setWorldTransform(const btTransform &world_transform)
			{
				position_and_orientation_from_transform(position, orientation, world_transform);
				
				// sync up the ghost object when this rigid body moves
				if (ghost)
				{
					ghost->setWorldTransform(world_transform);
				}
			}
		} // namespace bullet
	} // namespace physics
} // namespace gemini