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
			ClosestNotMeRayResultCallback (const btVector3& start,
										   const btVector3& direction,
										   bool ignore_ghost_objects = false,
										   bool ignore_static_objects = false,
										   btCollisionObject* ignored_object0 = nullptr,
										   btCollisionObject* ignored_object1 = nullptr) :
				btCollisionWorld::ClosestRayResultCallback(start, direction)
				, ignore_ghosts(ignore_ghost_objects)
				, ignore_static(ignore_static_objects)
				, ignored0(ignored_object0)
				, ignored1(ignored_object1)
			{
			}

			virtual bool needsCollision(btBroadphaseProxy* /*proxy*/) const
			{
				return true;
			}

			virtual btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace)
			{
//				LOGV("testing result\n");

				if (rayResult.m_collisionObject == ignored0 || rayResult.m_collisionObject == ignored1)
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
			btCollisionObject* ignored0;
			btCollisionObject* ignored1;
		};
//
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
//					LOGV("ignoring source ghost object\n");
					return btScalar(1.0);
				}

//				if (!convexResult.m_hitCollisionObject->hasContactResponse())
//				{
//					LOGV("ignoring object with no response\n");
//					return btScalar(1.0);
//				}

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

//				btScalar dotUp = m_up.dot(hitNormalWorld);
//				if (fabs(dotUp) < m_minSlopeDot) {
////					LOGV("ignoring hit with invalid slope\n");
//					return btScalar(1.0);
//				}

				return ClosestConvexResultCallback::addSingleResult(convexResult, normalInWorldSpace);
			}
		protected:
			btCollisionObject* m_me;
			const btVector3 m_up;
			btScalar m_minSlopeDot;
		};

	} // namespace physics
} // namespace gemini
