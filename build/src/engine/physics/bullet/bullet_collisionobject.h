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
			class CustomMotionState;
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

				CustomMotionState* motion_state;

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

				virtual void get_linear_velocity(glm::vec3& velocity);
				virtual void set_linear_velocity(const glm::vec3& velocity);

				virtual void collision_began(ICollisionObject* other);
				virtual void collision_ended(ICollisionObject* other);

				virtual void apply_impulse(const glm::vec3& force, const glm::vec3& local_position);
				virtual void apply_central_impulse(const glm::vec3& force);

				virtual ICollisionShape* get_shape() const;

				void remove_constraints();

				void set_collision_shape(btCollisionShape* collision_shape);
				btCollisionShape* get_collision_shape() const;

				void set_collision_object(btCollisionObject* collision_object);
				btCollisionObject* get_collision_object() const;

				void set_collision_ghost(btGhostObject* collision_ghost);
				btGhostObject* get_collision_ghost() const;
	//			virtual void set_mass_center_offset(const glm::vec3 &mass_center_offset);

				void set_motion_state(CustomMotionState* motionstate);
				btMotionState* get_motion_state() const;

				virtual void set_offset(const glm::vec3& offset) override;
				virtual const glm::vec3& get_offset() const override;
			};

		} // namespace bullet
	} // namespace physics
} // namespace gemini
