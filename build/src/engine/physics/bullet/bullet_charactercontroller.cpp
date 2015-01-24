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

#include <iostream>
#include "btBulletDynamicsCommon.h"
#include "physics/physics_common.h"

#include "physics/bullet/bullet_charactercontroller.h"

#include <core/mathlib.h>
#include "camera.h"

#include "debugdraw.h"

#define SMOOTH_UNIDIRECTIONAL_MOVEMENT 0
#define CHARACTER_MOVEMENT_MULTIPLIER 1.5
//
//  Hybrid Character Controller
//

/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2008 Erwin Coumans  http://bulletphysics.com

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it freely,
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/


#if 0
#undef F
#define F( name ) printf( "Function - %s()\n", #name )
#endif

#include "LinearMath/btIDebugDraw.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h"
#include "BulletCollision/CollisionShapes/btMultiSphereShape.h"
#include "BulletCollision/BroadphaseCollision/btOverlappingPairCache.h"
#include "BulletCollision/BroadphaseCollision/btCollisionAlgorithm.h"
#include "BulletCollision/CollisionDispatch/btCollisionWorld.h"
#include "LinearMath/btDefaultMotionState.h"


// for CharacterController
#include "input.h"

#include "bullet_common.h"

namespace gemini
{
	namespace physics
	{

		const btScalar DEFAULT_MOVEMENT_DAMPING = 0.85f;
		const btScalar movementWeightFactor = 1.0f;
		const btScalar AIR_MOVEMENT_DAMPING = 0.85f;

		// static helper method
		static btVector3
		getNormalizedVector(const btVector3& v)
		{
			btVector3 n = v.normalized();
			if (n.length() < SIMD_EPSILON) {
				n.setValue(0, 0, 0);
			}
			return n;
		}


		//
		// CharacterTwo
		// 

		CharacterTwo::CharacterTwo(btPairCachingGhostObject* ghost_object, btConvexShape* shape)
		{
			ghost = ghost_object;
			position.setValue(0, 0, 0);
			target_position.setValue(0, 0, 0);
			velocity.setValue(0, 0, 0);			
			acceleration.setValue(0, 0, 0);
			gravity.setValue(0.0f, -9.8f, 0.0f);
			rotation.setValue(0.0f, 0.0f, 0.0f, 1.0f);
			
			active_shape = shape;
			
			// set initial transform from ghost
			const btTransform& tx = ghost->getWorldTransform();
			position = tx.getOrigin();
			
			
			step_height = 0.35f;
		}

		void CharacterTwo::updateAction(btCollisionWorld* world, btScalar delta_time)
		{
			btTransform xform = ghost->getWorldTransform();
			btQuaternion current_rotation = xform.getRotation();
		
			// player step
			acceleration.setValue(0, 0, 0);
			acceleration = gravity;
			
			// the movement vector is oriented to the character.
			// we rotate the movement vector by the current rotation
			// and then we can apply it to the acceleration.
			// p' = q * p * q^(-1)
			btQuaternion p(movement.x(), movement.y(), movement.z(), 0);
			btQuaternion result = current_rotation * p * current_rotation.inverse();
			btVector3 result_movement(result.x(), result.y(), result.z());
			
			// draw the result_movement vector for debugging purposes.
//			glm::vec3 target_origin(position.x(), position.y(), position.z());
//			glm::vec3 basis(result_movement.x(), result_movement.y(), result_movement.z());
//			debugdraw::basis(target_origin, basis, 2.0f, 0);

			acceleration += result_movement*30.0f;

			velocity += acceleration*delta_time;
		
			glm::vec3 initial_position = toglm(position);
		
			target_position = position;

			
			step_up(world, delta_time);
			step_forward_and_strafe(world, delta_time);
			step_down(world, delta_time);



			const float friction = 0.85f;
			velocity.setX(velocity.x() * friction);
			velocity.setZ(velocity.z() * friction);
			
			
			position = target_position;
			
			{
				// draw velocity
//				glm::vec3 target_origin(position.x(), position.y(), position.z());
//				debugdraw::line(target_origin, target_origin+glm::vec3(velocity.x(), velocity.y(), velocity.z()), Color(255, 255, 0), 0);

//				debugdraw::line(initial_position, toglm(target_position), Color(255, 0, 128), 2.0);
			}
	
			xform.setOrigin(position);
//			xform.setRotation(rotation);
			ghost->setWorldTransform(xform);
		}
		
		void CharacterTwo::debugDraw(btIDebugDraw* debug_draw)
		{
			
		}

		void CharacterTwo::warp(const btVector3& target_position)
		{
			btTransform xform;
			xform.setIdentity();
			xform.setOrigin(target_position);
			ghost->setWorldTransform(xform);
		}
		
		void CharacterTwo::set_view_direction(const btVector3 &vec_right, const btVector3 &vec_view)
		{
			right = vec_right;
			view = vec_view;
		}
		
