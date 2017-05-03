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

#include <stdint.h>
#include <core/mathlib.h>
#include <core/interface.h>

#include <core/typedefs.h>

namespace gemini
{
	namespace physics
	{
		class ICollisionShape;
		class ICollisionObject;

		struct SurfaceProperties
		{

		}; // SurfaceProperties


		struct ObjectProperties
		{
			float mass_kg;
			float restitution;
			float friction;

			ObjectProperties() : mass_kg(0.0f), restitution(0.25f), friction(0.75f) {}
		}; // ObjectProperties

		struct RaycastInfo
		{
			glm::vec3 hit;
			glm::vec3 hit_normal;
			ICollisionObject* object;
			float closest_hit_fraction;

			RaycastInfo() : object(0)
			{
			}
		};

		enum CollisionGroup
		{
			DefaultFilter	= 1,
			StaticFilter	= 2,
			KinematicFilter = 4,
			DebrisFilter	= 8,
			SensorTrigger	= 16,
			CharacterFilter = 32,
			AllFilter		= -1
		};

		struct SweepTestResult
		{
			glm::vec3 hit_normal_world;
			glm::vec3 hit_point_world;
			float closest_hit_fraction;
			uint32_t hit_count;
			ICollisionObject* hit_object;


			uint32_t hit_items() { return hit_count; }

			SweepTestResult() :
				closest_hit_fraction(0.0f),
				hit_count(0),
				hit_object(nullptr)
			{
			}
		};

		class IPhysicsInterface
		{
		public:
			virtual ~IPhysicsInterface() {};

			virtual physics::ICollisionObject* create_physics_object(ICollisionShape* shape, const glm::vec3& position, const glm::quat& orientation, ObjectProperties& properties) = 0;
			virtual physics::ICollisionObject* create_physics_model(int32_t model_index, ObjectProperties& properties) = 0;
			virtual physics::ICollisionObject* create_character_object(ICollisionShape* shape) = 0;
			virtual physics::ICollisionObject* create_trigger_object(ICollisionShape* shape, const glm::vec3& position, const glm::quat& orientation) = 0;
			virtual physics::ICollisionObject* create_kinematic_object(ICollisionShape* shape, const glm::vec3& position, const glm::quat& orientation, uint16_t collision_mask = SensorTrigger) = 0;

			// Shapes
			// Shapes are handled internally by the physics system and do not
			// need to be explicitly deleted.
			virtual physics::ICollisionShape* create_capsule(float radius_meters, float height_meters) = 0;
			virtual physics::ICollisionShape* create_box(const glm::vec3& dimensions) = 0;
			virtual physics::ICollisionShape* create_cylinder(float radius_meters, float height_meters) = 0;
			virtual physics::ICollisionShape* create_sphere(float radius_meters) = 0;
			virtual physics::ICollisionShape* create_convex_shape(const glm::vec3* vertices, size_t total_vertices) = 0;

			virtual void destroy_object(ICollisionObject* object) = 0;

			virtual void step_simulation(float step_interval_seconds) = 0;

			virtual RaycastInfo raycast(
				const glm::vec3& start,
				const glm::vec3& direction,
				float max_distance,
				ICollisionObject* ignored_object0 = nullptr,
				ICollisionObject* ignored_object1 = nullptr
			) = 0;

			virtual SweepTestResult sweep(
				ICollisionObject* source_object,
				ICollisionShape* shape,
				const glm::vec3& start,
				const glm::vec3& end,
				float min_angle_cosine = 0.0f,
				const glm::vec3& angle = glm::vec3(0.0f, 1.0f, 0.0f)
			) = 0;

			virtual bool update_shape_geometry(ICollisionShape* shape, const glm::vec3* vertices, size_t total_vertices) = 0;
		};


		GEMINI_DECLARE_INTERFACE(IPhysicsInterface);
	} // namespace physics
} // namespace gemini
