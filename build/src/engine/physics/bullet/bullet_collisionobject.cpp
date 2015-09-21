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

#include "bullet_collisionobject.h"

#include "bullet_common.h"

#include "BulletCollision/CollisionDispatch/btGhostObject.h"

namespace gemini
{
	namespace physics
	{
		namespace bullet
		{
			BulletCollisionObject::BulletCollisionObject() :
				object(0),
				shape(0),
				ghost(0),
				motion_state(0),
				user_data(0),
				callback(0)
			{
			}
				
			BulletCollisionObject::~BulletCollisionObject()
			{
				if (object)
				{
					remove_constraints();
					
					bullet::get_world()->removeCollisionObject(object);
					delete object;
					object = 0;
				}
				
				if (ghost)
				{
					bullet::get_world()->removeCollisionObject(ghost);
					delete ghost;
					ghost = 0;
				}
				
				shape = 0;
				
				
				
				// maybe the motion states are already deleted by bullet?
//				if (motion_state)
//				{
//					delete motion_state;
//					motion_state = 0;
//				}
			}
				
				
			void BulletCollisionObject::set_user_data(void* userdata)
			{
				user_data = userdata;
			}
			
			void* BulletCollisionObject::get_user_data() const
			{
				return user_data;
			}
			
			void BulletCollisionObject::set_collision_callback(CollisionCallback collision_callback)
			{
				callback = collision_callback;
			}
			
			void BulletCollisionObject::set_world_transform(const glm::vec3& position, const glm::quat& orientation)
			{
				assert(object != nullptr);
				btTransform transform = bullet::position_and_orientation_to_transform(position, orientation);
				object->setWorldTransform(transform);

				if (ghost)
				{
					ghost->setWorldTransform(transform);
				}
			}
			
			void BulletCollisionObject::get_world_transform(glm::vec3& out_position, glm::quat& out_orientation)
			{
				assert(object != 0);
				
				btTransform world_transform;
				
				if (motion_state)
				{
					// grab interpolated current transform
					motion_state->getWorldTransform(world_transform);
				}
				else // fall back and grab immediate state of the body (not interpolated)
				{
					// static and kinematic bodies have to use this; they don't have a motionstate.
					world_transform = object->getWorldTransform();
				}
				
				position_and_orientation_from_transform(out_position, out_orientation, world_transform);
			}
			
			
			void BulletCollisionObject::get_linear_velocity(glm::vec3& velocity)
			{
				btRigidBody* rigid_body = btRigidBody::upcast(object);
				if (rigid_body)
				{
					const btVector3& in_velocity = rigid_body->getLinearVelocity();
					velocity.x = in_velocity.x();
					velocity.y = in_velocity.y();
					velocity.z = in_velocity.z();
				}
			}
			
			void BulletCollisionObject::set_linear_velocity(const glm::vec3& velocity)
			{
				btRigidBody* rigid_body = btRigidBody::upcast(object);
				if (rigid_body)
				{
					rigid_body->setLinearVelocity(btVector3(velocity.x, velocity.y, velocity.z));
				}
			}
			
			
			void BulletCollisionObject::collision_began(ICollisionObject* other)
			{
				if (callback)
				{
					callback(Collision_Began, this, other);
				}
			}
			
			void BulletCollisionObject::collision_ended(ICollisionObject* other)
			{
				if (callback)
				{
					callback(Collision_Ended, this, other);
				}
			}
			
			// forces are given for the full second, but need to be re-applied each step.
			// impulses are an instant change to velocity.
			
			void BulletCollisionObject::apply_impulse(const glm::vec3& impulse, const glm::vec3& local_position)
			{
				btRigidBody* rigid_body = btRigidBody::upcast(object);
				if (rigid_body)
				{
					rigid_body->activate();
					rigid_body->applyImpulse(btVector3(impulse.x, impulse.y, impulse.z), btVector3(local_position.x, local_position.y, local_position.z));
				}
			}
			
			void BulletCollisionObject::apply_central_impulse(const glm::vec3 &impulse)
			{
				btRigidBody* rigid_body = btRigidBody::upcast(object);
				if (rigid_body)
				{
					rigid_body->activate();
					rigid_body->applyCentralImpulse(btVector3(impulse.x, impulse.y, impulse.z));
				}
			}
			
			
			void BulletCollisionObject::remove_constraints()
			{
	//				// need to remove constraints here...
	//				for (int i = 0; i < MAX_CONSTRAINTS_PER_OBJECT; ++i)
	//				{
	//					if (constraints[i])
	//					{
	//						constraints[i]->remove();
	//					}
	//				}
			}
			
			
			void BulletCollisionObject::set_collision_shape(btCollisionShape* collision_shape)
			{
				shape = collision_shape;
				
				btCollisionObject* collision_object = static_cast<btCollisionObject*>(object);
				assert(collision_object != 0);
				
				// you cannot assign a collision shape until the body has been created
				collision_object->setCollisionShape(collision_shape);
			}
			
			btCollisionShape* BulletCollisionObject::get_collision_shape() const { return shape; }
			
			
			
			void BulletCollisionObject::set_collision_object(btCollisionObject* collision_object)
			{
				object = collision_object;
			}
			
			btCollisionObject* BulletCollisionObject::get_collision_object() const { return object; }
			
			
			
			void BulletCollisionObject::set_collision_ghost(btGhostObject* collision_ghost)
			{
				ghost = collision_ghost;
			}
			
			btGhostObject* BulletCollisionObject::get_collision_ghost() const { return ghost; }
			
	//		void BulletCollisionObject::set_mass_center_offset(const glm::vec3 &mass_center_offset)
	//		{
	//			this->mass_center_offset = mass_center_offset;
	//		}
			
			void BulletCollisionObject::set_motion_state(btMotionState *motionstate)
			{
				motion_state = motionstate;
			}
			
			btMotionState* BulletCollisionObject::get_motion_state() const  { return motion_state; }
		} // namespace bullet
	} // namespace physics
} // namespace gemini
