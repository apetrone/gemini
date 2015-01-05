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
			void* user_data;
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
			
			void set_user_data(void* userdata) { user_data = userdata; }
			void* get_user_data() const { return user_data; }
//			
//			void set_collision_callback(CollisionCallback _callback)
//			{
//				callback = _callback;
//			}
						
//			virtual void set_mass_center_offset(const glm::vec3& mass_center_offset) = 0;
			
			virtual void get_world_transform(glm::vec3& position, glm::quat& orientation) = 0;
			virtual void set_world_transform(const glm::vec3& position, const glm::quat& orientation) = 0;
			
			// invoked when this object when another object collides with it
			virtual void collision_began(ICollisionObject* other) = 0;
			
			// invoked when this object no longer collides with other
			virtual void collision_ended(ICollisionObject* other) = 0;
			
			virtual void apply_force(const glm::vec3& force, const glm::vec3& local_position) {};
			virtual void apply_central_force(const glm::vec3& force) {};
			
			virtual void set_mass(float mass) {};
			
			virtual void set_parent(ICollisionObject* first, ICollisionObject* second) {};
		};
	} // namespace physics
} // namespace gemini