		void CharacterTwo::step_up(btCollisionWorld *world, btScalar delta_time)
		{
			target_position += btVector3(0, step_height, 0) * delta_time;
		}
		
		
		void CharacterTwo::attempt_move(const btVector3& target_velocity, float delta_time)
		{
			btVector3 new_position = target_position + target_velocity*delta_time;
			
			// try to move
			btTransform start, end;
			start.setIdentity();
			end.setIdentity();
			
			start.setOrigin(target_position);
			end.setOrigin(new_position);
			
			ClosestNotMeConvexResultCallback callback(ghost, btVector3(0, 1.0f, 0), 0.0f);
			callback.m_collisionFilterGroup = ghost->getBroadphaseHandle()->m_collisionFilterGroup;
			callback.m_collisionFilterMask = ghost->getBroadphaseHandle()->m_collisionFilterMask;
			
			ghost->convexSweepTest(active_shape, start, end, callback, bullet::get_world()->getDispatchInfo().m_allowedCcdPenetration);
			
			if (callback.hasHit())
			{
				LOGV("frac: %g\n", callback.m_closestHitFraction);
				
				debugdraw::sphere(toglm(callback.m_hitPointWorld), Color(255, 0, 0), 0.05f, 2.0f);
				debugdraw::line(toglm(callback.m_hitPointWorld), toglm(callback.m_hitPointWorld+callback.m_hitNormalWorld), Color(0, 255, 255), 2.0f);

				target_position.setInterpolate3(target_position, new_position, callback.m_closestHitFraction);

				glm::vec3 normal = toglm(callback.m_hitNormalWorld);

				
			}
			else
			{
//				LOGV("no hit\n");
				target_position = new_position;
			}
			debugdraw::line(toglm(position), toglm(target_position), Color(255, 0, 0), 2.0f);
//			LOGV("target_position: %2.2f\n", target_position.x());
		}
		
		
		bool CharacterTwo::collide_segment(ClosestNotMeConvexResultCallback& callback, const btVector3& start_position, const btVector3& end_position)
		{
			btTransform start;
			btTransform end;
			start.setIdentity();
			end.setIdentity();
			start.setOrigin(start_position);
			end.setOrigin(end_position);

			callback.m_collisionFilterGroup = ghost->getBroadphaseHandle()->m_collisionFilterGroup;
			callback.m_collisionFilterMask = ghost->getBroadphaseHandle()->m_collisionFilterMask;
			
			ghost->convexSweepTest(active_shape, start, end, callback, bullet::get_world()->getDispatchInfo().m_allowedCcdPenetration);
			
			if (callback.hasHit())
			{
				debugdraw::sphere(toglm(callback.m_hitPointWorld), Color(255, 0, 0), 0.025f, 2.0f);
				debugdraw::line(toglm(callback.m_hitPointWorld), toglm(callback.m_hitPointWorld+callback.m_hitNormalWorld), Color(0, 255, 255), 2.0f);
			}
			else
			{
				debugdraw::line(toglm(start_position), toglm(end_position), Color(255, 255, 0), 2.0f);
			}
		
			return callback.hasHit();
		}
		
		
		
		void CharacterTwo::step_forward_and_strafe(btCollisionWorld *world, btScalar delta_time)
		{
//			attempt_move(btVector3(velocity.x(), 0, 0), delta_time);
			//attempt_move(btVector3(0.0f, 0.0f, velocity.z()), delta_time);
			btVector3 move_velocity = btVector3(velocity.x(), 0.0f, velocity.z());
			btVector3 remaining_velocity = move_velocity;
			
			float remaining_time = delta_time;
		
			for (int i = 0; i < 1 && remaining_time > 0.0f; i++)
			{
				btVector3 new_position = target_position + remaining_velocity*remaining_time;
				
				btTransform start, end;
				start.setIdentity();
				end.setIdentity();
				
				start.setOrigin(target_position);
				end.setOrigin(new_position);
				
				ClosestNotMeConvexResultCallback callback(ghost, btVector3(0, 1.0f, 0), 0.0f);
				collide_segment(callback, target_position, new_position);

				if (callback.hasHit())
				{
					float margin = 0.0f;
					float used_time = (callback.m_closestHitFraction-margin) * delta_time;
					remaining_time = remaining_time - used_time;

					// travel until the first collision
					btVector3 hitpoint = lerp(target_position, new_position, callback.m_closestHitFraction);
					
					// if the new forward normal is orthogonal OR facing away from the wall

					{
						target_position = hitpoint;
	
						// the velocity that extends into the wall
						btVector3 new_velocity = move_velocity - 1*move_velocity.dot(callback.m_hitNormalWorld)*callback.m_hitNormalWorld;
						btVector3 remaining_velocity = move_velocity - new_velocity;
						btVector3 vector_projection = (callback.m_hitNormalWorld.dot(remaining_velocity) / remaining_velocity.dot(remaining_velocity)) * remaining_velocity;

						hitpoint = remaining_time*new_velocity*.9f;
						
						// see if the new velocity would cause a collision
						ClosestNotMeConvexResultCallback cb2(ghost, btVector3(0, 1.0f, 0), 0.0f);
						collide_segment(cb2, target_position, target_position+hitpoint);

						btVector3 final_position = target_position + hitpoint;
						target_position = final_position;
						velocity = new_velocity;
					}
				}
				else
				{
//					assert(new_position.z() > 1.2f);
					target_position = new_position;
					break;
				}
			}
		}
		
