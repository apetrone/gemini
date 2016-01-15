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


#include <core/mathlib.h>

#include <vector>

namespace gemini
{
	namespace physics
	{
		// forward declarations
		class ICollisionObject;
		class IConstraint;

		// enums, flags
//		enum CollisionObjectType
//		{
//			CollisionType_Invalid = 0,
//			CollisionType_Trigger = 2,
//			CollisionType_Character = 3,
//			CollisionType_Dynamic = 4
//		};

		enum CollisionEventType
		{
			Collision_Began,
			Collision_Ended
		};

		typedef void (*CollisionCallback)(CollisionEventType, ICollisionObject*, ICollisionObject*);

		static const uint8_t MAX_CONSTRAINTS_PER_OBJECT = 1;


		class ICollisionShape
		{
		public:
			virtual ~ICollisionShape() {};
		};

		class ICollisionObject
		{
		protected:
//			CollisionObjectType collision_type;
//			CollisionCallback callback;
//			void* user_data;
//			Constraint* constraints[MAX_CONSTRAINTS_PER_OBJECT];
//			glm::vec3 mass_center_offset;
//			std::vector<ICollisionObject*> overlapping_shapes;

		public:
//			ICollisionObject(CollisionObjectType type = CollisionType_Invalid) :
//				collision_type(type),
//				callback(0),
//				user_data(0)
//			{
//				memset(constraints, 0, sizeof(Constraint*)*MAX_CONSTRAINTS_PER_OBJECT);
//			}

			virtual ~ICollisionObject() {}

//			bool is_type(CollisionObjectType _type) const { return collision_type == _type; }

//			void add_constraint(Constraint* constraint)
//			{
//				constraints[0] = constraint;
//			}

			virtual void set_user_data(void* userdata) = 0;
			virtual void* get_user_data() const = 0;


			virtual void set_collision_callback(CollisionCallback collision_callback) = 0;
//
//			void set_collision_callback(CollisionCallback _callback)
//			{
//				callback = _callback;
//			}

//			virtual void set_mass_center_offset(const glm::vec3& mass_center_offset) = 0;

			virtual void get_world_transform(glm::vec3& position, glm::quat& orientation) = 0;
			virtual void set_world_transform(const glm::vec3& position, const glm::quat& orientation) = 0;

			virtual void get_linear_velocity(glm::vec3& velocity) = 0;
			virtual void set_linear_velocity(const glm::vec3& velocity) = 0;

			// invoked when this object when another object collides with it
			virtual void collision_began(ICollisionObject* other) = 0;

			// invoked when this object no longer collides with other
			virtual void collision_ended(ICollisionObject* other) = 0;

			virtual void apply_impulse(const glm::vec3& force, const glm::vec3& local_position) {};
			virtual void apply_central_impulse(const glm::vec3& force) {};
//

			virtual ICollisionShape* get_shape() const = 0;
//			virtual void set_mass(float mass) {};
//
//			virtual void set_parent(ICollisionObject* first, ICollisionObject* second) {};
		};
	} // namespace physics
} // namespace gemini
