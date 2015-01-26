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
//		const btScalar AIR_MOVEMENT_DAMPING = 0.85f;
		const btScalar MOVEMENT_MULTIPLIER = 30.0f;

		//
		// KinematicCharacter
		// 

		KinematicCharacter::KinematicCharacter(btPairCachingGhostObject* ghost_object, btConvexShape* shape)
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

		void KinematicCharacter::updateAction(btCollisionWorld* world, btScalar delta_time)
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
		
		void KinematicCharacter::debugDraw(btIDebugDraw* debug_draw)
		{
			
		}

		void KinematicCharacter::warp(const btVector3& target_position)
		{
			btTransform xform;
			xform.setIdentity();
			xform.setOrigin(target_position);
			ghost->setWorldTransform(xform);
		}
		
		void KinematicCharacter::set_view_direction(const btVector3 &vec_right, const btVector3 &vec_view)
		{
			right = vec_right;
			view = vec_view;
		}
		
		void KinematicCharacter::step_up(btCollisionWorld *world, btScalar delta_time)
		{
			target_position += btVector3(0, step_height, 0) * delta_time;
		}
		
				
		bool KinematicCharacter::collide_segment(ClosestNotMeConvexResultCallback& callback, const btVector3& start_position, const btVector3& end_position)
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
				btVector3 normal = callback.m_hitNormalWorld.normalize();
//				debugdraw::sphere(toglm(callback.m_hitPointWorld), Color(255, 0, 0), 0.005f, 2.0f);
//				debugdraw::line(toglm(callback.m_hitPointWorld), toglm(callback.m_hitPointWorld+normal*0.1f), Color(0, 255, 255), 2.0f);
			}
		
			return callback.hasHit();
		}
		
		
		
		void KinematicCharacter::step_forward_and_strafe(btCollisionWorld *world, btScalar delta_time)
		{
			btVector3 move_velocity = btVector3(velocity.x(), 0.0f, velocity.z());
			btVector3 remaining_velocity = move_velocity;
			
			float remaining_time = delta_time;
		
			for (int i = 0; i < 3 && remaining_time > 0.0f; ++i)
			{
				// calculate the new position based on the remaining velocity and time
				btVector3 new_position = target_position + remaining_velocity*remaining_time;
				
				btTransform start, end;
				start.setIdentity();
				end.setIdentity();
				
				start.setOrigin(target_position);
				end.setOrigin(new_position);
				
				ClosestNotMeConvexResultCallback callback(ghost, btVector3(0, 1.0f, 0), 0.0f);
				collide_segment(callback, target_position, new_position);

				// draw remaining velocity
//				debugdraw::line(toglm(target_position), toglm(target_position+(remaining_velocity*remaining_time)), Color(255, 255, 0), 2.0f);

				if (callback.hasHit())
				{
//					LOGV("[%i] hit something, t = %g, frac: %g\n", i, remaining_time, callback.m_closestHitFraction);
					float t = callback.m_closestHitFraction;
					
					// If the sweep test finds a collision and m_closestHitFraction is 0,
					// this enters an odd state because:
					// - it hits a wall, but can't move because the fraction is 0.
					// - so, velocity changes to a perpendicular vector.
					// - since the previous iteration already hit something at t = 0,
					// the next sweep test will likely report the same wall, again at t = 0.
					// This basically forces the vectors to alternate until the loop ends and prevents
					// any movement. To alleviate this, we're ALWAYS going to back up by 10%.
					t -= 0.1f;

					// lerp to that position: if we don't interpolate,
					// releasing movement keys will snap the character to its final position rather jarringly.
					target_position.setInterpolate3(target_position, new_position, t);
					
					// remove the used time (that we travel on that collision)
					// from the remaining time
					float used_time = t * remaining_time;
					remaining_time -= used_time;
	
					// the velocity that extends into the wall
					remaining_velocity = remaining_velocity - 1*remaining_velocity.dot(callback.m_hitNormalWorld)*callback.m_hitNormalWorld;
				}
				else
				{
//					LOGV("[%i] move step, %g\n", i, remaining_time);
					target_position = new_position;
					break;
				}
			}
		}
		
		void KinematicCharacter::step_down(btCollisionWorld* world, btScalar delta_time)
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
	} // namespace physics
} // namespace gemini