		void CharacterTwo::step_down(btCollisionWorld* world, btScalar delta_time)
		{

			// test for drop
			btVector3 new_position;
			btVector3 step_drop = btVector3(0, -step_height+velocity.y(), 0);
			new_position = target_position + step_drop*delta_time;

			btTransform start, end;
			start.setIdentity();
			end.setIdentity();
			
			start.setOrigin(target_position);
			end.setOrigin(new_position);
			
			ClosestNotMeConvexResultCallback callback(ghost, btVector3(0, 1.0f, 0), btScalar(btCos(45.0f)));
			callback.m_collisionFilterGroup = ghost->getBroadphaseHandle()->m_collisionFilterGroup;
			callback.m_collisionFilterMask = ghost->getBroadphaseHandle()->m_collisionFilterMask;
			
			ghost->convexSweepTest(active_shape, start, end, callback, bullet::get_world()->getDispatchInfo().m_allowedCcdPenetration);
			
			if (callback.hasHit())
			{
				// we only dropped a fraction of the distance
				target_position.setInterpolate3(target_position, new_position, callback.m_closestHitFraction);

				// at this point, we've hit the ground
//				LOGV("we've hit the ground.\n");
			}
			else
			{
				target_position = new_position;
			}
		}

		//
		// KinematicCharacter
		//
		
		/*
		 * Returns the reflection direction of a ray going 'direction' hitting a surface with normal 'normal'
		 *
		 * from: http://www-cs-students.stanford.edu/~adityagp/final/node3.html
		 */
		btVector3 KinematicCharacter::computeReflectionDirection (const btVector3& direction, const btVector3& normal)
		{
			return direction - (btScalar(2.0) * direction.dot(normal)) * normal;
		}

		/*
		 * Returns the portion of 'direction' that is parallel to 'normal'
		 */
		btVector3 KinematicCharacter::parallelComponent (const btVector3& direction, const btVector3& normal)
		{
			btScalar magnitude = direction.dot(normal);
			return normal * magnitude;
		}

		/*
		 * Returns the portion of 'direction' that is perpindicular to 'normal'
		 */
		btVector3 KinematicCharacter::perpindicularComponent (const btVector3& direction, const btVector3& normal)
		{
			return direction - parallelComponent(direction, normal);
		}

		KinematicCharacter::KinematicCharacter (btPairCachingGhostObject* ghostObject,btConvexShape* convexShape,btScalar stepHeight, int upAxis)
		{
			m_upAxis = upAxis;
			m_addedMargin = 0.02;
			m_walkDirection.setValue(0,0,0);
			m_useGhostObjectSweepTest = true;
			m_ghostObject = ghostObject;
			m_stepHeight = stepHeight;
			m_turnAngle = btScalar(0.0);
			m_convexShape=convexShape;
			m_useWalkDirection = true;	// use walk direction by default, legacy behavior
			m_velocityTimeInterval = 0.0;
			m_verticalVelocity = 0.0;
			m_verticalOffset = 0.0;
			//m_gravity = 9.8; // 1G acceleration.
			m_gravity = 9.8;
			m_fallSpeed = m_gravity; // Terminal velocity of a sky diver in m/s.
			//m_jumpSpeed = 10.0; // ?
			m_jumpSpeed = 10;
			m_wasOnGround = false;
			m_wasJumping = false;
			setMaxSlope(btRadians(45.0));
			m_dampenMovement = true;
			bEnableFreeCam = false;
			m_velocity.setValue(0,0,0);

			m_movementDamping = DEFAULT_MOVEMENT_DAMPING;
			m_moveWeight.setZero();
			m_prevMoveWeight.setZero();
		}

		KinematicCharacter::~KinematicCharacter ()
		{
		}

		btPairCachingGhostObject* KinematicCharacter::getGhostObject()
		{
			return m_ghostObject;
		}

		void KinematicCharacter::StopWalking()
		{

			//m_useWalkDirection = false;
			m_dampenMovement = true;
		}

		bool KinematicCharacter::recoverFromPenetration ( btCollisionWorld* collisionWorld)
		{

			bool penetration = false;

			collisionWorld->getDispatcher()->dispatchAllCollisionPairs(m_ghostObject->getOverlappingPairCache(), collisionWorld->getDispatchInfo(), collisionWorld->getDispatcher());

			m_currentPosition = m_ghostObject->getWorldTransform().getOrigin();

			btScalar maxPen = btScalar(0.0);
			for (int i = 0; i < m_ghostObject->getOverlappingPairCache()->getNumOverlappingPairs(); i++)
			{
				m_manifoldArray.resize(0);

				btBroadphasePair* collisionPair = &m_ghostObject->getOverlappingPairCache()->getOverlappingPairArray()[i];

				btCollisionObject* obj0 = static_cast<btCollisionObject*>(collisionPair->m_pProxy0->m_clientObject);
				btCollisionObject* obj1 = static_cast<btCollisionObject*>(collisionPair->m_pProxy1->m_clientObject);

				if ((obj0 && !obj0->hasContactResponse()) || (obj1 && !obj1->hasContactResponse()))
					continue;

				if (collisionPair->m_algorithm)
					collisionPair->m_algorithm->getAllContactManifolds(m_manifoldArray);


				for (int j=0;j<m_manifoldArray.size();j++)
				{
					btPersistentManifold* manifold = m_manifoldArray[j];
					btScalar directionSign = manifold->getBody0() == m_ghostObject ? btScalar(-1.0) : btScalar(1.0);
					for (int p=0;p<manifold->getNumContacts();p++)
					{
						const btManifoldPoint&pt = manifold->getContactPoint(p);

						btScalar dist = pt.getDistance();

						if (dist < 0.0)
						{
							if (dist < maxPen)
							{
								maxPen = dist;
								m_touchingNormal = pt.m_normalWorldOnB * directionSign;//??

							}
							m_currentPosition += pt.m_normalWorldOnB * directionSign * dist * btScalar(0.2);
							penetration = true;
						} else {
							//printf("touching %f\n", dist);
						}
					}

					//manifold->clearManifold();
				}
			}
			btTransform newTrans = m_ghostObject->getWorldTransform();
			newTrans.setOrigin(m_currentPosition);
			m_ghostObject->setWorldTransform(newTrans);
//			printf("m_touchingNormal = %f,%f,%f\n",m_touchingNormal[0],m_touchingNormal[1],m_touchingNormal[2]);
			return penetration;
		}

