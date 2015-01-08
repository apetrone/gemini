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

class btCollisionShape;
class btCollisionObject;
class btGhostObject;

namespace gemini
{
	namespace physics
	{
		namespace bullet
		{
			class BulletCollisionShape : public ICollisionShape
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
			
			class BulletCollisionObject : public ICollisionObject
			{
			protected:
				btCollisionObject* object;
				
				// This will have keep a pointer to its active collision shape
				// however, it will NOT manage it -- as it can be hot swapped.
				btCollisionShape* shape;
				
				// ghost object to sense collisions with other objects
				btGhostObject* ghost;
				
				btMotionState* motion_state;
				
				void* user_data;
							
				CollisionCallback callback;
			public:
				
				BulletCollisionObject();
				virtual ~BulletCollisionObject();
				
				virtual void set_user_data(void* userdata);
				virtual void* get_user_data() const;
				
				virtual void set_collision_callback(CollisionCallback collision_callback);
				
				virtual void set_world_transform(const glm::vec3& position, const glm::quat& orientation);
				virtual void get_world_transform(glm::vec3& out_position, glm::quat& out_orientation);
				
				virtual void collision_began(ICollisionObject* other);
				virtual void collision_ended(ICollisionObject* other);
				
				void remove_constraints();
							
				void set_collision_shape(btCollisionShape* collision_shape);
				btCollisionShape* get_collision_shape() const;
				
				void set_collision_object(btCollisionObject* collision_object);
				btCollisionObject* get_collision_object() const;
				
				void set_collision_ghost(btGhostObject* collision_ghost);
				btGhostObject* get_collision_ghost() const;
	//			virtual void set_mass_center_offset(const glm::vec3 &mass_center_offset);
	
				void set_motion_state(btMotionState* motionstate);
				btMotionState* get_motion_state() const;
			};
		
		} // namespace bullet
	} // namespace physics
} // namespace gemini