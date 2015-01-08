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

//#include "physics_collisionobject.h"
//#include <core/mathlib.h>


#include <btBulletDynamicsCommon.h>

class btDiscreteDynamicsWorld;

namespace gemini
{
	namespace physics
	{
		class Constraint;
	
		namespace bullet
		{
			class BulletConstraint;
		
			btDiscreteDynamicsWorld* get_world();
			void set_world(btDiscreteDynamicsWorld* world);
			
			
			void add_constraint(BulletConstraint* constraint);
			void remove_constraint(BulletConstraint* constraint);
			
			void startup();
			void shutdown();
			void step(float seconds);
			void debug_draw();
			
			btTransform position_and_orientation_to_transform(const glm::vec3& position, const glm::quat& orientation);
			void position_and_orientation_from_transform(glm::vec3& out_position, glm::quat& out_orientation, const btTransform& world_transform);
		}
	} // namespace physics
} // namespace gemini