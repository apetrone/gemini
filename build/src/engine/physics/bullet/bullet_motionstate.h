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
				btTransform initial_transform;
				//PhysicsMotionInterface* motion_interface;
//				glm::vec3 mass_center_offset;
				btGhostObject* ghost;
				btRigidBody* body;

			public:
				CustomMotionState(const btTransform& transform
//									PhysicsMotionInterface* motion,
//									const glm::vec3& center_mass_offset
//									btRigidBody* rigid_body,
//									btGhostObject* ghost_object
								  );
						  
				void set_body_and_ghost(btRigidBody* body, btGhostObject* ghost);

				virtual ~CustomMotionState() {}

				virtual void getWorldTransform(btTransform &world_transform) const;
				virtual void setWorldTransform(const btTransform &world_transform);
			};
		} // namespace bullet
	} // namespace physics
} // namespace gemini