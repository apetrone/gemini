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