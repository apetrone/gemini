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

#include <sdk/physics_rigidbody.h>

#include "bullet_collisionobject.h"

namespace gemini
{
	namespace physics
	{	
		namespace bullet
		{			
			class BulletRigidBody : public BulletCollisionObject, public RigidBody
			{				
			public:
				BulletRigidBody();
				virtual ~BulletRigidBody();

				btRigidBody* get_bullet_body() const;
				
				virtual void apply_force(const glm::vec3& force, const glm::vec3& local_position);
				virtual void apply_central_force(const glm::vec3& force);
				virtual void set_mass(float mass);
				virtual void set_parent(ICollisionObject* first, ICollisionObject* second);
			};
		} // namespace bullet
	} // namespace physics
} // namespace gemini