		void KinematicCharacter::stepUp ( btCollisionWorld* world)
		{
			F( stepUp );

			// phase 1: up
			btTransform start, end;
			m_targetPosition = m_currentPosition + getUpAxisDirections()[m_upAxis] * (m_stepHeight + (m_verticalOffset > 0.0?m_verticalOffset:0.0));

			start.setIdentity ();
			end.setIdentity ();

			/* FIXME: Handle penetration properly */
			start.setOrigin (m_currentPosition + getUpAxisDirections()[m_upAxis] * (m_convexShape->getMargin() + m_addedMargin));
			end.setOrigin (m_targetPosition);

			ClosestNotMeConvexResultCallback callback (m_ghostObject, -getUpAxisDirections()[m_upAxis], btScalar(0.7071));
			callback.m_collisionFilterGroup = getGhostObject()->getBroadphaseHandle()->m_collisionFilterGroup;
			callback.m_collisionFilterMask = getGhostObject()->getBroadphaseHandle()->m_collisionFilterMask;

			if (m_useGhostObjectSweepTest)
			{
				m_ghostObject->convexSweepTest (m_convexShape, start, end, callback, world->getDispatchInfo().m_allowedCcdPenetration);
			}
			else
			{
				world->convexSweepTest (m_convexShape, start, end, callback);
			}

			if (callback.hasHit())
			{
				//printf( "stepUp: Hit something...\n" );
				// Only modify the position if the hit was a slope and not a wall or ceiling.
				if(callback.m_hitNormalWorld.dot(getUpAxisDirections()[m_upAxis]) > 0.0)
				{
					//printf( "Moved up a fraction...\n" );
					// we moved up only a fraction of the step height
					m_currentStepOffset = m_stepHeight * callback.m_closestHitFraction;
					m_currentPosition.setInterpolate3 (m_currentPosition, m_targetPosition, callback.m_closestHitFraction);
				}
				m_verticalVelocity = 0.0;
				m_verticalOffset = 0.0;
			} else {
				m_currentStepOffset = m_stepHeight;
				m_currentPosition = m_targetPosition;
			}
		}

		void KinematicCharacter::updateTargetPositionBasedOnCollision (const btVector3& hitNormal, btScalar tangentMag, btScalar normalMag)
		{
			F( updateTargetPositionBasedOnCollision );

			btVector3 movementDirection = m_targetPosition - m_currentPosition;
			btScalar movementLength = movementDirection.length();
			if (movementLength>SIMD_EPSILON)
			{
				movementDirection.normalize();

				btVector3 reflectDir = computeReflectionDirection (movementDirection, hitNormal);
				reflectDir.normalize();

				btVector3 parallelDir, perpindicularDir;

				parallelDir = parallelComponent (reflectDir, hitNormal);
				perpindicularDir = perpindicularComponent (reflectDir, hitNormal);

				m_targetPosition = m_currentPosition;
				if (0)//tangentMag != 0.0)
				{
					btVector3 parComponent = parallelDir * btScalar (tangentMag*movementLength);
		//			printf("parComponent=%f,%f,%f\n",parComponent[0],parComponent[1],parComponent[2]);
					m_targetPosition +=  parComponent;
				}

				if (normalMag != 0.0)
				{
					btVector3 perpComponent = perpindicularDir * btScalar (normalMag*movementLength);
		//			printf("perpComponent=%f,%f,%f\n",perpComponent[0],perpComponent[1],perpComponent[2]);
					m_targetPosition += perpComponent;
				}
			} else
			{
		//		printf("movementLength don't normalize a zero vector\n");
			}
		}

		void KinematicCharacter::setMovementWeight( float _forward, float _back, float _left, float _right )
		{
			mFrontMove = _forward * movementWeightFactor;
			mBackMove = _back * movementWeightFactor;
			mLeftMove = _left * movementWeightFactor;
			mRightMove = _right * movementWeightFactor;
	//		LOGV("weight: %g %g %g %g\n", _forward, _back, _left, _right);
		}

