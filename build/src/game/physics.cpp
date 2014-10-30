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
#include <gemini/typedefs.h>
#include <slim/xlog.h>
#include "physics.h"
#include "color.h"
#include "debugdraw.h"

#include <btBulletDynamicsCommon.h>

const float PHYSICS_PLAYER_HALF_WIDTH = 0.25f; // .25 == 1.6 ft wide
const float PHYSICS_PLAYER_HALF_HEIGHT = 0.91f; // .91 == 6 ft tall

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
	
	
	class BulletRigidBody : public RigidBody
	{
	
		
	
	public:
	
		btRigidBody* body;
	
	
		BulletRigidBody() : body(nullptr)
		{
			
		}
		
		virtual void set_world_position(const glm::vec3& position)
		{
			assert(body != nullptr);
			
			btTransform& world_transform = body->getWorldTransform();
			world_transform.setOrigin(btVector3(position.x, position.y, position.z));
			body->setWorldTransform(world_transform);
		}
		
		virtual glm::vec3 get_world_position() const
		{
			assert(body != nullptr);
						
			const btTransform& world_transform = body->getWorldTransform();
			const btVector3& origin = world_transform.getOrigin();
			return glm::vec3(origin.x(), origin.y(), origin.z());
		}
	};
	
	
	
	void DebugPhysicsRenderer::drawLine( const btVector3 & from, const btVector3 & to, const btVector3 & color )
	{
		Color c = Color::fromFloatPointer( &color[0], 3 );
		debugdraw::line( BTVECTOR3_TO_VEC3( from ), BTVECTOR3_TO_VEC3( to ), c, 0 );
	}
	
	void DebugPhysicsRenderer::drawLine( const btVector3& from, const btVector3& to, const btVector3& fromColor, const btVector3& color )
	{
		Color c = Color::fromFloatPointer( &color[0], 3 );
		debugdraw::line( BTVECTOR3_TO_VEC3( from ), BTVECTOR3_TO_VEC3( to ), c, 0 );
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
		return (btIDebugDraw::DBG_DrawWireframe); // | btIDebugDraw::DBG_DrawAabb;
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
	
	void debug_draw()
	{
		if (dynamics_world)
		{
			dynamics_world->debugDrawWorld();
		}
	} // debug_draw

	CharacterController* create_character_controller(const btVector3& spawnLocation, bool addActionToWorld)
	{
		btTransform tr;
		tr.setIdentity();
		
		btPairCachingGhostObject * ghost = new btPairCachingGhostObject();
		ghost->setWorldTransform( tr );
		
		// what the hell does this do?
		pair_cache->setInternalGhostPairCallback( new btGhostPairCallback() );
		
		btCapsuleShape* capsule_shape = new btCapsuleShape(PHYSICS_PLAYER_HALF_WIDTH, (PHYSICS_PLAYER_HALF_HEIGHT*2)-(PHYSICS_PLAYER_HALF_WIDTH*2));
		//btConvexShape * playerShape = new btCylinderShape( btVector3( .6, .90, .6 ) );

		
		ghost->setCollisionShape( capsule_shape );
		ghost->setCollisionFlags( btCollisionObject::CF_CHARACTER_OBJECT );
		
		btScalar stepHeight = btScalar(.36);
		CharacterController * character = new CharacterController (ghost,capsule_shape,stepHeight);
		
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
	
	void copy_ghost_to_camera(btPairCachingGhostObject* ghost, Camera& cam)
	{
		btTransform tr = ghost->getWorldTransform();
		btVector3 origin = tr.getOrigin();
		origin += btVector3(0, .9, 0);
		
		cam.pos = glm::vec3(origin.x(), origin.y(), origin.z());
	} // copy_ghost_to_camera
	
	void player_move(CharacterController* character, Camera& camera, const MovementCommand& command)
	{
		glm::vec3 cam_right = camera.side;
		glm::vec3 cam_dir = camera.view;
		
		if (character)
		{
			character->setFacingDirections(
				btVector3(cam_dir.x, cam_dir.y, cam_dir.z),
				btVector3(cam_right.x, cam_right.y, cam_right.z)
			);
			
			character->setMovementWeight(
				command.forward,
				command.back,
				command.left,
				command.right
			);
				
			bool movement_is_zero = (
				command.forward == 0 &&
				command.right == 0 &&
				command.left == 0 &&
				command.back == 0
			);
			
			character->enableDamping(movement_is_zero);
		}
	} // player_move

	RigidBody* create_physics_for_mesh(assets::Mesh* mesh, float mass_kg)
	{
		bool use_quantized_bvh_tree = true;
		btBvhTriangleMeshShape * trishape = 0;
		btTransform xf;
		btScalar mass(mass_kg);
		btVector3 localInertia(0, 0, 0);
		
		if (!mesh)
		{
			LOGW("Unable to create physics for null mesh\n");
			return nullptr;
		}
		
		if (!dynamics_world)
		{
			LOGE("Unable to add physics for mesh; invalid physics state\n");
			return nullptr;
		}
		
		BulletRigidBody* rb = 0;
		rb = CREATE(BulletRigidBody);
		
		for( uint32_t i = 0; i < mesh->geometry.size(); ++i )
		{
			assets::Geometry* geo = &mesh->geometry[ i ];
			
			FixedArray<glm::vec3>& vertices = geo->vertices;
			
			
			
			// specify verts/indices from our meshdef
			// NOTE: This does NOT make a copy of the data. Whatever you pass it
			// must persist for the life of the shape.
			btTriangleIndexVertexArray * mesh = new btTriangleIndexVertexArray(geo->index_count/3, (int*)&geo->indices[0], sizeof(int)*3, geo->vertex_count, (btScalar*)&vertices[0], sizeof(glm::vec3));
			
			// use that to creat ea Bvh triangle mesh shape
			trishape = new btBvhTriangleMeshShape( mesh, use_quantized_bvh_tree );
			
			// setup transform and parameters for static rigid body
			xf.setIdentity();
			
			btDefaultMotionState * myMotionState = new btDefaultMotionState( xf );
			btRigidBody::btRigidBodyConstructionInfo rbInfo( mass, myMotionState, trishape, localInertia );
			btRigidBody * body = new btRigidBody( rbInfo );
			rb->body = body;
			body->setCollisionFlags( body->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT );
			
			collision_shapes.push_back(trishape);
			dynamics_world->addRigidBody(body);
		}
		
		return rb;
	} // create_physics_for_mesh
}; // namespace physics