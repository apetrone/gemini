// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone

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

#include <sdk/physics_api.h>

#include "physics_constraint.h"
#include "physics_collisionobject.h"
#include "physics_rigidbody.h"

namespace assets
{
	class Mesh;
}

namespace gemini
{

	namespace physics
	{	


	#define BTVECTOR3_TO_VEC3( v ) glm::vec3( v.x(), v.y(), v.z() )

		class KinematicCharacter;


		struct RaycastInfo
		{
			glm::vec3 hit;
			CollisionObject* object;
			
			RaycastInfo() : object(0)
			{
			}
		};

		void startup();
		void shutdown();
		void step(float seconds);
		void debug_draw();
		
		
		//CollisionObject*

		
		
		RaycastInfo raycast(CollisionObject* ignored_object, const glm::vec3& start, const glm::vec3& direction, float max_distance);
		
		KinematicCharacter* create_character_controller(const glm::vec3& spawn_location, bool add_action_to_world);
		KinematicCharacter* get_character_controller(int index);
		CollisionObject* create_character_proxy(KinematicCharacter* controller);

		CollisionObject* create_physics_for_mesh(assets::Mesh* mesh, float mass_kg = 0.0f, PhysicsMotionInterface* motion = nullptr, const glm::vec3& mass_center_offset = glm::vec3(0, 0, 0));
		CollisionObject* create_trigger(const glm::vec3& size);
	}; // namespace physics
} // namespace gemini