		void KinematicCharacter::setFacingDirections( const btVector3 & front, const btVector3 & right )
		{
			mDirFront = front;
			mDirRight = right;

			if ( !bEnableFreeCam )
			{
				mDirFront.setY(0);
				mDirRight.setY(0);
			}

			mDirFront.normalize();
			mDirRight.normalize();
		}

		void KinematicCharacter::enableDamping( bool bEnable )
		{
			m_dampenMovement = bEnable;
		}

		void KinematicCharacter::stepForwardAndStrafe ( btCollisionWorld* collisionWorld, const btVector3& walkMove, const btVector3 & airMove, btScalar dt)
		{
			F( stepForwardAndStrafe );

			btVector3 curMovement;

		#if SMOOTH_UNIDIRECTIONAL_MOVEMENT
			curMovement = m_velocity;
		#endif

			if ( m_wasOnGround )
			{
				//printf( "stepForwardAndStrafe() - OnGround: Using Walk Movement\n" );
		#if SMOOTH_UNIDIRECTIONAL_MOVEMENT
				curMovement += walkMove;
		#else
				curMovement = walkMove;
		#endif

				if (m_dampenMovement)
				{
		#if SMOOTH_UNIDIRECTIONAL_MOVEMENT
					m_velocity = (m_velocity * m_movementDamping);
		#else
					//printf( "Damping..\n" );
					m_walkDirection = (m_walkDirection * m_movementDamping);
		#endif
				}
			}
			else
			{
				if ( !bEnableFreeCam )
				{
					//printf( "stepForwardAndStrafe() - Not OnGround: Using Air Movement %g %g %g\n", BTV3(airMove) );
					curMovement = airMove + (walkMove * AIR_MOVEMENT_DAMPING);

		#if SMOOTH_UNIDIRECTIONAL_MOVEMENT
		#else
					m_walkDirection = airMove;
		#endif

					//curMovement = walkMove;
					//m_walkDirection = walkMove;
				}
				else
				{
					curMovement = walkMove;

		#if SMOOTH_UNIDIRECTIONAL_MOVEMENT
					m_velocity = (m_velocity * m_movementDamping);
		#else
					m_walkDirection = (m_walkDirection * m_movementDamping);
		#endif
				}
			}
			// printf("m_normalizedDirection=%f,%f,%f\n",
			// 	m_normalizedDirection[0],m_normalizedDirection[1],m_normalizedDirection[2]);
			// phase 2: forward and strafe
			btTransform start, end;
			//m_targetPosition = m_currentPosition + walkMove;


		#if SMOOTH_UNIDIRECTIONAL_MOVEMENT
			m_targetPosition = m_currentPosition + (m_velocity * dt * CHARACTER_MOVEMENT_MULTIPLIER);
			//printf( "Velocity: %g %g %g\n", BTV3(m_velocity) );
		#else
			m_targetPosition = m_currentPosition + (curMovement * (dt * 2)); // .2
		#endif
			start.setIdentity ();
			end.setIdentity ();

			btScalar fraction = 1.0;
			btScalar distance2;// = (m_currentPosition-m_targetPosition).length2();
		//	printf("distance2=%f\n",distance2);

			if (m_touchingContact)
			{
				if (m_normalizedDirection.dot(m_touchingNormal) > btScalar(0.0))
				{
					updateTargetPositionBasedOnCollision (m_touchingNormal);
				}
			}

			int maxIter = 10;

			while (fraction > btScalar(0.01) && maxIter-- > 0)
			{
				start.setOrigin (m_currentPosition);
				end.setOrigin (m_targetPosition);
				btVector3 sweepDirNegative(m_currentPosition - m_targetPosition);

				ClosestNotMeConvexResultCallback callback (m_ghostObject, sweepDirNegative, btScalar(0.0));
				callback.m_collisionFilterGroup = getGhostObject()->getBroadphaseHandle()->m_collisionFilterGroup;
				callback.m_collisionFilterMask = getGhostObject()->getBroadphaseHandle()->m_collisionFilterMask;


				btScalar margin = m_convexShape->getMargin();
				m_convexShape->setMargin(margin + m_addedMargin);


				if (m_useGhostObjectSweepTest)
				{
					m_ghostObject->convexSweepTest (m_convexShape, start, end, callback, collisionWorld->getDispatchInfo().m_allowedCcdPenetration);
				} else
				{
					collisionWorld->convexSweepTest (m_convexShape, start, end, callback, collisionWorld->getDispatchInfo().m_allowedCcdPenetration);
				}

				m_convexShape->setMargin(margin);


				fraction -= callback.m_closestHitFraction;

				if (callback.hasHit())
				{
					// we moved only a fraction
					//btScalar hitDistance = (callback.m_hitPointWorld - m_currentPosition).length();

		//			m_currentPosition.setInterpolate3 (m_currentPosition, m_targetPosition, callback.m_closestHitFraction);

					updateTargetPositionBasedOnCollision (callback.m_hitNormalWorld);
					btVector3 currentDir = m_targetPosition - m_currentPosition;
					distance2 = currentDir.length2();
					if (distance2 > SIMD_EPSILON)
					{
						currentDir.normalize();
						/* See Quake2: "If velocity is against original velocity, stop ead to avoid tiny oscilations in sloping corners." */
						if (currentDir.dot(m_normalizedDirection) <= btScalar(0.0))
						{
							break;
						}
					} else
					{
		//				printf("currentDir: don't normalize a zero vector\n");
						break;
					}
				} else {
					// we moved whole way
					m_currentPosition = m_targetPosition;
				}

			//	if (callback.m_closestHitFraction == 0.f)
			//		break;

			}
		}

