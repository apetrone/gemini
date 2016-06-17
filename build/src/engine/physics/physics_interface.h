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

#include <vector>

#include <stdint.h>
#include <core/mathlib.h>

#include <sdk/physics_api.h>



namespace gemini
{
	namespace physics
	{
		class PhysicsInterface : public IPhysicsInterface
		{
		private:
			std::vector<ICollisionShape*> collision_shapes;

		public:
			PhysicsInterface() {}
			virtual ~PhysicsInterface();

			virtual physics::ICollisionObject* create_physics_object(ICollisionShape* shape, const glm::vec3& position, const glm::quat& orientation, ObjectProperties& properties);
			virtual physics::ICollisionObject* create_physics_model(int32_t model_index, ObjectProperties& properties);
			virtual physics::ICollisionObject* create_character_object(ICollisionShape* shape);
			virtual physics::ICollisionObject* create_trigger_object(ICollisionShape* shape, const glm::vec3& position, const glm::quat& orientation);
			virtual physics::ICollisionObject* create_kinematic_object(ICollisionShape* shape, const glm::vec3& position, const glm::quat& orientation);

			virtual physics::ICollisionShape* create_capsule(float radius_meters, float height_meters);
			virtual physics::ICollisionShape* create_box(const glm::vec3& dimensions);
			virtual physics::ICollisionShape* create_cylinder(float radius_meters, float height_meters);
			virtual physics::ICollisionShape* create_sphere(float radius_meters);
			virtual physics::ICollisionShape* create_convex_shape(const glm::vec3* vertices, size_t total_vertices);

			virtual void destroy_object(ICollisionObject* object);

			virtual void step_simulation(float step_interval_seconds);

			virtual RaycastInfo raycast(ICollisionObject* ignored_object, const glm::vec3& start, const glm::vec3& direction, float max_distance);
			virtual SweepTestResult sweep(ICollisionObject* source_object, ICollisionShape* shape, const glm::vec3& start, const glm::vec3& end, float angle_threshold);
		};

	} // namespace physics
} // namespace gemini
