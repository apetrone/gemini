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
#include "typedefs.h"
#include <slim/xlog.h>
#include "physics.hpp"
#include "color.hpp"
#include "debugdraw.hpp"

#include <btBulletDynamicsCommon.h>

const float PHYSICS_PLAYER_HALF_WIDTH = 0.325f;
const float PHYSICS_PLAYER_HALF_HEIGHT = 0.91f;

namespace physics
{
	// TODO: move these to an internal namespace
	btDefaultCollisionConfiguration * collision_config;
	btCollisionDispatcher * dispatcher;
	btSequentialImpulseConstraintSolver * constraint_solver;
	btDiscreteDynamicsWorld * dynamics_world;
	btOverlappingPairCache * pair_cache;
	btAlignedObjectArray<btCollisionShape*> collision_shapes;
	btBroadphaseInterface * broadphase;
	DebugPhysicsRenderer* debug_renderer;

	// TODO: this should support an array of character controllers,
	// but for now, a single one will do.
	CharacterController* _controller = 0;
	
	void DebugPhysicsRenderer::drawLine( const btVector3 & from, const btVector3 & to, const btVector3 & color )
	{
		Color c = Color::fromFloatPointer( &color[0], 3 );
		debugdraw::line( BTVECTOR3_TO_VEC3( from ), BTVECTOR3_TO_VEC3( to ), c );
	}
	
	void DebugPhysicsRenderer::drawLine( const btVector3& from, const btVector3& to, const btVector3& fromColor, const btVector3& color )
	{
		Color c = Color::fromFloatPointer( &color[0], 3 );
		debugdraw::line( BTVECTOR3_TO_VEC3( from ), BTVECTOR3_TO_VEC3( to ), c );
	}
	
	void DebugPhysicsRenderer::drawContactPoint(const btVector3& PointOnB,const btVector3& normalOnB,btScalar distance,int lifeTime,const btVector3& color)
	{
	}
	
	void DebugPhysicsRenderer::reportErrorWarning(const char* warningString)
	{
		LOGE("[bullet2] %s\n", warningString );
	}
	
	void DebugPhysicsRenderer::draw3dText(const btVector3& location,const char* textString)
	{
	}
	
	void DebugPhysicsRenderer::setDebugMode(int debugMode)
	{
	}
	
	int	DebugPhysicsRenderer::getDebugMode() const
	{
		return (btIDebugDraw::DBG_DrawWireframe | btIDebugDraw::DBG_DrawAabb);
	}


	
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
		collision_config = new btDefaultCollisionConfiguration();
		dispatcher = new btCollisionDispatcher( collision_config );
		
		btVector3 worldAabbMin(-1000,-1000,-1000);
		btVector3 worldAabbMax(1000,1000,1000);
		
		constraint_solver = new btSequentialImpulseConstraintSolver();
		
		//		broadphase = new btDbvtBroadphase();
		broadphase = new btAxisSweep3(worldAabbMin, worldAabbMax);
		
		pair_cache = broadphase->getOverlappingPairCache();
		dynamics_world = new btDiscreteDynamicsWorld(dispatcher, (btBroadphaseInterface*)broadphase, constraint_solver, collision_config);
		dynamics_world->setGravity( btVector3( 0, -10, 0 ) );
		dynamics_world->getDispatchInfo().m_useConvexConservativeDistanceUtil = true;
		dynamics_world->getDispatchInfo().m_convexConservativeDistanceThreshold = 0.01;
		dynamics_world->getDispatchInfo().m_allowedCcdPenetration = 0.0;
		
		
		//btAlignedAllocSetCustom( bullet2_custom_alloc, bullet2_custom_free );
		
		// instance and set the debug renderer
		debug_renderer = CREATE(DebugPhysicsRenderer);
		dynamics_world->setDebugDrawer(debug_renderer);
	}
	
	void shutdown()
	{
		//
		// Cleanup
		//
		for( int i = dynamics_world->getNumCollisionObjects()-1; i >= 0; --i )
		{
			btCollisionObject * obj = dynamics_world->getCollisionObjectArray()[i];
			btRigidBody * body = btRigidBody::upcast(obj);
			if ( body && body->getMotionState() )
			{
				delete body->getMotionState();
			}
			dynamics_world->removeCollisionObject( obj );
			delete obj;
		}
		
		//delete collision shapes
		for ( int j = 0; j < collision_shapes.size(); j++ )
		{
			btCollisionShape* shape = collision_shapes[j];
			collision_shapes[j] = 0;
			delete shape;
		}
		
		dynamics_world->setDebugDrawer(0);
		DESTROY(DebugPhysicsRenderer, debug_renderer);
		
		//delete dynamics world
		delete dynamics_world;
		
		//delete solver
		delete constraint_solver;
		
		//delete broadphase
		//delete mPairCache;
		delete broadphase;
		
		//delete dispatcher
		delete dispatcher;
		
		// delete the collision configuration
		delete collision_config;
		
		//next line is optional: it will be cleared by the destructor when the array goes out of scope
		collision_shapes.clear();
		
		
		// cleanup controllers
//		delete _controller;
	} // shutdown
	
	
	void step( float seconds )
	{
		assert( dynamics_world != 0 );
		
		dynamics_world->stepSimulation( seconds, 1, 1/60.0f );
		
		// TODO: update actions
		if (_controller)
		{
			_controller->updateAction(dynamics_world, 1/60.0f);
		}
		
	} // step

	CharacterController* create_character_controller(const btVector3& spawnLocation, bool addActionToWorld)
	{
		btTransform tr;
		tr.setIdentity();
		
		btPairCachingGhostObject * ghost = new btPairCachingGhostObject();
		ghost->setWorldTransform( tr );
		
		// what the hell does this do?
		pair_cache->setInternalGhostPairCallback( new btGhostPairCallback() );
		
		btConvexShape* playerShape = new btCapsuleShape((PHYSICS_PLAYER_HALF_WIDTH*2), (PHYSICS_PLAYER_HALF_HEIGHT*2));
		//btConvexShape * playerShape = new btCylinderShape( btVector3( .6, .90, .6 ) );
		
		ghost->setCollisionShape( playerShape );
		ghost->setCollisionFlags( btCollisionObject::CF_CHARACTER_OBJECT );
		
		btScalar stepHeight = btScalar(.36);
		CharacterController * character = new CharacterController (ghost,playerShape,stepHeight);
		
		///only collide with static for now (no interaction with dynamic objects)
		dynamics_world->addCollisionObject(ghost, btBroadphaseProxy::CharacterFilter, btBroadphaseProxy::StaticFilter | btBroadphaseProxy::DefaultFilter);
		
		if ( addActionToWorld )
		{
			dynamics_world->addAction(character);
		}
		
		ghost->setContactProcessingThreshold( 0. );
		//m_ghost->setCcdMotionThreshold( 1 );
		ghost->setCcdMotionThreshold( .1 );
		ghost->setCcdSweptSphereRadius( 0.1 );
		
		character->SetSpawnLocation( spawnLocation );
		character->reset();
		
		_controller = character;
		
		return character;
	} // create_character_controller
	
	void CopyGhostToCamera(btPairCachingGhostObject* ghost, Camera& cam)
	{
		btTransform tr = ghost->getWorldTransform();
		btVector3 origin = tr.getOrigin();
		origin += btVector3(0, .9, 0);
		
		cam.pos = glm::vec3(origin.x(), origin.y(), origin.z());
		cam.update_view();
	} // CopyGhostToCamera

}; // namespace physics