		void KinematicCharacter::stepDown ( btCollisionWorld* collisionWorld, btScalar dt)
		{
			F( stepDown );

			btTransform start, end;

			// phase 3: down
			/*btScalar additionalDownStep = (m_wasOnGround && !onGround()) ? m_stepHeight : 0.0;
			btVector3 step_drop = getUpAxisDirections()[m_upAxis] * (m_currentStepOffset + additionalDownStep);
			btScalar downVelocity = (additionalDownStep == 0.0 && m_verticalVelocity<0.0?-m_verticalVelocity:0.0) * dt;
			btVector3 gravity_drop = getUpAxisDirections()[m_upAxis] * downVelocity;
			m_targetPosition -= (step_drop + gravity_drop);*/

			btScalar downVelocity = (m_verticalVelocity<0.0?-m_verticalVelocity:0.0) * dt;
			/*
			if(downVelocity > 0.0 && downVelocity < m_stepHeight
				&& (m_wasOnGround || !m_wasJumping))
			{
				downVelocity = m_stepHeight * .5;
			}*/

			btVector3 step_drop = getUpAxisDirections()[m_upAxis] * (m_currentStepOffset + downVelocity);
			m_targetPosition -= step_drop;

			start.setIdentity ();
			end.setIdentity ();

			start.setOrigin (m_currentPosition);
			end.setOrigin (m_targetPosition);

			ClosestNotMeConvexResultCallback callback (m_ghostObject, getUpAxisDirections()[m_upAxis], m_maxSlopeCosine);
			callback.m_collisionFilterGroup = getGhostObject()->getBroadphaseHandle()->m_collisionFilterGroup;
			callback.m_collisionFilterMask = getGhostObject()->getBroadphaseHandle()->m_collisionFilterMask;

			if (m_useGhostObjectSweepTest)
			{
				m_ghostObject->convexSweepTest (m_convexShape, start, end, callback, collisionWorld->getDispatchInfo().m_allowedCcdPenetration);
			} else
			{
				collisionWorld->convexSweepTest (m_convexShape, start, end, callback, collisionWorld->getDispatchInfo().m_allowedCcdPenetration);
			}

			if (callback.hasHit())
			{
				// we dropped a fraction of the height -> hit floor
				m_currentPosition.setInterpolate3 (m_currentPosition, m_targetPosition, callback.m_closestHitFraction);
				m_verticalVelocity = 0.0;
				m_verticalOffset = 0.0;
				m_wasJumping = false;
			} else {
				// we dropped the full height

				m_currentPosition = m_targetPosition;

			}
		}



		void KinematicCharacter::setWalkDirection
		(
		const btVector3& walkDirection
		)
		{
			//printf( "SetWalkDirection!\n" );

			m_useWalkDirection = true;
			m_walkDirection = getNormalizedVector(walkDirection);

			//printf( "Walk Direction: %g %g %g\n", BTV3( m_walkDirection ) );
			m_normalizedDirection = getNormalizedVector(m_walkDirection);
			m_dampenMovement = false;
		}



		void KinematicCharacter::setVelocityForTimeInterval
		(
		const btVector3& velocity,
		btScalar timeInterval
		)
		{
		//	printf("setVelocity!\n");
		//	printf("  interval: %f\n", timeInterval);
		//	printf("  velocity: (%f, %f, %f)\n",
		//		 velocity.x(), velocity.y(), velocity.z());

			m_useWalkDirection = false;
			m_walkDirection = velocity;
			m_normalizedDirection = getNormalizedVector(m_walkDirection);
			m_velocityTimeInterval = timeInterval;
		}

		void KinematicCharacter::SetSpawnLocation( const btVector3 & spawn )
		{
			mSpawnPoint = spawn;
		}

		void KinematicCharacter::get_movement_command(physics::MovementCommand& command)
		{
			
		}

		void KinematicCharacter::reset (btCollisionWorld* collisionWorld)
		{
			F( reset );
			
	//		// clear pair cache on ghostobject
	//		btHashedOverlappingPairCache *cache = m_ghostObject->getOverlappingPairCache();
	//		while (cache->getOverlappingPairArray().size() > 0)
	//		{
	//			cache->removeOverlappingPair(cache->getOverlappingPairArray()[0].m_pProxy0, cache->getOverlappingPairArray()[0].m_pProxy1, collisionWorld->getDispatcher());
	//		}
		}

		void KinematicCharacter::setUpInterpolate(bool value)
		{
			
		}

