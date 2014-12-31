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

#include <stdint.h>
#include <core/mathlib.h>

#include <sdk/interface.h>

namespace gemini
{
	namespace physics
	{
		class CollisionShape;
		class CollisionObject;
		
		class PhysicsMotionInterface
		{
		public:
			virtual ~PhysicsMotionInterface() {};
			
			// called when the physics body's motion state needs to be retrieved
			virtual void get_transform(glm::vec3& position, const glm::quat& orientation) = 0;
			
			// called when the physics body's motion state has been set
			virtual void set_transform(const glm::vec3& position, const glm::quat& orientation, const glm::vec3& mass_center_offset) = 0;
		};
		
		
		class PlayerController
		{
		public:
			virtual ~PlayerController() {};
			
		}; // PlayerController
		
		class PhysicsInterface
		{
		public:
			virtual ~PhysicsInterface() {};
			
			virtual physics::CollisionObject* create_physics_model(
				int32_t model_index,
				float mass_kg = 0.0f,
				physics::PhysicsMotionInterface* motion_interface = 0,
				const glm::vec3& mass_center_offset = glm::vec3(0, 0, 0)
			) = 0;
			
			// Shapes
			// Shapes are handled internally by the physics system and do not
			// need to be explicitly deleted.
			virtual physics::CollisionShape* create_capsule(
				float radius_meters,
				float height_meters
			) = 0;
			
			virtual void destroy_object(CollisionObject* object) = 0;
			
			
			virtual PlayerController* create_player_controller(CollisionObject* object) = 0;
			virtual void destroy_player_controller(PlayerController* object) = 0;
		};

		
		typedef Interface<PhysicsInterface> api;
	} // namespace physics
} // namespace gemini