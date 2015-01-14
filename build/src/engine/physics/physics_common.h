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

#include <core/logging.h>

// This _should_ only be included in files that have ALREADY
// included Bullet. It's not meant to be included outright.
namespace gemini
{
	namespace physics
	{
		enum ObjectType
		{
			STATIC_OBJECT = 1,
			GHOST_OBJECT = 512
		};
	
		///@todo Interact with dynamic objects,
		///Ride kinematicly animated platforms properly
		///More realistic (or maybe just a config option) falling
		/// -> Should integrate falling velocity manually and use that in stepDown()
		///Support jumping
		///Support ducking
		class ClosestNotMeRayResultCallback : public btCollisionWorld::ClosestRayResultCallback
		{
			bool ignore_ghosts;
			bool ignore_static;
			
		public:
			ClosestNotMeRayResultCallback (btCollisionObject* me, bool ignore_ghost_objects = false, bool ignore_static_objects = false) :
				btCollisionWorld::ClosestRayResultCallback(btVector3(0.0, 0.0, 0.0), btVector3(0.0, 0.0, 0.0)),
				ignore_ghosts(ignore_ghost_objects),
				ignore_static(ignore_static_objects)
				
			{
				m_me = me;
			}
			
			virtual bool needsCollision(btBroadphaseProxy* proxy) const
			{
				return true;
			}
			
			virtual btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace)
			{
//				LOGV("testing result\n");
				
				if (rayResult.m_collisionObject == m_me)
				{
//					LOGV("ignoring me\n");
					return btScalar(1.0);
				}

				bool has_static = rayResult.m_collisionObject->getCollisionFlags() & STATIC_OBJECT;
				bool has_ghost = rayResult.m_collisionObject->getCollisionFlags() & GHOST_OBJECT;
				
				if (ignore_ghosts && !rayResult.m_collisionObject->hasContactResponse() && has_ghost)
				{
//					LOGV("ignoring ghost\n");
					return btScalar(1.0);
				}
				

				if (ignore_static && has_static && !has_ghost)
				{
					return btScalar(1.0f);
				}
				
				return ClosestRayResultCallback::addSingleResult (rayResult, normalInWorldSpace);
			}
		protected:
			btCollisionObject* m_me;
		};
		
		class ClosestNotMeConvexResultCallback : public btCollisionWorld::ClosestConvexResultCallback
		{
		public:
			ClosestNotMeConvexResultCallback (btCollisionObject* me, const btVector3& up, btScalar minSlopeDot)
			: btCollisionWorld::ClosestConvexResultCallback(btVector3(0.0, 0.0, 0.0), btVector3(0.0, 0.0, 0.0))
			, m_me(me)
			, m_up(up)
			, m_minSlopeDot(minSlopeDot)
			{
			}
			
			virtual btScalar addSingleResult(btCollisionWorld::LocalConvexResult& convexResult,bool normalInWorldSpace)
			{
				if (convexResult.m_hitCollisionObject == m_me)
				{
					return btScalar(1.0);
				}
				
				if (!convexResult.m_hitCollisionObject->hasContactResponse())
				{
					return btScalar(1.0);
				}
				
				btVector3 hitNormalWorld;
				if (normalInWorldSpace)
				{
					hitNormalWorld = convexResult.m_hitNormalLocal;
				}
				else
				{
					///need to transform normal into worldspace
					hitNormalWorld = m_hitCollisionObject->getWorldTransform().getBasis()*convexResult.m_hitNormalLocal;
				}
				
				btScalar dotUp = m_up.dot(hitNormalWorld);
				if (dotUp < m_minSlopeDot) {
					return btScalar(1.0);
				}
				
				return ClosestConvexResultCallback::addSingleResult (convexResult, normalInWorldSpace);
			}
		protected:
			btCollisionObject* m_me;
			const btVector3 m_up;
			btScalar m_minSlopeDot;
		};

	} // namespace physics
} // namespace gemini