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
#include <platform/typedefs.h>
#include <slim/xlog.h>
#include "physics.h"
#include "physics_common.h"
#include <renderer/color.h>
#include <renderer/debugdraw.h>

#include <btBulletDynamicsCommon.h>

const float PHYSICS_PLAYER_HALF_WIDTH = 0.25f; // .25 == 1.6 ft wide
const float PHYSICS_PLAYER_HALF_HEIGHT = 0.91f; // .91 == 6 ft tall

namespace physics
{
	class BulletConstraint;

	// TODO: move these to an internal namespace
	btDefaultCollisionConfiguration * collision_config;
	btCollisionDispatcher * dispatcher;
	btSequentialImpulseConstraintSolver * constraint_solver;
	btDiscreteDynamicsWorld * dynamics_world;
	btOverlappingPairCache * pair_cache;
	btAlignedObjectArray<btCollisionShape*> collision_shapes;
	btBroadphaseInterface * broadphase;
	DebugPhysicsRenderer* debug_renderer;
	std::vector<BulletConstraint*> constraints;

	// TODO: this should support an array of character controllers,
	// but for now, a single one will do.
	KinematicCharacter* _controller = 0;

	class CustomGhostPairCallback : public btOverlappingPairCallback
	{
	public:
		CustomGhostPairCallback() {}
		virtual ~CustomGhostPairCallback() {}

		virtual btBroadphasePair* addOverlappingPair(btBroadphaseProxy* proxy0, btBroadphaseProxy* proxy1)
		{
			btCollisionObject* colObj0 = (btCollisionObject*) proxy0->m_clientObject;
			btCollisionObject* colObj1 = (btCollisionObject*) proxy1->m_clientObject;
			btGhostObject* ghost0 = btGhostObject::upcast(colObj0);
			btGhostObject* ghost1 = btGhostObject::upcast(colObj1);
			CollisionObject* obj0 = 0;
			CollisionObject* obj1 = 0;
			
			if (ghost0)
			{
				obj0 = static_cast<CollisionObject*>(ghost0->getUserPointer());
				ghost0->addOverlappingObjectInternal(proxy1, proxy0);
			}
			
			if (ghost1)
			{
				obj1 = static_cast<CollisionObject*>(ghost1->getUserPointer());
				ghost1->addOverlappingObjectInternal(proxy0, proxy1);
			}

			if (obj0 && obj1)
			{
				obj0->collision_began(obj1);
				obj1->collision_began(obj0);
			}
			
			return 0;
		}
		
		virtual void* removeOverlappingPair(btBroadphaseProxy* proxy0, btBroadphaseProxy* proxy1, btDispatcher* dispatcher)
		{
			btCollisionObject* colObj0 = (btCollisionObject*) proxy0->m_clientObject;
			btCollisionObject* colObj1 = (btCollisionObject*) proxy1->m_clientObject;
			btGhostObject* ghost0 =	btGhostObject::upcast(colObj0);
			btGhostObject* ghost1 =	btGhostObject::upcast(colObj1);
			CollisionObject* obj0 = 0;
			CollisionObject* obj1 = 0;
			
			if (ghost0)
			{
				obj0 = static_cast<CollisionObject*>(ghost0->getUserPointer());
				ghost0->removeOverlappingObjectInternal(proxy1,dispatcher,proxy0);
			}
			
			if (ghost1)
			{
				obj1 = static_cast<CollisionObject*>(ghost1->getUserPointer());
				ghost1->removeOverlappingObjectInternal(proxy0,dispatcher,proxy1);
			}
			
			if (obj0 && obj1)
			{
				obj0->collision_ended(obj1);
				obj1->collision_ended(obj0);
			}
			return 0;
		}
		
		virtual void	removeOverlappingPairsContainingProxy(btBroadphaseProxy* /*proxy0*/,btDispatcher* /*dispatcher*/)
		{
			btAssert(0);
		}
	};

	class BulletCollisionObject : public CollisionObject
	{
	protected:
		btCollisionObject* object;
		
		
	public:
	
		BulletCollisionObject() : object(0) {}
	
		virtual ~BulletCollisionObject()
		{
			if (object)
			{
				remove_constraints();
			
				dynamics_world->removeCollisionObject(object);
				delete object;
				object = 0;
			}
		}
		
