// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone

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

#include <sdk/physics_api.h>
#include "physics_interface.h"


#include <btBulletDynamicsCommon.h>
#include "BulletCollision/CollisionDispatch/btGhostObject.h"

#include "bullet/bullet_common.h"
#include "bullet/bullet_rigidbody.h"
#include "bullet/bullet_motionstate.h"

#include "assets/asset_mesh.h"


#include <sdk/engine_api.h>
#include <sdk/model_api.h>

using namespace gemini;
using namespace gemini::physics::bullet;

namespace gemini
{
	namespace physics
	{

		PhysicsInterface::~PhysicsInterface()
		{
			// purge collision shapes
			for (auto& shape : collision_shapes)
			{
				DESTROY(CollisionShape, shape);
			}
		}

		physics::CollisionObject* PhysicsInterface::create_physics_model(
				int32_t model_index,
				ObjectProperties& properties
			)
		{
			bool use_quantized_bvh_tree = true;
			
			
			btScalar mass(properties.mass_kg);
			btVector3 local_inertia(0, 0, 0);
			
			
			IModelInstanceData* model_interface = engine::api::instance()->models()->get_instance_data(model_index);
			assets::Mesh* mesh = assets::meshes()->find_with_id(model_interface->asset_index());
			if (!mesh)
			{
				LOGW("Unable to create physics for null mesh\n");
				return nullptr;
			}
			
			if (!bullet::get_world())
			{
				LOGE("Unable to add physics for mesh; invalid physics state\n");
				return nullptr;
			}
			
			CollisionObject* object = 0;
			BulletStaticBody* static_body = 0;
			BulletRigidBody* rb = 0;
			
			bool dynamic_body = (mass != 0.0f);
			
			LOGV("dynamic body: %s\n", dynamic_body ? "Yes" : "No");
			
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

				const glm::vec3& mass_center_offset = mesh->mass_center_offset;
//				xf.setOrigin(btVector3(mass_center_offset.x, mass_center_offset.y, mass_center_offset.z));
				btMotionState* motion_state = new btDefaultMotionState(xf);
//				CustomMotionState * motion_state = new CustomMotionState(xf, motion_interface, mesh->mass_center_offset);
				btCollisionShape* shape = 0;
				// NOTE: Triangle shapes can ONLY be static objects.
				// TODO: look into an alternative with btGImpactMeshShape or
				// btCompoundShape + convex decomposition.
				// Could also use btConvexHullShape.
				
				btTransform local_transform;
				local_transform.setIdentity();
//				local_transform.setOrigin(btVector3(-mass_center_offset.x, -mass_center_offset.y, -mass_center_offset.z));
				
				if (dynamic_body)
				{
					shape = new btBoxShape(btVector3(0.5f, 0.5f, 0.5f));
					
					// calculate local intertia
					shape->calculateLocalInertia(mass, local_inertia);
					
					btCompoundShape* compound = new btCompoundShape();

					compound->addChildShape(local_transform, shape);

					btRigidBody::btRigidBodyConstructionInfo rbInfo( mass, motion_state, compound, local_inertia );
					body = new btRigidBody( rbInfo );
					
					rb->set_collision_object(body);
					rb->set_collision_shape(compound);
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
					
					btRigidBody::btRigidBodyConstructionInfo rigid_body_info(0.0f, motion_state, trishape, local_inertia );
					body = new btRigidBody(rigid_body_info);
					static_body->set_collision_object(body);
					static_body->add_shape(trishape);
					
					body->setUserPointer(static_body);
					
					int body_flags = body->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT;
					body->setCollisionFlags( body_flags );
					body->setFriction(0.75f);
				}
				
				bullet:get_world()->addRigidBody(body);
			}
			
			return object;
		} // create_physics_model
		
		physics::CollisionObject* PhysicsInterface::create_character_object(CollisionShape* shape)
		{
			BulletCollisionObject* collision_object = CREATE(BulletCollisionObject);
			
			btPairCachingGhostObject* ghost = new btPairCachingGhostObject();
			
			LOGV("TODO: set character object transform!\n");
			
			BulletCollisionShape* bullet_shape = static_cast<BulletCollisionShape*>(shape);
			assert(bullet_shape != 0);
			
			short collision_filter_mask = btBroadphaseProxy::StaticFilter | btBroadphaseProxy::DefaultFilter | btBroadphaseProxy::SensorTrigger;
			
			// NOTE: you must set the collision shape before adding it to the world
			ghost->setCollisionShape(bullet_shape->get_shape());
			ghost->setCollisionFlags(btCollisionObject::CF_CHARACTER_OBJECT);
			ghost->setContactProcessingThreshold(0.0f);
			ghost->setCcdMotionThreshold(0.1f);
			ghost->setCcdSweptSphereRadius(0.1f);
		
			bullet::get_world()->addCollisionObject(ghost, btBroadphaseProxy::CharacterFilter, collision_filter_mask);
		
			return collision_object;
		}
		
		physics::CollisionShape* PhysicsInterface::create_capsule(float radius_meters, float height_meters)
		{
			BulletCollisionShape* collision_shape = CREATE(BulletCollisionShape);
			collision_shapes.push_back(collision_shape);
						
			btCollisionShape* capsule = new btCapsuleShape(radius_meters, height_meters);
			collision_shape->set_shape(capsule);
			return collision_shape;
		}
		
		
		void PhysicsInterface::destroy_object(CollisionObject* object)
		{
			DESTROY(CollisionObject, object);
		} // destroy_object


		IPlayerController* PhysicsInterface::create_player_controller(CollisionObject* object)
		{
			return 0;
		}
		
		void PhysicsInterface::destroy_player_controller(IPlayerController* object)
		{
			
		}
		
		void PhysicsInterface::step_simulation(float delta_seconds)
		{
			bullet::step(delta_seconds);
		}
		
	} // namespace physics
} // namespace gemini