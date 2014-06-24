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

#include <iostream>
#include "btBulletDynamicsCommon.h"

#include "charactercontroller.h"



#define SMOOTH_UNIDIRECTIONAL_MOVEMENT 1
#define CHARACTER_MOVEMENT_MULTIPLIER 1.5
//
//  Hybrid Character Controller
//
#if AENGINE_CHARACTER_CONTROLLER_HYBRID

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
//#include "btKinematicCharacterController.h"



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


///@todo Interact with dynamic objects,
///Ride kinematicly animated platforms properly
///More realistic (or maybe just a config option) falling
/// -> Should integrate falling velocity manually and use that in stepDown()
///Support jumping
///Support ducking
class btKinematicClosestNotMeRayResultCallback : public btCollisionWorld::ClosestRayResultCallback
{
public:
	btKinematicClosestNotMeRayResultCallback (btCollisionObject* me) : btCollisionWorld::ClosestRayResultCallback(btVector3(0.0, 0.0, 0.0), btVector3(0.0, 0.0, 0.0))
	{
		m_me = me;
	}

	virtual btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult,bool normalInWorldSpace)
	{
		if (rayResult.m_collisionObject == m_me)
			return 1.0;

		return ClosestRayResultCallback::addSingleResult (rayResult, normalInWorldSpace);
	}
protected:
	btCollisionObject* m_me;
};

class btKinematicClosestNotMeConvexResultCallback : public btCollisionWorld::ClosestConvexResultCallback
{
public:
	btKinematicClosestNotMeConvexResultCallback (btCollisionObject* me, const btVector3& up, btScalar minSlopeDot)
	: btCollisionWorld::ClosestConvexResultCallback(btVector3(0.0, 0.0, 0.0), btVector3(0.0, 0.0, 0.0))
	, m_me(me)
	, m_up(up)
	, m_minSlopeDot(minSlopeDot)
	{
	}

