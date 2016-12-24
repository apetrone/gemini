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
#include <vector>

#include <core/mem.h>
#include <core/logging.h>

#include <BulletCollision/CollisionDispatch/btGhostObject.h>

#include <sdk/physics_collisionobject.h>

#include "bullet_constraint.h"
#include "bullet_common.h"
#include "bullet_debugdraw.h"


namespace gemini
{
	namespace physics
	{
		namespace bullet
		{

			class CustomGhostPairCallback : public btOverlappingPairCallback
			{
			public:
				CustomGhostPairCallback() {}
				virtual ~CustomGhostPairCallback() {}

				virtual btBroadphasePair* addOverlappingPair(btBroadphaseProxy* proxy0, btBroadphaseProxy* proxy1)
				{
					// this is called for kinematic, rigid, and static bodies
					btCollisionObject* colObj0 = (btCollisionObject*) proxy0->m_clientObject;
					btCollisionObject* colObj1 = (btCollisionObject*) proxy1->m_clientObject;
					btGhostObject* ghost0 = btGhostObject::upcast(colObj0);
					btGhostObject* ghost1 = btGhostObject::upcast(colObj1);
					ICollisionObject* obj0 = 0;
					ICollisionObject* obj1 = 0;

					if (ghost0)
					{
						obj0 = static_cast<ICollisionObject*>(ghost0->getUserPointer());
						ghost0->addOverlappingObjectInternal(proxy1, proxy0);
					}

					if (ghost1)
					{
						obj1 = static_cast<ICollisionObject*>(ghost1->getUserPointer());
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
					ICollisionObject* obj0 = 0;
					ICollisionObject* obj1 = 0;

					if (ghost0)
					{
						obj0 = static_cast<ICollisionObject*>(ghost0->getUserPointer());
						ghost0->removeOverlappingObjectInternal(proxy1,dispatcher,proxy0);
					}

					if (ghost1)
					{
						obj1 = static_cast<ICollisionObject*>(ghost1->getUserPointer());
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
			gemini::Allocator* physics_allocator = nullptr;

			btDiscreteDynamicsWorld* get_world()
			{
				return dynamics_world;
			}

			void set_world(btDiscreteDynamicsWorld* world)
			{
				dynamics_world = world;
			}


			void add_constraint(BulletConstraint* constraint)
			{
				get_world()->addConstraint(constraint->constraint);
				constraints.push_back(constraint);
			}

			void remove_constraint(BulletConstraint* constraint)
			{
				get_world()->removeConstraint(constraint->constraint);
				for (std::vector<BulletConstraint*>::iterator it = constraints.begin(); it != constraints.end(); ++it)
				{
					if (*it == constraint)
					{
						constraints.erase(it);
						break;
					}
				}
			}

			void startup(gemini::Allocator& allocator)
			{
				physics_allocator = &allocator;
				// TODO@apetrone: set up custom allocation with btAlignedAllocSetCustom.

				collision_config = new btDefaultCollisionConfiguration();
				dispatcher = new btCollisionDispatcher( collision_config );

				btVector3 worldAabbMin(-1000,-1000,-1000);
				btVector3 worldAabbMax(1000,1000,1000);

				constraint_solver = new btSequentialImpulseConstraintSolver();

				broadphase = new btDbvtBroadphase();
//				broadphase = new btAxisSweep3(worldAabbMin, worldAabbMax);

				pair_cache = broadphase->getOverlappingPairCache();
				// setup ghost pair callback instance
				pair_cache->setInternalGhostPairCallback( new CustomGhostPairCallback() );

				dynamics_world = new btDiscreteDynamicsWorld(dispatcher, (btBroadphaseInterface*)broadphase, constraint_solver, collision_config);
				dynamics_world->setGravity( btVector3( 0, -9.8f, 0 ) );
//				dynamics_world->getDispatchInfo().m_useConvexConservativeDistanceUtil = true;
//				dynamics_world->getDispatchInfo().m_convexConservativeDistanceThreshold = 0.01;
				dynamics_world->getDispatchInfo().m_allowedCcdPenetration = 0.0001f;

				// instance and set the debug renderer
				debug_renderer = MEMORY2_NEW(*physics_allocator, bullet::DebugPhysicsRenderer, core::memory::global_allocator());
				dynamics_world->setDebugDrawer(debug_renderer);
			}

			void shutdown()
			{
				// remove all constraints from objects
				for (int i = constraints.size()-1; i >= 0; --i)
				{
					MEMORY2_DELETE(*physics_allocator, constraints[i]);
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
//				LOGV("total collision shapes: %ld\n", (unsigned int)collision_shapes.size());
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
				MEMORY2_DELETE(*physics_allocator, debug_renderer);

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
			}

			void step(float fixed_step_seconds)
			{
				assert( dynamics_world != 0 );

				const int max_substeps = 0;

				// From the Bullet wiki on stepping the world: timeStep < maxSubSteps * fixedTimeStep
				// The above equation must be satisfied, otherwise we're losing time.

				// We force substeps to 0 and pass in the fixed timestep as the delta time.
				// this treats it as a variable frame delta step -- however:
				// this step function is only called when we SHOULD step at our pre-determined fixed step.
				dynamics_world->stepSimulation(fixed_step_seconds, max_substeps);


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
			}

			void debug_draw()
			{
				if (dynamics_world)
				{
					dynamics_world->debugDrawWorld();
				}
			}

			btTransform position_and_orientation_to_transform(const glm::vec3& position, const glm::quat& orientation)
			{
				btTransform transform;
				transform.setIdentity();
				transform.setRotation(btQuaternion(orientation.x, orientation.y, orientation.z, orientation.w));
				transform.setOrigin(btVector3(position.x, position.y, position.z));
				return transform;
			}

			void position_and_orientation_from_transform(glm::vec3& out_position, glm::quat& out_orientation, const btTransform& world_transform)
			{
				const btVector3& origin = world_transform.getOrigin();
				const btQuaternion& rot = world_transform.getRotation();
				out_position = glm::vec3(origin.x(), origin.y(), origin.z());
				out_orientation = glm::quat(rot.w(), rot.x(), rot.y(), rot.z());
			}
		} // namespace bullet
	} // namespace physics
} // namespace gemini
