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

//#include "btKinematicCharacterInterface.h"
#include "BulletCollision/BroadphaseCollision/btCollisionAlgorithm.h"



class btCollisionShape;
class btRigidBody;
class btCollisionWorld;
class btCollisionDispatcher;
class btPairCachingGhostObject;

struct Camera;

#include <sdk/experimental_api.h>

namespace gemini
{

	namespace physics
	{
		class ClosestNotMeConvexResultCallback;
		
		class KinematicCharacter : public btActionInterface
		{
			btPairCachingGhostObject* ghost;

			btVector3 position;
			btVector3 target_position;
			btVector3 velocity;
			btVector3 acceleration;
			btVector3 gravity;
			btQuaternion rotation;
			
			btConvexShape* active_shape;
			
			// basis vectors for orientation
			btVector3 right;
			btVector3 view;
			
			btVector3 movement;
			
			
			btScalar step_height;
			
		public:
			KinematicCharacter(btPairCachingGhostObject* ghost_object, btConvexShape* shape);
			virtual ~KinematicCharacter() {}
		
			// action interface overrides
			virtual void updateAction(btCollisionWorld* world, btScalar delta_time);
			virtual void debugDraw(btIDebugDraw* debug_draw);

			void warp(const btVector3& target_position);
			void set_view_direction(const btVector3& right, const btVector3& view);
			
			void step_up(btCollisionWorld* world, btScalar delta_time);
			bool collide_segment(ClosestNotMeConvexResultCallback& callback, const btVector3& start, const btVector3& end);
			void step_forward_and_strafe(btCollisionWorld* world, btScalar delta_time);
			void step_down(btCollisionWorld* world, btScalar delta_time);
			
			btPairCachingGhostObject* get_ghost() const { return ghost; }
			void set_movement(const btVector3& move) { movement = move; }
		};
	
	} // namespace physics
} // namespace gemini