		void remove_constraints()
		{
			// need to remove constraints here...
			for (int i = 0; i < MAX_CONSTRAINTS_PER_OBJECT; ++i)
			{
				if (constraints[i])
				{
					constraints[i]->remove();
				}
			}
		}
		
		void set_collision_object(btCollisionObject* collision_object)
		{
			object = collision_object;
		}
		
		btCollisionObject* get_collision_object() const { return object; }
	
		virtual void set_world_position(const glm::vec3& position)
		{
			assert(object != nullptr);
			btTransform& world_transform = object->getWorldTransform();
			world_transform.setOrigin(btVector3(position.x, position.y, position.z));
			object->setWorldTransform(world_transform);
		}
		
		virtual glm::vec3 get_world_position() const
		{
			assert(object != nullptr);
			const btTransform& world_transform = object->getWorldTransform();
			const btVector3& origin = world_transform.getOrigin();
			return glm::vec3(origin.x(), origin.y(), origin.z());
		}
		
		virtual void collision_began(CollisionObject* other)
		{
			if (callback)
			{
				callback(Collision_Began, this, other);
			}
		}
		
		virtual void collision_ended(CollisionObject* other)
		{
			if (callback)
			{
				callback(Collision_Ended, this, other);
			}
		}
	};

	
	class BulletConstraint : public Constraint
	{
	public:
		btTypedConstraint* constraint;
		
		BulletConstraint(btTypedConstraint* bullet_constraint)
		{
			constraint = bullet_constraint;
			constraints.push_back(this);
			dynamics_world->addConstraint(bullet_constraint);
		}
		
		virtual ~BulletConstraint()
		{
			if (constraint)
			{
				remove();
				for (std::vector<BulletConstraint*>::iterator it = constraints.begin(); it != constraints.end(); ++it)
				{
					if (*it == this)
					{
						constraints.erase(it);
						break;
					}
				}
				delete constraint;
				constraint = 0;
			}
		}
		
		virtual void remove()
		{
			if (constraint)
			{
				dynamics_world->removeConstraint(constraint);
			}
		}
	};
	
	class BulletRigidBody : public BulletCollisionObject, public RigidBody
	{
		btCollisionShape* shape;
	
	public:
		BulletRigidBody() : shape(nullptr)
		{
			this->collision_type = physics::CollisionType_Dynamic;
		}
		
		~BulletRigidBody()
		{
			remove_constraints();
	
			btRigidBody* body = get_bullet_body();
			
			if (body && body->getMotionState())
			{
				delete body->getMotionState();
			}
			dynamics_world->removeCollisionObject(body);

			delete shape;
			shape = 0;
		}
		
		void set_collision_shape(btCollisionShape* collision_shape)
		{
			shape = collision_shape;
		}
		
		btCollisionShape* get_collision_shape() const { return shape; }
		btRigidBody* get_bullet_body() const { return  btRigidBody::upcast(object); }
		
		virtual void apply_force(const glm::vec3& force, const glm::vec3& local_position)
		{
			btRigidBody* body = get_bullet_body();
			if (body)
			{
				body->activate();
				body->applyForce(btVector3(force.x, force.y, force.z), btVector3(local_position.x, local_position.y, local_position.z));
			}
		}
		
		virtual void apply_central_force(const glm::vec3& force)
		{
			btRigidBody* body = get_bullet_body();
			if (body)
			{
				body->activate();
				body->applyCentralForce(btVector3(force.x, force.y, force.z));
			}
		}
		
		virtual void set_mass(float mass)
		{
			btRigidBody* body = get_bullet_body();
			if (body)
			{
				// update mass and inertia tensor
				btVector3 local_inertia;
				body->getCollisionShape()->calculateLocalInertia(mass, local_inertia);
				body->setMassProps(mass, local_inertia);
				body->updateInertiaTensor();
			}
		}
		
		virtual void set_parent(CollisionObject* first, CollisionObject* second)
		{
			btRigidBody* rb0 = get_bullet_body();
			btRigidBody* rb1 = (static_cast<BulletRigidBody*>(second))->get_bullet_body();

			btTypedConstraint* joint = new btPoint2PointConstraint(*rb0, *rb1, btVector3(0, 1, 0), btVector3(0, -1, 0));
			dynamics_world->addConstraint(joint);
			joint->setDbgDrawSize(btScalar(5.0f));
			
			BulletConstraint* constraint = CREATE(BulletConstraint, joint);
			first->add_constraint(constraint);
			second->add_constraint(constraint);
		}
	};


