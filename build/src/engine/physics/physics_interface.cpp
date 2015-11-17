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

#include "physics_interface.h"

#include "BulletCollision/CollisionDispatch/btGhostObject.h"

#include "bullet/bullet_common.h"
#include "bullet/bullet_rigidbody.h"
#include "bullet/bullet_staticbody.h"
#include "bullet/bullet_motionstate.h"

#include "assets/asset_mesh.h"

#include "physics_common.h"

#include "kernel.h"

#include <sdk/engine_api.h>
#include <sdk/model_api.h>
#include <sdk/physics_api.h>

#include <core/typedefs.h>

using namespace gemini;
using namespace gemini::physics::bullet;
using gemini::physics::RaycastInfo;

using namespace core;

namespace gemini
{
	namespace physics
	{
		PhysicsInterface::~PhysicsInterface()
		{
			// purge collision shapes
			for (auto& shape : collision_shapes)
			{
				MEMORY_DELETE(shape, core::memory::global_allocator());
			}
		}

		physics::ICollisionObject* PhysicsInterface::create_physics_object(ICollisionShape* shape, const glm::vec3& position, const glm::quat& orientation, ObjectProperties& properties)
		{
			BulletRigidBody* rigidbody = MEMORY_NEW(BulletRigidBody, core::memory::global_allocator());

			btScalar mass(properties.mass_kg);
			btVector3 local_inertia(0.0f, 0.0f, 0.0f);
			CustomMotionState* motion_state = new CustomMotionState(position, orientation);

			btCollisionShape* bullet_shape = ((BulletCollisionShape*)shape)->get_shape();

			rigidbody->set_motion_state(motion_state);

			// calculate local intertia
			bullet_shape->calculateLocalInertia(mass, local_inertia);

			btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motion_state, bullet_shape, local_inertia);
			btRigidBody* body = new btRigidBody(rbInfo);
			body->setUserPointer(rigidbody);
			body->setRestitution(properties.restitution);
			body->setFriction(properties.friction);
			body->setCcdMotionThreshold(0.01f);
			body->setCcdSweptSphereRadius(0.01f);

			btGhostObject* ghost = new btPairCachingGhostObject();
			ghost->setCollisionShape(bullet_shape);
			ghost->setUserPointer(rigidbody);
			ghost->setCollisionFlags(ghost->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);

			motion_state->set_body_and_ghost(body, ghost);

			rigidbody->set_collision_object(body);
			rigidbody->set_collision_shape(bullet_shape);
			rigidbody->set_collision_ghost(ghost);

			bullet::get_world()->addRigidBody(body);
			bullet::get_world()->addCollisionObject(ghost, btBroadphaseProxy::SensorTrigger, btBroadphaseProxy::CharacterFilter);

			return rigidbody;
		}

		btCompoundShape* compound_shape_from_geometry(assets::Mesh* mesh, bool is_dynamic, float mass)
		{
			bool use_quantized_bvh_tree = true;

			btCompoundShape* compound = new btCompoundShape();

			for( uint32_t index = 0; index < mesh->geometry.size(); ++index )
			{
				assets::Geometry* geo = &mesh->geometry[index];
				FixedArray<glm::vec3>& vertices = geo->vertices;

				// this shape's transform
				btTransform local_transform;
				local_transform.setIdentity();

				// setup shape for a dynamic object
				if (is_dynamic)
				{
					// create a box for now. this needs to be replaced
					btCollisionShape* shape = new btBoxShape(btVector3(0.5f, 0.5f, 0.5f));
					btVector3 local_inertia(0, 0, 0);

					// calculate local intertia
					shape->calculateLocalInertia(mass, local_inertia);
					compound->addChildShape(local_transform, shape);
				}
				else
				{
					// NOTE: Triangle shapes can ONLY be static objects.
					// TODO: look into an alternative with btGImpactMeshShape or
					// btCompoundShape + convex decomposition.
					// Could also use btConvexHullShape.

					// specify verts/indices from our meshdef
					// NOTE: This does NOT make a copy of the data. Whatever you pass it
					// must persist for the life of the shape.
					btTriangleIndexVertexArray* triangle_vertex_array = new btTriangleIndexVertexArray(
						geo->index_count/3,
						(int*)&geo->indices[0],
						sizeof(int)*3, geo->vertex_count,
						(btScalar*)&vertices[0],
						sizeof(glm::vec3)
					);

					// use that to create a Bvh triangle mesh shape
					// TODO: use a btConvexTriangleMeshShape ?
					btBvhTriangleMeshShape* triangle_mesh = new btBvhTriangleMeshShape(triangle_vertex_array, use_quantized_bvh_tree);
					compound->addChildShape(local_transform, triangle_mesh);
				}
			}

			return compound;
		}

