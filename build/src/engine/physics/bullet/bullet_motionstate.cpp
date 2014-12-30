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

//#include <sdk/physics_collisionobject.h>
//#include <sdk/physics_constraint.h>

//#include "physics/bullet/bullet_common.h"
#include "physics/bullet/bullet_motionstate.h"

namespace gemini
{
	namespace physics
	{
		namespace bullet
		{
			CustomMotionState::CustomMotionState(const btTransform& transform,
							  PhysicsMotionInterface* motion,
							  const glm::vec3& center_mass_offset) : initial_transform(transform),
			motion_interface(motion),
			mass_center_offset(center_mass_offset)
			{
			}
			
			CustomMotionState::~CustomMotionState()
			{
			}
			
			void CustomMotionState::getWorldTransform(btTransform &world_transform) const
			{
				world_transform = initial_transform;
			}
			
			void CustomMotionState::setWorldTransform(const btTransform &world_transform)
			{
				btQuaternion rot = world_transform.getRotation();
				btVector3 pos = world_transform.getOrigin();
				glm::quat orientation(rot.w(), rot.x(), rot.y(), rot.z());
				glm::vec3 position(pos.x(), pos.y(), pos.z());
				
				if (motion_interface)
				{
					motion_interface->set_transform(position, orientation, mass_center_offset);
				}
			}
		} // namespace bullet
	} // namespace physics
} // namespace gemini