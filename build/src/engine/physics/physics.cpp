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
#include <btBulletDynamicsCommon.h>
#include "physics_common.h"
#include <renderer/color.h>
#include <renderer/debugdraw.h>

#include "charactercontroller.h"

#include "assets/asset_mesh.h"

#include <vector>

const float PHYSICS_PLAYER_HALF_WIDTH = 0.25f; // .25 == 1.6 ft wide
const float PHYSICS_PLAYER_HALF_HEIGHT = 0.91f; // .91 == 6 ft tall

#include "physics/bullet/bullet_collisionobject.h"
#include "physics/bullet/bullet_constraint.h"
#include "physics/bullet/bullet_debugdraw.h"


#include "physics_interface.h"

namespace gemini
{
	namespace physics
	{


		// TODO: this should support an array of character controllers,
		// but for now, a single one will do.
		KinematicCharacter* _controller = 0;		
		
		


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
			PhysicsInterface* physics_interface = CREATE(PhysicsInterfaceImpl);
			api::set_instance(physics_interface);
		
			bullet::startup();
		}
		
		void shutdown()
		{
			//
			// Cleanup
			//

			bullet::shutdown();
			
			PhysicsInterface* physics_interface = api::instance();
			DESTROY(PhysicsInterface, physics_interface);
		} // shutdown
		
		
		void step( float seconds )
		{
			bullet::step(seconds);
		} // step
		
		void debug_draw()
		{
			bullet::debug_draw();
		} // debug_draw
		
		RaycastInfo raycast(CollisionObject* ignored_object, const glm::vec3& start, const glm::vec3& direction, float max_distance)
		{
//			glm::vec3 destination = (start + direction * max_distance);
//			btVector3 ray_start(start.x, start.y, start.z);
//			btVector3 ray_end(destination.x, destination.y, destination.z);
//			
//			BulletCollisionObject* bullet_object = static_cast<BulletCollisionObject*>(ignored_object);
//			btCollisionObject* obj = bullet_object->get_collision_object();
//			
//			ClosestNotMeRayResultCallback callback(obj);
//			dynamics_world->rayTest(ray_start, ray_end, callback);
			
			RaycastInfo info;
			
			
//			if (callback.hasHit())
//			{
//				info.hit = start + (destination * callback.m_closestHitFraction);
//				info.object = static_cast<CollisionObject*>(callback.m_collisionObject->getUserPointer());
//				
//				LOGV("fraction: %2.2f\n", callback.m_closestHitFraction);
//				
//				if (callback.m_collisionObject->getCollisionFlags() & btCollisionObject::CF_STATIC_OBJECT)
//				{
//					LOGV("hit: %g (static object)\n", callback.m_closestHitFraction);
//				}
//				else
//				{
//					LOGV("hit: %g (dynamic object)\n", callback.m_closestHitFraction);
//				}
//			}
			
			// should probably return a structure with data regarding the hit?
			return info;
		} // raycast

		KinematicCharacter* create_character_controller(const glm::vec3& spawnLocation, bool addActionToWorld)
		{
			return nullptr;
//			btTransform tr;
//			tr.setIdentity();
//			
//			btPairCachingGhostObject * ghost = new btPairCachingGhostObject();
//			ghost->setWorldTransform( tr );
//			
//			btCapsuleShape* capsule_shape = new btCapsuleShape(PHYSICS_PLAYER_HALF_WIDTH, (PHYSICS_PLAYER_HALF_HEIGHT*2)-(PHYSICS_PLAYER_HALF_WIDTH*2));
//			//btConvexShape * playerShape = new btCylinderShape( btVector3( .6, .90, .6 ) );
//
//			
//			ghost->setCollisionShape( capsule_shape );
//			ghost->setCollisionFlags( btCollisionObject::CF_CHARACTER_OBJECT );
//			
//			btScalar stepHeight = btScalar(.36);
//			KinematicCharacter * character = new KinematicCharacter (ghost,capsule_shape,stepHeight);
//			
//			///only collide with static for now (no interaction with dynamic objects)
//			dynamics_world->addCollisionObject(ghost, btBroadphaseProxy::CharacterFilter, btBroadphaseProxy::StaticFilter | btBroadphaseProxy::DefaultFilter | btBroadphaseProxy::SensorTrigger);
//			
//			if ( addActionToWorld )
//			{
//				dynamics_world->addAction(character);
//			}
//			
//			ghost->setContactProcessingThreshold( 0. );
//			//m_ghost->setCcdMotionThreshold( 1 );
//			ghost->setCcdMotionThreshold( .1 );
//			ghost->setCcdSweptSphereRadius( 0.1 );
//			
//			character->SetSpawnLocation( btVector3(spawnLocation.x, spawnLocation.y, spawnLocation.z) );
//			character->clear_state();
//			
//			_controller = character;
//					
//			return character;
		} // create_character_controller
		