	virtual btScalar addSingleResult(btCollisionWorld::LocalConvexResult& convexResult,bool normalInWorldSpace)
	{
		if (convexResult.m_hitCollisionObject == m_me)
			return btScalar(1.0);

		btVector3 hitNormalWorld;
		if (normalInWorldSpace)
		{
			hitNormalWorld = convexResult.m_hitNormalLocal;
		} else
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

/*
 * Returns the reflection direction of a ray going 'direction' hitting a surface with normal 'normal'
 *
 * from: http://www-cs-students.stanford.edu/~adityagp/final/node3.html
 */
btVector3 btKinematicCharacterController::computeReflectionDirection (const btVector3& direction, const btVector3& normal)
{
	return direction - (btScalar(2.0) * direction.dot(normal)) * normal;
}

/*
 * Returns the portion of 'direction' that is parallel to 'normal'
 */
btVector3 btKinematicCharacterController::parallelComponent (const btVector3& direction, const btVector3& normal)
{
	btScalar magnitude = direction.dot(normal);
	return normal * magnitude;
}

/*
 * Returns the portion of 'direction' that is perpindicular to 'normal'
 */
btVector3 btKinematicCharacterController::perpindicularComponent (const btVector3& direction, const btVector3& normal)
{
	return direction - parallelComponent(direction, normal);
}

btKinematicCharacterController::btKinematicCharacterController (btPairCachingGhostObject* ghostObject,btConvexShape* convexShape,btScalar stepHeight, int upAxis)
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

btKinematicCharacterController::~btKinematicCharacterController ()
{
}

btPairCachingGhostObject* btKinematicCharacterController::getGhostObject()
{
	return m_ghostObject;
}

void btKinematicCharacterController::StopWalking()
{

	//m_useWalkDirection = false;
	m_dampenMovement = true;
}

bool btKinematicCharacterController::recoverFromPenetration ( btCollisionWorld* collisionWorld)
{

	bool penetration = false;

	collisionWorld->getDispatcher()->dispatchAllCollisionPairs(m_ghostObject->getOverlappingPairCache(), collisionWorld->getDispatchInfo(), collisionWorld->getDispatcher());

	m_currentPosition = m_ghostObject->getWorldTransform().getOrigin();

	btScalar maxPen = btScalar(0.0);
	for (int i = 0; i < m_ghostObject->getOverlappingPairCache()->getNumOverlappingPairs(); i++)
	{
		m_manifoldArray.resize(0);

		btBroadphasePair* collisionPair = &m_ghostObject->getOverlappingPairCache()->getOverlappingPairArray()[i];

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
//	printf("m_touchingNormal = %f,%f,%f\n",m_touchingNormal[0],m_touchingNormal[1],m_touchingNormal[2]);
	return penetration;
}

void btKinematicCharacterController::stepUp ( btCollisionWorld* world)
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

	btKinematicClosestNotMeConvexResultCallback callback (m_ghostObject, -getUpAxisDirections()[m_upAxis], btScalar(0.7071));
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

void btKinematicCharacterController::updateTargetPositionBasedOnCollision (const btVector3& hitNormal, btScalar tangentMag, btScalar normalMag)
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

void btKinematicCharacterController::setMovementWeight( float _forward, float _back, float _left, float _right )
{
	mFrontMove = _forward * movementWeightFactor;
	//printf( "Front Move: %g\n", mFrontMove );
	mBackMove = _back * movementWeightFactor;
	mLeftMove = _left * movementWeightFactor;
	mRightMove = _right * movementWeightFactor;
}

void btKinematicCharacterController::setFacingDirections( const btVector3 & front, const btVector3 & right )
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

void btKinematicCharacterController::enableDamping( bool bEnable )
{
	m_dampenMovement = bEnable;
}

void btKinematicCharacterController::stepForwardAndStrafe ( btCollisionWorld* collisionWorld, const btVector3& walkMove, const btVector3 & airMove, btScalar dt)
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

		btKinematicClosestNotMeConvexResultCallback callback (m_ghostObject, sweepDirNegative, btScalar(0.0));
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

void btKinematicCharacterController::stepDown ( btCollisionWorld* collisionWorld, btScalar dt)
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

	btKinematicClosestNotMeConvexResultCallback callback (m_ghostObject, getUpAxisDirections()[m_upAxis], m_maxSlopeCosine);
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



void btKinematicCharacterController::setWalkDirection
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



void btKinematicCharacterController::setVelocityForTimeInterval
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

void btKinematicCharacterController::SetSpawnLocation( const btVector3 & spawn )
{
	mSpawnPoint = spawn;
}

void btKinematicCharacterController::reset ()
{
	F( reset );

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

void btKinematicCharacterController::warp (const btVector3& origin)
{
	F( warp );

	btTransform xform;
	xform.setIdentity();
	xform.setOrigin (origin);
	m_ghostObject->setWorldTransform (xform);
}

void btKinematicCharacterController::preStep (  btCollisionWorld* collisionWorld)
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

void btKinematicCharacterController::updateAction( btCollisionWorld* collisionWorld,btScalar deltaTime)
{
	//printf( "------------------ Update Action Begin------------\n" );
	preStep ( collisionWorld);
	//printf( "CurrentPosition: %g %g %g\n", BTV3( m_currentPosition ) );
	playerStep (collisionWorld, deltaTime);
	//printf( "------------------ Update Action End------------\n" );
}
#include <stdio.h>

void btKinematicCharacterController::playerStep (  btCollisionWorld* collisionWorld, btScalar dt)
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

	btVector3 fbMove;
	float fbw = (mFrontMove - mBackMove) * dt * moveSpeed;



	//fbMove = (mDirFront * (mFrontMove * dt * moveSpeed));
	//fbMove -= (mDirFront * (mBackMove * dt * moveSpeed));

	if ( fbw > 1.0 )
		fbw = 1.0;
	else if ( fbw < -1.0 )
		fbw = -1.0f;

	fbMove = (mDirFront * fbw);

	btVector3 lrMove;
	lrMove.setZero();
	float lrw = (mRightMove - mLeftMove) * dt * moveSpeed;
	lrMove = (mDirRight * lrw);

	//printf( "mFrontMove: %g | mBackMove: %g (%g)| mLeftMove: %g | mRightMove: %g (%g)\n", mFrontMove, mBackMove, fbw, mLeftMove, mRightMove, lrw );

	//if ( mFrontMove > 0 || mBackMove > 0 || mLeftMove > 0 || mRightMove > 0 )


	//printf( "FBMove: %g %g %g - LRMove: %g %g %g\n", BTV3( fbMove ), BTV3( lrMove ) );
	//printf( "m_velocity: %g %g %g\n", BTV3( m_velocity ) );

	btScalar length = 0;
	const float MAX_MAG = 3.0f;

#if SMOOTH_UNIDIRECTIONAL_MOVEMENT
    m_moveWeight = (fbMove+lrMove);

    length = m_moveWeight.length();

    if ( length > MAX_MAG )
    {
        m_moveWeight = (m_moveWeight*(MAX_MAG/length));
    }

	m_velocity += (m_moveWeight * .5) + (m_prevMoveWeight * .5);
	//m_velocity += m_moveWeight;
	m_prevMoveWeight = m_moveWeight;

	length = m_velocity.length();

	//printf( "Length: %g\n", length );

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
}

void btKinematicCharacterController::setFallSpeed (btScalar fallSpeed)
{
	m_fallSpeed = fallSpeed;
}

void btKinematicCharacterController::setJumpSpeed (btScalar jumpSpeed)
{
	m_jumpSpeed = jumpSpeed;
}

void btKinematicCharacterController::setMaxJumpHeight (btScalar maxJumpHeight)
{
	m_maxJumpHeight = maxJumpHeight;
}

bool btKinematicCharacterController::canJump () const
{
	return onGround();
}

void btKinematicCharacterController::jump ()
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

void btKinematicCharacterController::setGravity(btScalar gravity)
{
	m_gravity = gravity;
}

btScalar btKinematicCharacterController::getGravity() const
{
	return m_gravity;
}

void btKinematicCharacterController::setMaxSlope(btScalar slopeRadians)
{
	m_maxSlopeRadians = slopeRadians;
	m_maxSlopeCosine = btCos(slopeRadians);
}

btScalar btKinematicCharacterController::getMaxSlope() const
{
	return m_maxSlopeRadians;
}

bool btKinematicCharacterController::onGround () const
{
	//return ( m_verticalVelocity < 0.001 && m_verticalOffset < 0.001 );

	return m_verticalVelocity == 0.0 && m_verticalOffset == 0.0;
}


btVector3* btKinematicCharacterController::getUpAxisDirections()
{
	static btVector3 sUpAxisDirection[3] = { btVector3(1.0f, 0.0f, 0.0f), btVector3(0.0f, 1.0f, 0.0f), btVector3(0.0f, 0.0f, 1.0f) };

	return sUpAxisDirection;
}

void btKinematicCharacterController::debugDraw(btIDebugDraw* debugDrawer)
{
}

void btKinematicCharacterController::ToggleFreeCam()
{
	bEnableFreeCam = !bEnableFreeCam;
}

void btKinematicCharacterController::setDamping( btScalar dampingFactor )
{
	m_movementDamping = dampingFactor;
}

const btVector3 btKinematicCharacterController::getVelocity()
{
	return m_velocity*CHARACTER_MOVEMENT_MULTIPLIER;	
}

btVector3 btKinematicCharacterController::getOrigin()
{
	return this->getGhostObject()->getWorldTransform().getOrigin();
}

#endif
