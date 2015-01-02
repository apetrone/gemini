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

class btCollisionShape;

namespace gemini
{
	namespace physics
	{	
		namespace bullet
		{
			// The static body will manage its collision shapes
			class BulletStaticBody : public BulletCollisionObject
			{
				btAlignedObjectArray<btCollisionShape*> shapes;
				
			public:
				BulletStaticBody();
				virtual ~BulletStaticBody();
				
				void add_shape(btCollisionShape* shape);
			};
			
			class BulletRigidBody : public BulletCollisionObject, public RigidBody
			{
				// This will have keep a pointer to its active collision shape
				// however, it will NOT manage it -- as it can be hot swapped.
				btCollisionShape* shape;
				glm::vec3 mass_center_offset;
				
			public:
				BulletRigidBody();
				virtual ~BulletRigidBody();
				
				void set_collision_shape(btCollisionShape* collision_shape);
				
				btCollisionShape* get_collision_shape() const { return shape; }
				btRigidBody* get_bullet_body() const;
				
				virtual void apply_force(const glm::vec3& force, const glm::vec3& local_position);
				virtual void apply_central_force(const glm::vec3& force);
				virtual void set_mass(float mass);
				virtual void set_parent(CollisionObject* first, CollisionObject* second);
				virtual void set_mass_center_offset(const glm::vec3& mass_center_offset);
				
				virtual void set_world_position(const glm::vec3& position);				
				virtual glm::vec3 get_world_position() const;
			};
		} // namespace bullet
	} // namespace physics
} // namespace gemini