		physics::ICollisionObject* PhysicsInterface::create_physics_model(int32_t model_index, ObjectProperties& properties)
		{
			// Passed an invalid model_index. Perhaps the model wasn't loaded?
			assert(model_index != -1);

			btScalar mass(properties.mass_kg);
			btVector3 local_inertia(0, 0, 0);


			IModelInstanceData* model_interface = engine::instance()->models()->get_instance_data(model_index);
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

			ICollisionObject* object = 0;
			BulletStaticBody* static_body = 0;
			BulletRigidBody* rb = 0;

			bool dynamic_body = (mass != 0.0f);
			if (!dynamic_body)
			{
				static_body = MEMORY_NEW(BulletStaticBody, core::memory::global_allocator());
				object = static_body;
			}
			else
			{
				rb = MEMORY_NEW(BulletRigidBody, core::memory::global_allocator());
				object = rb;
			}


			// create a compound shape and add geometries to it

			btCompoundShape* compound = compound_shape_from_geometry(mesh, dynamic_body, mass);
			btRigidBody* body = 0;

			// The rigid body world transform is the center of mass. This is at the origin.
			btTransform xf;
			xf.setIdentity();

			btMotionState* motion_state = new btDefaultMotionState(xf);

			btRigidBody::btRigidBodyConstructionInfo rigid_body_info(mass, motion_state, compound, local_inertia);
			body = new btRigidBody(rigid_body_info);

			btPairCachingGhostObject* ghost = new btPairCachingGhostObject();
			ghost->setUserPointer(object);
			ghost->setCollisionShape(compound);
			ghost->setCollisionFlags(btCollisionObject::CF_NO_CONTACT_RESPONSE);
			bullet::get_world()->addCollisionObject(ghost, btBroadphaseProxy::SensorTrigger, btBroadphaseProxy::CharacterFilter);

			// now setup the body using the compound shape
			if (dynamic_body)
			{
				rb->set_collision_ghost(ghost);
				rb->set_collision_object(body);
				rb->set_collision_shape(compound);
				body->setUserPointer(rb);

				body->setRestitution(properties.restitution);
				body->setFriction(properties.friction);
				body->setCcdMotionThreshold(0.1f);
				body->setCcdSweptSphereRadius(0.1f);
			}
			else
			{
				static_body->set_collision_ghost(ghost);
				static_body->set_collision_object(body);
				static_body->add_shape(compound);

				body->setUserPointer(static_body);

				int body_flags = body->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT;
				body->setCollisionFlags( body_flags );
				body->setFriction(0.75f);
			}



			bullet::get_world()->addRigidBody(body);

			return object;
		} // create_physics_model