	// If I try to make KinematicCharacter derive from physics::CollisionObject,
	// the vtable is all kinds of fucked up. Until that is figured out,
	// this proxy object will have to be in place.
	struct CharacterProxyObject : public physics::CollisionObject
	{
		KinematicCharacter* character;
		
		CharacterProxyObject(KinematicCharacter* character_controller) : character(character_controller) {}
		
		virtual void set_world_position(const glm::vec3& position);
		virtual glm::vec3 get_world_position() const;
		
		virtual void collision_began(physics::CollisionObject* other) {};
		virtual void collision_ended(physics::CollisionObject* other) {};
	};
	
	void CharacterProxyObject::set_world_position(const glm::vec3 &position)
	{
		btGhostObject* ghost = character->getGhostObject();
		assert(ghost != nullptr);
		btTransform& world_transform = ghost->getWorldTransform();
		world_transform.setOrigin(btVector3(position.x, position.y, position.z));
		ghost->setWorldTransform(world_transform);
	}
	
	glm::vec3 CharacterProxyObject::get_world_position() const
	{
		btGhostObject* ghost = character->getGhostObject();
		assert(ghost != nullptr);
		const btTransform& world_transform = ghost->getWorldTransform();
		const btVector3& origin = world_transform.getOrigin();
		return glm::vec3(origin.x(), origin.y(), origin.z());
	}

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
		
		// setup ghost pair callback instance
		pair_cache->setInternalGhostPairCallback( new CustomGhostPairCallback() );
		
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
		
		// remove all constraints from objects
		for (int i = constraints.size()-1; i >= 0; --i)
		{
			DESTROY(BulletConstraint, constraints[i]);
		}
		constraints.clear();

		
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
		LOGV("total collision shapes: %ld\n", (unsigned int)collision_shapes.size());
		for ( int j = 0; j < collision_shapes.size(); j++ )
		{
			btCollisionShape* shape = collision_shapes[j];
			if (shape)
			{
				collision_shapes[j] = 0;
				delete shape;
			}
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
			
			_controller->reset(dynamics_world);
		}
		

		//int total_manifolds = dynamics_world->getDispatcher()->getNumManifolds();
		//for (int i = 0; i < total_manifolds; ++i)
		//{
		//	btPersistentManifold* manifold = dynamics_world->getDispatcher()->getManifoldByIndexInternal(i);
		//	const btCollisionObject* obj1 = static_cast<const btCollisionObject*>(manifold->getBody0());
		//	const btCollisionObject* obj2 = static_cast<const btCollisionObject*>(manifold->getBody1());

