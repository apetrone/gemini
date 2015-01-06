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
		class ICollisionShape;
		class ICollisionObject;
	

		
		struct MovementCommand;
		
		class PhysicsMotionInterface
		{
		public:
			virtual ~PhysicsMotionInterface() {};
			
			// called when the physics body's motion state needs to be retrieved
			virtual void get_transform(glm::vec3& position, const glm::quat& orientation) = 0;
			
			// called when the physics body's motion state has been set
			virtual void set_transform(const glm::vec3& position, const glm::quat& orientation, const glm::vec3& mass_center_offset) = 0;
		};
		
		
		class IPlayerController
		{
		public:
			virtual ~IPlayerController() {};
						
			virtual void set_controlled_object(ICollisionObject* collision_object) = 0;
			virtual ICollisionObject* get_controlled_object() const = 0;
			
			virtual void simulate(float delta_seconds) = 0;
			virtual void apply_movement_command(const physics::MovementCommand& command) = 0;
			
			// set view angles in degrees
			virtual void set_view_angles(const glm::vec2& view_angles) = 0;
		}; // PlayerController
		
		struct SurfaceProperties
		{
			
		}; // SurfaceProperties
		
		
		struct ObjectProperties
		{
			float mass_kg;
			float restitution;
			float friction;
			
			ObjectProperties() : mass_kg(0.0f), restitution(0.25f), friction(0.25f) {}
		}; // ObjectProperties
	
		
		
		
		struct RaycastInfo
		{
			glm::vec3 hit;
			ICollisionObject* object;
			
			RaycastInfo() : object(0)
			{
			}
		};
		
		
		
		class IPhysicsInterface
		{
		public:
			virtual ~IPhysicsInterface() {};
			
			
//			virtual physics::ICollisionObject* create_static_object(
//				ICollisionShape* shape,
//				const glm::vec3& mass_center_offset = glm::vec3(0, 0, 0)
//			) = 0;
			
			virtual physics::ICollisionObject* create_physics_object(ICollisionShape* shape, ObjectProperties& properties) = 0;
			virtual physics::ICollisionObject* create_physics_model(int32_t model_index, ObjectProperties& properties) = 0;
			virtual physics::ICollisionObject* create_character_object(ICollisionShape* shape) = 0;
			virtual physics::ICollisionObject* create_trigger_object(ICollisionShape* shape) = 0;
			
			// Shapes
			// Shapes are handled internally by the physics system and do not
			// need to be explicitly deleted.
			virtual physics::ICollisionShape* create_capsule(float radius_meters, float height_meters) = 0;
			virtual physics::ICollisionShape* create_box(const glm::vec3& dimensions) = 0;
			
			virtual void destroy_object(ICollisionObject* object) = 0;
			
			
			virtual IPlayerController* create_player_controller(ICollisionObject* object) = 0;
			virtual void destroy_player_controller(IPlayerController* object) = 0;
			
			virtual void step_simulation(float delta_seconds) = 0;
			
//			RaycastInfo raycast(ICollisionObject* ignored_object, const glm::vec3& start, const glm::vec3& direction, float max_distance);
		};

		
		typedef Interface<IPhysicsInterface> api;
	} // namespace physics
} // namespace gemini