		void KinematicCharacter::clear_state()
		{
			btTransform tr;
			tr.setIdentity();
			tr.setOrigin( mSpawnPoint );
			
			m_ghostObject->setWorldTransform( tr );
			
			// set gravity back to zero
			//mVerticalAccel = 0;
			//m_verticalVelocity = 0;
			//m_velocity.setValue( 0, 0, 0 );
			
			m_accMoveDir.setValue(0,1,0);
			
			mFrontMove = 0;
			mBackMove = 0;
			mLeftMove = 0;
			mRightMove = 0;
		}

		void KinematicCharacter::warp (const btVector3& origin)
		{
			F( warp );

			btTransform xform;
			xform.setIdentity();
			xform.setOrigin (origin);
			m_ghostObject->setWorldTransform (xform);
		}

		void KinematicCharacter::preStep (  btCollisionWorld* collisionWorld)
		{
			F( preStep );

			int numPenetrationLoops = 0;
			m_touchingContact = false;
			while (recoverFromPenetration (collisionWorld))
			{
				numPenetrationLoops++;
				m_touchingContact = true;
				if (numPenetrationLoops > 4)
				{
					//printf("character could not recover from penetration = %d\n", numPenetrationLoops);
					break;
				}
			}

			m_currentPosition = m_ghostObject->getWorldTransform().getOrigin();
			m_targetPosition = m_currentPosition;
		//	printf("m_targetPosition=%f,%f,%f\n",m_targetPosition[0],m_targetPosition[1],m_targetPosition[2]);


		}

		void KinematicCharacter::updateAction( btCollisionWorld* collisionWorld,btScalar deltaTime)
		{
			//printf( "------------------ Update Action Begin------------\n" );
			preStep ( collisionWorld);
			//printf( "CurrentPosition: %g %g %g\n", BTV3( m_currentPosition ) );
			playerStep (collisionWorld, deltaTime);
			//printf( "------------------ Update Action End------------\n" );
		}
		

		
		#include <stdio.h>

		void KinematicCharacter::playerStep (  btCollisionWorld* collisionWorld, btScalar dt)
		{
			F( playerStep );

		//	printf("playerStep(): ");
		//	printf("  dt = %f", dt);

			// quick check...
			if (!m_useWalkDirection && m_velocityTimeInterval <= 0.0) {
		//		printf("\n");
				return;		// no motion
			}

			m_wasOnGround = onGround();

			// caused by a jump or movement off ledge
			if ( m_prevOnGround && !m_wasOnGround )
			{
				//printf( "Moved from Ground to Air\n" );
				m_airMove = m_walkDirection;
			}

			// Update fall velocity.
			m_verticalVelocity -= m_gravity * dt;
			if(m_verticalVelocity > 0.0 && m_verticalVelocity > m_jumpSpeed)
			{
				m_verticalVelocity = m_jumpSpeed;
			}
			if(m_verticalVelocity < 0.0 && btFabs(m_verticalVelocity) > btFabs(m_fallSpeed))
			{
				m_verticalVelocity = -btFabs(m_fallSpeed);
			}
			m_verticalOffset = m_verticalVelocity * dt;

			//printf( "Velocity: %g m_wasOnGround: %i\n", m_verticalVelocity, m_wasOnGround );


			btTransform xform;
			xform = m_ghostObject->getWorldTransform ();

		//	printf("walkDirection(%f,%f,%f)\n",m_walkDirection[0],m_walkDirection[1],m_walkDirection[2]);
		//	printf("walkSpeed=%f\n",walkSpeed);



			if ( m_wasOnGround )
			{
				m_airMove.setValue( 0, 0, 0 );
			}

			const float moveSpeed = 100;
			float fbw = (mFrontMove - mBackMove);
			if ( fbw > 1.0f )
				fbw = 1.0f;
			else if ( fbw < -1.0 )
				fbw = -1.0f;

			btVector3 fbMove;
			fbMove.setZero();
			fbMove = (mDirFront * fbw) * dt * moveSpeed;

			float lrw = (mRightMove - mLeftMove);
			if (lrw > 1.0f)
				lrw = 1.0f;
			else if (lrw < -1.0f)
				lrw = -1.0f;
				
			btVector3 lrMove;
			lrMove.setZero();
			lrMove = (mDirRight * lrw) * dt * moveSpeed;
			
	//		printf( "mFrontMove: %g | mBackMove: %g (%g)| mLeftMove: %g | mRightMove: %g (%g)\n", mFrontMove, mBackMove, fbw, mLeftMove, mRightMove, lrw );

			btScalar length = 0;
			const float MAX_MAG = 3.0f;

		#if SMOOTH_UNIDIRECTIONAL_MOVEMENT
			m_moveWeight = (fbMove+lrMove);

			length = m_moveWeight.length();

			if ( length > MAX_MAG )
			{
				m_moveWeight = (m_moveWeight*(MAX_MAG/length));
			}

			m_velocity = (m_moveWeight * MAX_MAG) * 0.5f + (m_prevMoveWeight * MAX_MAG * 0.5f);
			m_prevMoveWeight = m_moveWeight;

			length = m_velocity.length();
			if ( length > MAX_MAG )
			{
				m_velocity.setX( (m_velocity.x() / length) * MAX_MAG );
				m_velocity.setY( (m_velocity.y() / length) * MAX_MAG );
				m_velocity.setZ( (m_velocity.z() / length) * MAX_MAG );
			}
			else if ( length < FLT_EPSILON )
			{
				m_velocity.setValue(0,0,0);
			}

		#else
			m_walkDirection += (fbMove + lrMove);

			length = m_walkDirection.length();

			//printf( "Length: %g\n", length );
			if ( length > MAX_MAG )
			{
				m_walkDirection.setX( (m_walkDirection.x() / length) * MAX_MAG );
				m_walkDirection.setY( (m_walkDirection.y() / length) * MAX_MAG );
				m_walkDirection.setZ( (m_walkDirection.z() / length) * MAX_MAG );
			}


		#endif

			//printf( "walk Direction: %g %g %g\n", BTV3( m_walkDirection ) );

			if( !bEnableFreeCam )
				stepUp (collisionWorld);

			if (m_useWalkDirection) {
				//printf( "Walk Direction: %g %g %g\n", BTV3( m_walkDirection ) );
				stepForwardAndStrafe (collisionWorld, m_walkDirection, m_airMove, dt);
			} else {
				//printf("  time: %f", m_velocityTimeInterval);
				// still have some time left for moving!
				btScalar dtMoving =
					(dt < m_velocityTimeInterval) ? dt : m_velocityTimeInterval;
				m_velocityTimeInterval -= dt;

				// how far will we move while we are moving?
				btVector3 move = m_walkDirection * dtMoving;

				// printf("  dtMoving: %f", dtMoving);

				// okay, step
				stepForwardAndStrafe(collisionWorld, move, m_airMove, dt);
			}



			if ( !bEnableFreeCam )
				stepDown (collisionWorld, dt);

			//printf( "CurrentPosition: %g %g %g\n", BTV3( m_currentPosition ) );
			// printf("\n");

			xform.setOrigin (m_currentPosition);
			m_ghostObject->setWorldTransform (xform);

			m_prevOnGround = m_wasOnGround;

			// clear the pair cache at the end of the step
	//		btHashedOverlappingPairCache* cache = m_ghostObject->getOverlappingPairCache();
	//		while (cache->getOverlappingPairArray().size() > 0)
	//		{
	//			cache->removeOverlappingPair(cache->getOverlappingPairArray()[0].m_pProxy0,
	//				cache->getOverlappingPairArray()[0].m_pProxy1,
	//				collisionWorld->getDispatcher());
	//		}
		}

