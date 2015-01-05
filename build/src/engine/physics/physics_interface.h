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
		
			virtual physics::ICollisionObject* create_physics_model(
				int32_t model_index,
				ObjectProperties& properties
			);
			
			virtual physics::ICollisionObject* create_character_object(ICollisionShape* shape);
			virtual physics::ICollisionObject* create_trigger_object(ICollisionShape* shape);
			
			virtual physics::ICollisionShape* create_capsule(
				float radius_meters,
				float height_meters
			 );
			
			virtual void destroy_object(ICollisionObject* object);
			
			virtual IPlayerController* create_player_controller(ICollisionObject* object);
			virtual void destroy_player_controller(IPlayerController* controller);
			
			virtual void step_simulation(float delta_seconds);
		};

	} // namespace physics
} // namespace gemini