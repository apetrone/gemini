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

namespace physics
{
	// forward declarations
	class CollisionObject;
	
	// enums, flags
	enum CollisionObjectType
	{
		CollisionType_Invalid = 0,
		CollisionType_Trigger = 2,
		CollisionType_Character = 3,
	};
	
	enum CollisionEventType
	{
		Collision_Began,
		Collision_Ended
	};
	
	typedef void (*CollisionCallback)(CollisionEventType, CollisionObject*, CollisionObject*);
	
	class CollisionObject
	{
	protected:
		CollisionObjectType collision_type;
		CollisionCallback callback;
		void* user_data;
	
	public:
		CollisionObject(CollisionObjectType type = CollisionType_Invalid) :
			collision_type(type),
			callback(0),
			user_data(0)
		{
		}
		
		virtual ~CollisionObject() {}
		
		bool is_type(CollisionObjectType _type) const { return collision_type == _type; }
		
		
		void set_user_data(void* userdata) { user_data = userdata; }
		void* get_user_data() const { return user_data; }
		
		void set_collision_callback(CollisionCallback _callback)
		{
			callback = _callback;
		}
		
		virtual void set_world_position(const glm::vec3& position) = 0;
		virtual glm::vec3 get_world_position() const = 0;
		
		// invoked when this object when another object collides with it
		virtual void collision_began(CollisionObject* other) = 0;
		
		// invoked when this object no longer collides with other
		virtual void collision_ended(CollisionObject* other) = 0;
	};
}; // namespace physics