		void KinematicCharacter::setFallSpeed (btScalar fallSpeed)
		{
			m_fallSpeed = fallSpeed;
		}

		void KinematicCharacter::setJumpSpeed (btScalar jumpSpeed)
		{
			m_jumpSpeed = jumpSpeed;
		}

		void KinematicCharacter::setMaxJumpHeight (btScalar maxJumpHeight)
		{
			m_maxJumpHeight = maxJumpHeight;
		}

		bool KinematicCharacter::canJump () const
		{
			return onGround();
		}

		void KinematicCharacter::jump ()
		{
			F( jump );

			if (!canJump())
				return;

			m_verticalVelocity = m_jumpSpeed;
			m_wasJumping = true;
		#if 0
			currently no jumping.
			btTransform xform;
			m_rigidBody->getMotionState()->getWorldTransform (xform);
			btVector3 up = xform.getBasis()[1];
			up.normalize ();
			btScalar magnitude = (btScalar(1.0)/m_rigidBody->getInvMass()) * btScalar(8.0);
			m_rigidBody->applyCentralImpulse (up * magnitude);
		#endif
		}

		void KinematicCharacter::setGravity(btScalar gravity)
		{
			m_gravity = gravity;
		}

		btScalar KinematicCharacter::getGravity() const
		{
			return m_gravity;
		}

		void KinematicCharacter::setMaxSlope(btScalar slopeRadians)
		{
			m_maxSlopeRadians = slopeRadians;
			m_maxSlopeCosine = btCos(slopeRadians);
		}

		btScalar KinematicCharacter::getMaxSlope() const
		{
			return m_maxSlopeRadians;
		}

		bool KinematicCharacter::onGround () const
		{
			//return ( m_verticalVelocity < 0.001 && m_verticalOffset < 0.001 );

			return m_verticalVelocity == 0.0 && m_verticalOffset == 0.0;
		}


		btVector3* KinematicCharacter::getUpAxisDirections()
		{
			static btVector3 sUpAxisDirection[3] = { btVector3(1.0f, 0.0f, 0.0f), btVector3(0.0f, 1.0f, 0.0f), btVector3(0.0f, 0.0f, 1.0f) };

			return sUpAxisDirection;
		}

		void KinematicCharacter::debugDraw(btIDebugDraw* debugDrawer)
		{
		}

		void KinematicCharacter::ToggleFreeCam()
		{
			bEnableFreeCam = !bEnableFreeCam;
		}

		void KinematicCharacter::setDamping( btScalar dampingFactor )
		{
			m_movementDamping = dampingFactor;
		}

		const btVector3 KinematicCharacter::getVelocity()
		{
			return m_velocity*CHARACTER_MOVEMENT_MULTIPLIER;	
		}

		btVector3 KinematicCharacter::getOrigin()
		{
			return this->getGhostObject()->getWorldTransform().getOrigin();
		}		
	} // namespace physics
} // namespace gemini