		KinematicCharacter* get_character_controller(int index)
		{
			// not handling more than one at the moment
			assert(index == 0);
			
			return _controller;
		}
		
		CollisionObject* create_character_proxy(KinematicCharacter* controller)
		{
//			btGhostObject* ghost = controller->getGhostObject();
//			
//			CharacterProxyObject* proxy = CREATE(CharacterProxyObject, controller);
//			ghost->setUserPointer(proxy);
//			
//			return proxy;
			return nullptr;
		}


		CollisionObject* create_physics_for_mesh(assets::Mesh* mesh, float mass_kg, PhysicsMotionInterface* motion)
		{
#if 0
			bool use_quantized_bvh_tree = true;


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
			
			CollisionObject* object = 0;
			BulletStaticBody* static_body = 0;
			BulletRigidBody* rb = 0;

			bool dynamic_body = (mass != 0.0f);
			if (!dynamic_body)
			{
				static_body = CREATE(BulletStaticBody);
				object = static_body;
			}
			else
			{
				rb = CREATE(BulletRigidBody);
				object = rb;
			}
			
			for( uint32_t i = 0; i < mesh->geometry.size(); ++i )
			{
				assets::Geometry* geo = &mesh->geometry[ i ];
				
				FixedArray<glm::vec3>& vertices = geo->vertices;
				
				btRigidBody* body = 0;
				
				// The rigid body world transform is the center of mass. This is at the origin.
				btTransform xf;
				xf.setIdentity();
				CustomMotionState * motion_state = new CustomMotionState(xf, motion, mesh->mass_center_offset);
				
				btCollisionShape* shape = 0;
				// NOTE: Triangle shapes can ONLY be static objects.
				// TODO: look into an alternative with btGImpactMeshShape or
				// btCompoundShape + convex decomposition.
				// Could also use btConvexHullShape.
				if (dynamic_body)
				{
					shape = new btBoxShape(btVector3(0.5f, 0.5f, 0.5f));
					rb->set_collision_shape(shape);
					
					// calculate local intertia
					shape->calculateLocalInertia(mass, local_inertia);
					
					btCompoundShape* compound = new btCompoundShape();
					btTransform local_transform;
					local_transform.setIdentity();
					compound->addChildShape(local_transform, shape);
					
					btRigidBody::btRigidBodyConstructionInfo rbInfo( mass, motion_state, compound, local_inertia );
					body = new btRigidBody( rbInfo );
					
					rb->set_collision_object(body);
					body->setUserPointer(rb);
					
					body->setRestitution(0.25f);
					body->setFriction(0.5f);
					body->setCcdMotionThreshold(0.1f);
					body->setCcdSweptSphereRadius(0.1f);
				}
				else
				{
					// specify verts/indices from our meshdef
					// NOTE: This does NOT make a copy of the data. Whatever you pass it
					// must persist for the life of the shape.
					btTriangleIndexVertexArray * mesh = new btTriangleIndexVertexArray(geo->index_count/3, (int*)&geo->indices[0], sizeof(int)*3, geo->vertex_count, (btScalar*)&vertices[0], sizeof(glm::vec3));
				
					// use that to create a Bvh triangle mesh shape
					btBvhTriangleMeshShape * trishape = new btBvhTriangleMeshShape( mesh, use_quantized_bvh_tree );
					static_body->add_shape(trishape);
					
					btTransform local_transform;
					local_transform.setIdentity();
					btRigidBody::btRigidBodyConstructionInfo rigid_body_info(0.0f, motion_state, trishape, local_inertia );
					body = new btRigidBody(rigid_body_info);
					
					static_body->set_collision_object(body);
					body->setUserPointer(static_body);
					
					int body_flags = body->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT;
					body->setCollisionFlags( body_flags );
					body->setFriction(0.75f);
				}

				dynamics_world->addRigidBody(body);
			}
			
			return object;
#endif
			return nullptr;
		} // create_physics_for_mesh

		CollisionObject* create_trigger(const glm::vec3& size)
		{
#if 0
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
#endif
			return nullptr;
		} // create_trigger

	} // namespace physics
} // namespace gemini