		physics::ICollisionObject* PhysicsInterface::create_character_object(ICollisionShape* shape)
		{
			BulletCollisionObject* collision_object = MEMORY_NEW(BulletCollisionObject, core::memory::global_allocator());

			btPairCachingGhostObject* ghost = new btPairCachingGhostObject();
			ghost->setUserPointer(collision_object);

//			LOGV("TODO: set character object transform!\n");

			BulletCollisionShape* bullet_shape = static_cast<BulletCollisionShape*>(shape);
			assert(bullet_shape != 0);

			collision_object->set_collision_object(ghost);
//			collision_object->set_collision_ghost(ghost);
			collision_object->set_collision_shape(bullet_shape->get_shape());

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

		physics::ICollisionObject* PhysicsInterface::create_trigger_object(ICollisionShape* shape, const glm::vec3& position, const glm::quat& orientation)
		{
			BulletCollisionObject* collision_object = MEMORY_NEW(BulletCollisionObject, core::memory::global_allocator());
			btPairCachingGhostObject* ghost = new btPairCachingGhostObject();
			ghost->setUserPointer(collision_object);
			collision_object->set_collision_object(ghost);

			BulletCollisionShape* bullet_shape = static_cast<BulletCollisionShape*>(shape);
			assert(bullet_shape != 0);
			collision_object->set_collision_shape(bullet_shape->get_shape());

			int flags = ghost->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE;
			ghost->setCollisionShape(bullet_shape->get_shape());
			ghost->setCollisionFlags(flags);

			btTransform tr;
			tr.setIdentity();
			tr.setOrigin(btVector3(position.x, position.y, position.z));
			ghost->setWorldTransform(tr);

			// for now, triggers can only interact with character objects.
			// There was a fix in September of 2013 which fixed sensors and characters, see: https://code.google.com/p/bullet/issues/detail?id=719
			bullet::get_world()->addCollisionObject(ghost, btBroadphaseProxy::SensorTrigger, btBroadphaseProxy::CharacterFilter);

			return collision_object;
		}

		physics::ICollisionShape* PhysicsInterface::create_capsule(float radius_meters, float height_meters)
		{
			assert(radius_meters >= FLT_EPSILON);
			assert(height_meters >= FLT_EPSILON);

			BulletCollisionShape* collision_shape = MEMORY_NEW(BulletCollisionShape, core::memory::global_allocator());
			collision_shapes.push_back(collision_shape);

			btCollisionShape* capsule = new btCapsuleShape(radius_meters, height_meters);
			collision_shape->set_shape(capsule);
			return collision_shape;
		}


		physics::ICollisionShape* PhysicsInterface::create_box(const glm::vec3& dimensions)
		{
			assert(dimensions.x >= FLT_EPSILON);
			assert(dimensions.y >= FLT_EPSILON);
			assert(dimensions.z >= FLT_EPSILON);

			BulletCollisionShape* collision_shape = MEMORY_NEW(BulletCollisionShape, core::memory::global_allocator());
			collision_shapes.push_back(collision_shape);

			btVector3 half_extents(dimensions.x*0.5f, dimensions.y*0.5f, dimensions.z*0.5f);

//			btCompoundShape* compound = new btCompoundShape();
			btCollisionShape* shape = new btBoxShape(half_extents);
			collision_shape->set_shape(shape);

//			btTransform local_transform;
//			local_transform.setIdentity();
//			local_transform.setOrigin(btVector3(-0.0f, -0.0f, -0.0f));
//			compound->addChildShape(local_transform, box);
//			collision_shape->set_shape(compound);

			return collision_shape;
		}

		physics::ICollisionShape* PhysicsInterface::create_cylinder(float radius_meters, float height_meters)
		{
			BulletCollisionShape* collision_shape = MEMORY_NEW(BulletCollisionShape, core::memory::global_allocator());
			collision_shapes.push_back(collision_shape);
			btVector3 half_extents(radius_meters*0.5f, height_meters*0.5f, radius_meters*0.5f);
			btCollisionShape* shape = new btCylinderShape(half_extents);
			collision_shape->set_shape(shape);
			return collision_shape;
		}

		void PhysicsInterface::destroy_object(ICollisionObject* object)
		{
			MEMORY_DELETE(object, core::memory::global_allocator());
		} // destroy_object

		void PhysicsInterface::step_simulation(float framedelta_seconds)
		{
			bullet::step(framedelta_seconds, kernel::parameters().step_interval_seconds);
		}

		RaycastInfo PhysicsInterface::raycast(ICollisionObject* ignored_object, const glm::vec3& start, const glm::vec3& direction, float max_distance)
		{
			glm::vec3 destination = start + (direction * max_distance);
			btVector3 ray_start(start.x, start.y, start.z);
			btVector3 ray_end(destination.x, destination.y, destination.z);

			BulletCollisionObject* bullet_object = static_cast<BulletCollisionObject*>(ignored_object);
			btCollisionObject* obj = bullet_object->get_collision_object();

			ClosestNotMeRayResultCallback callback(obj, ray_start, ray_end);

			// rayTest accepts a line segment from start to end
			bullet::get_world()->rayTest(ray_start, ray_end, callback);

			RaycastInfo info;
			if (callback.hasHit())
			{
				const btVector3& hit_point_world = callback.m_hitPointWorld;
				info.hit = glm::vec3(hit_point_world.x(), hit_point_world.y(), hit_point_world.z());
				info.object = static_cast<ICollisionObject*>(callback.m_collisionObject->getUserPointer());

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
			}

			return info;
		}


		SweepTestResult PhysicsInterface::sweep(ICollisionObject* source_object, ICollisionShape* shape_object, const glm::vec3& start, const glm::vec3& end, float angle_threshold)
		{
			SweepTestResult result;

			BulletCollisionObject* bullet_object = static_cast<BulletCollisionObject*>(source_object);
			btGhostObject* ghost = (btGhostObject*)bullet_object->get_collision_object();
			assert(ghost != nullptr);

			BulletCollisionShape* bullet_shape = static_cast<BulletCollisionShape*>(shape_object);
			btCollisionShape* shape = bullet_shape->get_shape();
			btConvexShape* convex_shape = static_cast<btConvexShape*>(shape);
			assert(convex_shape != nullptr);

			btTransform source;
			btTransform destination;
			source.setIdentity();
			destination.setIdentity();
			source.setOrigin(fromglm(start));
			destination.setOrigin(fromglm(end));

			ClosestNotMeConvexResultCallback callback(ghost, btVector3(0, 1.0f, 0), angle_threshold);
			callback.m_collisionFilterGroup = ghost->getBroadphaseHandle()->m_collisionFilterGroup;
			callback.m_collisionFilterMask = ghost->getBroadphaseHandle()->m_collisionFilterMask;

			ghost->convexSweepTest(convex_shape, source, destination, callback, bullet::get_world()->getDispatchInfo().m_allowedCcdPenetration);

//			if (callback.hasHit())
//			{
//				btVector3 normal = callback.m_hitNormalWorld.normalize();
//				//				debugdraw::sphere(toglm(callback.m_hitPointWorld), Color(255, 0, 0), 0.005f, 2.0f);
//				//				debugdraw::line(toglm(callback.m_hitPointWorld), toglm(callback.m_hitPointWorld+normal*0.1f), Color(0, 255, 255), 2.0f);
//			}

//			return callback.hasHit();

			if (callback.hasHit())
			{
				result.hit_count = 1;
				result.closest_hit_fraction = callback.m_closestHitFraction;
				result.hit_normal_world = toglm(callback.m_hitNormalWorld);
				result.hit_point_world = toglm(callback.m_hitPointWorld);

				BulletCollisionObject* collision_object = static_cast<BulletCollisionObject*>(callback.m_hitCollisionObject->getUserPointer());
				result.hit_object = collision_object;
			}


			return result;
		}
	} // namespace physics
} // namespace gemini
