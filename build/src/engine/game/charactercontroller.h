// -------------------------------------------------------------
// Copyright (C) 2004-2010 Adam Petrone

//	This file is part of the aengine source code.
//
//  aengine is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.

//  This source is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//  GNU General Public License for more details.

//  You should have received a copy of the GNU General Public License
//  along with this source.  If not, see <http://www.gnu.org/licenses/>.
// -------------------------------------------------------------

#pragma once

#include "BulletDynamics/Character/btCharacterControllerInterface.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h"

#define AENGINE_CHARACTER_CONTROLLER_HYBRID 1

extern btVector3 upAxis;


#define AENGINE_CHARACTER_FUNCS 0

#if AENGINE_CHARACTER_FUNCS
#define F( name ) printf( "Function - %s()\n", #name )
#else
#define F( name )
#endif

#define BTV3( v ) v.x(), v.y(), v.z()



//
//  Chris Rodgers' Character Controller
//

#include "LinearMath/btVector3.h"

//#include "btCharacterControllerInterface.h"
#include "BulletDynamics/Character/btCharacterControllerInterface.h"

#include "BulletCollision/BroadphaseCollision/btCollisionAlgorithm.h"


class btCollisionShape;
class btRigidBody;
class btCollisionWorld;
class btCollisionDispatcher;
class btPairCachingGhostObject;


namespace physics
{
	///CharacterController is an object that supports a sliding motion in a world.
	///It uses a ghost object and convex sweep test to test for upcoming collisions. This is combined with discrete collision detection to recover from penetrations.
	///Interaction between CharacterController and dynamic rigid bodies needs to be explicity implemented by the user.
	class CharacterController : public btCharacterControllerInterface
	{
	protected:
		btScalar m_halfHeight;

		btPairCachingGhostObject* m_ghostObject;
		btConvexShape*	m_convexShape;//is also in m_ghostObject, but it needs to be convex, so we store it here to avoid upcast

		btScalar m_verticalVelocity;
		btScalar m_verticalOffset;
		btScalar m_fallSpeed;
		btScalar m_jumpSpeed;
		btScalar m_maxJumpHeight;
		btScalar m_maxSlopeRadians; // Slope angle that is set (used for returning the exact value)
		btScalar m_maxSlopeCosine;  // Cosine equivalent of m_maxSlopeRadians (calculated once when set, for optimization)
		btScalar m_gravity;
		btScalar m_movementDamping;

		btScalar m_turnAngle;

		btScalar m_stepHeight;

		btScalar	m_addedMargin;//@todo: remove this and fix the code

		///this is the desired walk direction, set by the user
		btVector3	m_walkDirection;
		btVector3	m_normalizedDirection;
		btVector3 m_airMove;
		btVector3 m_velocity;

		//some internal variables
		btVector3 m_currentPosition;
		btScalar  m_currentStepOffset;
		btVector3 m_targetPosition;

		///keep track of the contact manifolds
		btManifoldArray	m_manifoldArray;

		bool m_touchingContact;
		btVector3 m_touchingNormal;

		bool  m_wasOnGround;
		bool m_prevOnGround;
		bool  m_wasJumping;
		bool	m_useGhostObjectSweepTest;
		bool	m_useWalkDirection;
		bool m_dampenMovement;
		btScalar	m_velocityTimeInterval;
		int m_upAxis;

		static btVector3* getUpAxisDirections();


		btVector3 m_accMoveDir;


		btVector3 computeReflectionDirection (const btVector3& direction, const btVector3& normal);
		btVector3 parallelComponent (const btVector3& direction, const btVector3& normal);
		btVector3 perpindicularComponent (const btVector3& direction, const btVector3& normal);

		bool recoverFromPenetration ( btCollisionWorld* collisionWorld);
		void stepUp (btCollisionWorld* collisionWorld);
		void updateTargetPositionBasedOnCollision (const btVector3& hit_normal, btScalar tangentMag = btScalar(0.0), btScalar normalMag = btScalar(1.0));
		void stepForwardAndStrafe (btCollisionWorld* collisionWorld, const btVector3& walkMove, const btVector3 & airMove, btScalar dt);
		void stepDown (btCollisionWorld* collisionWorld, btScalar dt);

		float mFrontMove;
		float mBackMove;
		float mLeftMove;
		float mRightMove;

		btVector3 m_moveWeight;
		btVector3 m_prevMoveWeight;

		btVector3 mDirFront;
		btVector3 mDirRight;
		btVector3 mSpawnPoint;

		bool bEnableFreeCam;

	public:
		CharacterController (btPairCachingGhostObject* ghostObject,btConvexShape* convexShape,btScalar stepHeight, int upAxis = 1);
		virtual ~CharacterController ();

		void StopWalking();

		void setMovementWeight( float fwd, float back, float left, float right );
		void setFacingDirections( const btVector3 & front, const btVector3 & right );
		void enableDamping( bool bEnable );
		void setDamping( btScalar dampingFactor );
		const btVector3 getVelocity();
		btVector3 getOrigin();

		///btActionInterface interface
		virtual void updateAction( btCollisionWorld* collisionWorld,btScalar deltaTime);

//		virtual void handle_collision(CollisionObject* other);

		///btActionInterface interface
		void	debugDraw(btIDebugDraw* debugDrawer);

		void setUpAxis (int axis)
		{
			if (axis < 0)
				axis = 0;
			if (axis > 2)
				axis = 2;
			m_upAxis = axis;
		}

		/// This should probably be called setPositionIncrementPerSimulatorStep.
		/// This is neither a direction nor a velocity, but the amount to
		///	increment the position each simulation iteration, regardless
		///	of dt.
		/// This call will reset any velocity set by setVelocityForTimeInterval().
		virtual void	setWalkDirection(const btVector3& walkDirection);

		/// Caller provides a velocity with which the character should move for
		///	the given time period.  After the time period, velocity is reset
		///	to zero.
		/// This call will reset any walk direction set by setWalkDirection().
		/// Negative time intervals will result in no motion.
		virtual void setVelocityForTimeInterval(const btVector3& velocity,
					btScalar timeInterval);

		virtual void reset (btCollisionWorld* collisionWorld);
		virtual void setUpInterpolate(bool value);
		void clear_state();
		
		void warp (const btVector3& origin);

		void preStep (  btCollisionWorld* collisionWorld);
		void playerStep ( btCollisionWorld* collisionWorld, btScalar dt);

		void setFallSpeed (btScalar fallSpeed);
		void setJumpSpeed (btScalar jumpSpeed);
		void setMaxJumpHeight (btScalar maxJumpHeight);
		bool canJump () const;

		void jump ();

		void setGravity(btScalar gravity);
		btScalar getGravity() const;

		void ToggleFreeCam();

		/// The max slope determines the maximum angle that the controller can walk up.
		/// The slope angle is measured in radians.
		void setMaxSlope(btScalar slopeRadians);
		btScalar getMaxSlope() const;

		btPairCachingGhostObject* getGhostObject();
		void	setUseGhostSweepTest(bool useGhostObjectSweepTest)
		{
			m_useGhostObjectSweepTest = useGhostObjectSweepTest;
		}

		bool onGround () const;

		void SetSpawnLocation( const btVector3 & spawn );
	};
} // namespace physics