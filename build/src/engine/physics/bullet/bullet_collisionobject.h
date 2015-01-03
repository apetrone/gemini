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

#include "physics/bullet/bullet_common.h"


namespace gemini
{
	namespace physics
	{
	
		class BulletCollisionShape : public CollisionShape
		{
		protected:
			btCollisionShape* shape;
			
		public:
		
			virtual ~BulletCollisionShape()
			{
				// TODO: does this need to remove any bodies, first?
				// If we crash, then maybe.
				
				if (shape)
				{
					delete shape;
					shape = 0;
				}
			}
		
			void set_shape(btCollisionShape* _shape)
			{
				shape = _shape;
			}
			
			btCollisionShape* get_shape() const { return shape; }
		};
		
		class BulletCollisionObject : public CollisionObject
		{
		protected:
			btCollisionObject* object;
			
			
		public:
			
			BulletCollisionObject() : object(0) {}
			
			virtual ~BulletCollisionObject()
			{
				if (object)
				{
					remove_constraints();
					
					bullet::get_world()->removeCollisionObject(object);
					delete object;
					object = 0;
				}
			}
			
			void remove_constraints()
			{
				// need to remove constraints here...
				for (int i = 0; i < MAX_CONSTRAINTS_PER_OBJECT; ++i)
				{
					if (constraints[i])
					{
						constraints[i]->remove();
					}
				}
			}
			
			void set_collision_object(btCollisionObject* collision_object)
			{
				object = collision_object;
			}
			
			btCollisionObject* get_collision_object() const { return object; }
			
			virtual void set_mass_center_offset(const glm::vec3 &mass_center_offset)
			{
				this->mass_center_offset = mass_center_offset;
			}
			
			virtual void set_world_transform(const glm::vec3& position, const glm::quat& orientation)
			{
				assert(object != nullptr);
				btTransform world_transform = object->getWorldTransform();
				
				glm::vec3 target_position = position;// - mass_center_offset;

				btTransform transform;
				transform.setIdentity();
				transform.setRotation(btQuaternion(orientation.x, orientation.y, orientation.z, orientation.w));
				transform.setOrigin(btVector3(target_position.x, target_position.y, target_position.z));
				object->setWorldTransform(transform);
			}
			
			
			virtual void get_world_transform(glm::vec3& out_position, glm::quat& out_orientation)
			{
				assert(object != 0);
				const btTransform& world_transform = object->getWorldTransform();
				const btVector3& origin = world_transform.getOrigin();
				const btQuaternion& rot = world_transform.getRotation();
				
				out_position = glm::vec3(origin.x(), origin.y(), origin.z());// + mass_center_offset;
				out_orientation = glm::quat(rot.w(), rot.x(), rot.y(), rot.z());
			}
			
			virtual void collision_began(CollisionObject* other)
			{
				if (callback)
				{
					callback(Collision_Began, this, other);
				}
			}
			
			virtual void collision_ended(CollisionObject* other)
			{
				if (callback)
				{
					callback(Collision_Ended, this, other);
				}
			}
		};
	} // namespace physics
} // namespace gemini