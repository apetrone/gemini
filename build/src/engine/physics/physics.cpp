// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone
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
#include <vector>

#include "physics.h"
#include <btBulletDynamicsCommon.h>
#include "physics_common.h"

#include <core/typedefs.h>

#include <renderer/debug_draw.h>
#include <renderer/color.h>

#include "assets/asset_mesh.h"

#include "physics/bullet/bullet_collisionobject.h"
#include "physics/bullet/bullet_constraint.h"
#include "physics/bullet/bullet_debugdraw.h"


#include "physics_interface.h"

namespace gemini
{
	namespace physics
	{
		void * bullet2_custom_alloc( size_t size )
		{
			return MEMORY_ALLOC(size, core::memory::global_allocator());
		}

		void bullet2_custom_free( void * memblock )
		{
			MEMORY_DEALLOC(memblock, core::memory::global_allocator());
		}

		void startup()
		{
			IPhysicsInterface* physics_interface = MEMORY_NEW(PhysicsInterface, core::memory::global_allocator());
			set_instance(physics_interface);

			bullet::startup();
		}

		void shutdown()
		{
			//
			// Cleanup
			//

			bullet::shutdown();

			IPhysicsInterface* physics_interface = instance();
			MEMORY_DELETE(physics_interface, core::memory::global_allocator());
		} // shutdown


		void step(float framedelta_seconds, float fixed_step_seconds)
		{
			bullet::step(framedelta_seconds, fixed_step_seconds);
		} // step

		void debug_draw()
		{
			bullet::debug_draw();
		} // debug_draw
	} // namespace physics
} // namespace gemini