		//	int total_contacts = manifold->getNumContacts();
		//	for (int contact = 0; contact < total_contacts; ++contact)
		//	{
		//		btManifoldPoint& point = manifold->getContactPoint(contact);
		//		if (point.getDistance() < 0.0f)
		//		{
		//			const btVector3& position1 = point.getPositionWorldOnA();
		//			const btVector3& position2 = point.getPositionWorldOnB();
		//			const btVector3& normal2 = point.m_normalWorldOnB;
		//			//LOGV("normal: %2.f, %2.f, %2.f\n", normal2.x(), normal2.y(), normal2.z());
		//		}
		//	}
		//}
		
//		for (int i = 0; i < dynamics_world->getCollisionObjectArray().size(); ++i)
//		{
//			btCollisionObject* collision_object = dynamics_world->getCollisionObjectArray()[i];
//			
//			if (collision_object->getCollisionFlags() & btCollisionObject::CF_CHARACTER_OBJECT)
//			{
//				btGhostObject* ghost = btGhostObject::upcast(collision_object);
//				if (ghost)
//				{
//					LOGV("ghost userpointer: %p\n", ghost->getUserPointer());
//				}
//			}
//		}
	} // step
	
	void debug_draw()
	{
		if (dynamics_world)
		{
			dynamics_world->debugDrawWorld();
		}
	} // debug_draw
	
	RaycastInfo raycast(CollisionObject* ignored_object, const glm::vec3& start, const glm::vec3& direction, float max_distance)
	{
		glm::vec3 destination = (start + direction * max_distance);
		btVector3 ray_start(start.x, start.y, start.z);
		btVector3 ray_end(destination.x, destination.y, destination.z);
		
		BulletCollisionObject* bullet_object = static_cast<BulletCollisionObject*>(ignored_object);
		btCollisionObject* obj = bullet_object->get_collision_object();
		
		ClosestNotMeRayResultCallback callback(obj);
		dynamics_world->rayTest(ray_start, ray_end, callback);
		
		RaycastInfo info;
		
		
		if (callback.hasHit())
		{
			info.hit = start + (destination * callback.m_closestHitFraction);
			info.object = static_cast<CollisionObject*>(callback.m_collisionObject->getUserPointer());
			
			LOGV("fraction: %2.2f\n", callback.m_closestHitFraction);
			
			if (callback.m_collisionObject->getCollisionFlags() & btCollisionObject::CF_STATIC_OBJECT)
			{
				LOGV("hit: %g (static object)\n", callback.m_closestHitFraction);
			}
			else
			{
				LOGV("hit: %g (dynamic object)\n", callback.m_closestHitFraction);
			}
		}
		
		// should probably return a structure with data regarding the hit?
		return info;
	} // raycast

	KinematicCharacter* create_character_controller(const btVector3& spawnLocation, bool addActionToWorld)
	{
		btTransform tr;
		tr.setIdentity();
		
		btPairCachingGhostObject * ghost = new btPairCachingGhostObject();
		ghost->setWorldTransform( tr );
		
		btCapsuleShape* capsule_shape = new btCapsuleShape(PHYSICS_PLAYER_HALF_WIDTH, (PHYSICS_PLAYER_HALF_HEIGHT*2)-(PHYSICS_PLAYER_HALF_WIDTH*2));
		//btConvexShape * playerShape = new btCylinderShape( btVector3( .6, .90, .6 ) );

		
		ghost->setCollisionShape( capsule_shape );
		ghost->setCollisionFlags( btCollisionObject::CF_CHARACTER_OBJECT );
		
		btScalar stepHeight = btScalar(.36);
		KinematicCharacter * character = new KinematicCharacter (ghost,capsule_shape,stepHeight);
		
		///only collide with static for now (no interaction with dynamic objects)
		dynamics_world->addCollisionObject(ghost, btBroadphaseProxy::CharacterFilter, btBroadphaseProxy::StaticFilter | btBroadphaseProxy::DefaultFilter | btBroadphaseProxy::SensorTrigger);
		
		if ( addActionToWorld )
		{
			dynamics_world->addAction(character);
		}
		
		ghost->setContactProcessingThreshold( 0. );
		//m_ghost->setCcdMotionThreshold( 1 );
		ghost->setCcdMotionThreshold( .1 );
		ghost->setCcdSweptSphereRadius( 0.1 );
		
		character->SetSpawnLocation( spawnLocation );
		character->clear_state();
		
		_controller = character;
				
		return character;
	} // create_character_controller
	
	KinematicCharacter* get_character_controller(int index)
	{
		// not handling more than one at the moment
		assert(index == 0);
		
		return _controller;
	}
	
	CollisionObject* create_character_proxy(KinematicCharacter* controller)
	{
		btGhostObject* ghost = controller->getGhostObject();
		
		CharacterProxyObject* proxy = CREATE(CharacterProxyObject, controller);
		ghost->setUserPointer(proxy);
		
		return proxy;
	}

	class CustomMotionState : public btMotionState
	{
		btTransform initial_transform;
		PhysicsMotionInterface* motion_interface;
		glm::vec3 mass_center_offset;
		
	public:
		CustomMotionState(const btTransform& transform,
			PhysicsMotionInterface* motion,
			const glm::vec3& center_mass_offset) : initial_transform(transform),
													motion_interface(motion),
													mass_center_offset(center_mass_offset)
		{
		}
		
		virtual ~CustomMotionState()
		{
		}
		
		virtual void getWorldTransform(btTransform &world_transform) const
		{
			world_transform = initial_transform;
		}
		
		virtual void setWorldTransform(const btTransform &world_transform)
		{
			btQuaternion rot = world_transform.getRotation();
			btVector3 pos = world_transform.getOrigin();
			glm::quat orientation(rot.w(), rot.x(), rot.y(), rot.z());
			glm::vec3 position(pos.x(), pos.y(), pos.z());
			
			if (motion_interface)
			{
				motion_interface->set_transform(position, orientation, mass_center_offset);
			}
		}
	};

	CollisionObject* create_physics_for_mesh(assets::Mesh* mesh, float mass_kg, PhysicsMotionInterface* motion, const glm::vec3& mass_center_offset)
	{
		bool use_quantized_bvh_tree = true;
		btBvhTriangleMeshShape * trishape = 0;
		btTransform xf;
		btScalar mass(mass_kg);
		btVector3 local_inertia(0, 0, 0);
		
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
		
		// If you hit this, there will be a leak of RigidBody objects
		// as the loop below runs for each item in the geometry list
		// but this function only returns a single RigidBody.
		assert(mesh->geometry.size() == 1);
		
		int rigid_body_flags = 0;
		
		bool dynamic_body = (mass != 0.0f);
		if (!dynamic_body)
		{
			rigid_body_flags = btCollisionObject::CF_STATIC_OBJECT;
		}
		
		for( uint32_t i = 0; i < mesh->geometry.size(); ++i )
		{
			assets::Geometry* geo = &mesh->geometry[ i ];
			
			FixedArray<glm::vec3>& vertices = geo->vertices;
			
			btCollisionShape* shape = 0;
			// NOTE: Triangle shapes can ONLY be static objects.
			// TODO: look into an alternative with btGImpactMeshShape or
			// btCompoundShape + convex decomposition.
			// Could also use btConvexHullShape.
			if (dynamic_body)
			{
				shape = new btBoxShape(btVector3(0.5f, 0.5f, 0.5f));
			}
			else
			{
				// specify verts/indices from our meshdef
				// NOTE: This does NOT make a copy of the data. Whatever you pass it
				// must persist for the life of the shape.
				btTriangleIndexVertexArray * mesh = new btTriangleIndexVertexArray(geo->index_count/3, (int*)&geo->indices[0], sizeof(int)*3, geo->vertex_count, (btScalar*)&vertices[0], sizeof(glm::vec3));
			
				// use that to create a Bvh triangle mesh shape
				trishape = new btBvhTriangleMeshShape( mesh, use_quantized_bvh_tree );
				shape = trishape;
			}
			
			//collision_shapes.push_back(shape);
			rb->set_collision_shape(shape);
			
			// calculate local intertia for non-static objects
			if (dynamic_body)
			{
				shape->calculateLocalInertia(mass, local_inertia);
			}
			
			// The rigid body world transform is the center of mass. This is at the origin.
			xf.setIdentity();
			CustomMotionState * motion_state = new CustomMotionState(xf, motion, mass_center_offset);
			
			btCompoundShape* compound = new btCompoundShape();
			btTransform local_transform;
			local_transform.setIdentity();
			compound->addChildShape(local_transform, shape);
			
			btRigidBody::btRigidBodyConstructionInfo rbInfo( mass, motion_state, compound, local_inertia );
			btRigidBody * body = new btRigidBody( rbInfo );

			rb->set_collision_object(body);
			body->setUserPointer(rb);
			
			if (dynamic_body)
			{
				body->setRestitution(0.25f);
				body->setFriction(0.5f);
				body->setCcdMotionThreshold(0.1f);
				body->setCcdSweptSphereRadius(0.1f);
				body->setUserPointer(0);
			}
			else
			{
				int body_flags = body->getCollisionFlags() | rigid_body_flags;
				body->setCollisionFlags( body_flags );
				body->setFriction(0.75f);
			}
			
			dynamics_world->addRigidBody(body);
		}
		
		return rb;
	} // create_physics_for_mesh

	CollisionObject* create_trigger(const glm::vec3& size)
	{
		btPairCachingGhostObject* ghost = new btPairCachingGhostObject();
		
		BulletCollisionObject* collision_object = CREATE(BulletCollisionObject);
		ghost->setUserPointer(collision_object);
		
		collision_object->set_collision_object(ghost);

		btCollisionShape* shape = new btBoxShape(btVector3(size.x, size.y, size.z));
		collision_shapes.push_back(shape);

		ghost->setCollisionShape(shape);
		ghost->setCollisionFlags(ghost->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);

		btTransform tr;
		tr.setIdentity();
		ghost->setWorldTransform(tr);

		// There was a fix in September of 2013 which fixed sensors and characters, see: https://code.google.com/p/bullet/issues/detail?id=719
		dynamics_world->addCollisionObject(ghost, btBroadphaseProxy::SensorTrigger, btBroadphaseProxy::CharacterFilter);
		
		return collision_object;
	} // create_trigger

}; // namespace physics