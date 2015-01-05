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
#include <vector>

#include <platform/typedefs.h>
#include <slim/xlog.h>
#include "physics.h"
#include <btBulletDynamicsCommon.h>
#include "physics_common.h"
#include <renderer/color.h>
#include <renderer/debugdraw.h>

#include "bullet/bullet_charactercontroller.h"

#include "assets/asset_mesh.h"



const float PHYSICS_PLAYER_HALF_WIDTH = 0.25f; // .25 == 1.6 ft wide
const float PHYSICS_PLAYER_HALF_HEIGHT = 0.91f; // .91 == 6 ft tall

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
			return ALLOC( size );
		}
		
		void bullet2_custom_free( void * memblock )
		{
			DEALLOC( memblock );
		}
		
		void startup()
		{
			IPhysicsInterface* physics_interface = CREATE(PhysicsInterface);
			api::set_instance(physics_interface);
		
			bullet::startup();
		}
		
		void shutdown()
		{
			//
			// Cleanup
			//

			bullet::shutdown();
			
			IPhysicsInterface* physics_interface = api::instance();
			DESTROY(IPhysicsInterface, physics_interface);
		} // shutdown
		
		
		void step( float seconds )
		{
			bullet::step(seconds);
		} // step
		
		void debug_draw()
		{
			bullet::debug_draw();
		} // debug_draw
		
		RaycastInfo raycast(ICollisionObject* ignored_object, const glm::vec3& start, const glm::vec3& direction, float max_distance)
		{
//			glm::vec3 destination = (start + direction * max_distance);
//			btVector3 ray_start(start.x, start.y, start.z);
//			btVector3 ray_end(destination.x, destination.y, destination.z);
//			
//			BulletCollisionObject* bullet_object = static_cast<BulletCollisionObject*>(ignored_object);
//			btCollisionObject* obj = bullet_object->get_collision_object();
//			
//			ClosestNotMeRayResultCallback callback(obj);
//			dynamics_world->rayTest(ray_start, ray_end, callback);
			
			RaycastInfo info;
			
			
//			if (callback.hasHit())
//			{
//				info.hit = start + (destination * callback.m_closestHitFraction);
//				info.object = static_cast<ICollisionObject*>(callback.m_collisionObject->getUserPointer());
//				
//				LOGV("fraction: %2.2f\n", callback.m_closestHitFraction);
//				
//				if (callback.m_collisionObject->getCollisionFlags() & btCollisionObject::CF_STATIC_OBJECT)
//				{
//					LOGV("hit: %g (static object)\n", callback.m_closestHitFraction);
//				}
//				else
//				{
//					LOGV("hit: %g (dynamic object)\n", callback.m_closestHitFraction);
//				}
//			}
			
			// should probably return a structure with data regarding the hit?
			return info;
		} // raycast

	} // namespace physics
} // namespace gemini