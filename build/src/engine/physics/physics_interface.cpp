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

//#include "assets/asset_mesh.h"

#include "physics_common.h"

#include "kernel.h"

#include <renderer/debug_draw.h>

#include <runtime/assets.h>
#include <runtime/mesh.h>

#include <sdk/engine_api.h>
#include <sdk/model_api.h>
#include <sdk/physics_api.h>

#include <core/typedefs.h>
#include <renderer/color.h>

using namespace gemini;
using namespace gemini::physics::bullet;
using gemini::physics::RaycastInfo;

using namespace core;

namespace gemini
{
	namespace physics
	{
		short physics_to_bullet_filter_mask(int collision_mask)
		{
			short collision_filter_mask = 0;

			if (collision_mask & physics::DefaultFilter)
				collision_filter_mask |= btBroadphaseProxy::DefaultFilter;
			if (collision_mask & physics::StaticFilter)
				collision_filter_mask |= btBroadphaseProxy::StaticFilter;
			if (collision_mask & physics::KinematicFilter)
				collision_filter_mask |= btBroadphaseProxy::KinematicFilter;
			if (collision_mask & physics::DebrisFilter)
				collision_filter_mask |= btBroadphaseProxy::DebrisFilter;
			if (collision_mask & physics::SensorTrigger)
				collision_filter_mask |= btBroadphaseProxy::SensorTrigger;
			if (collision_mask & physics::CharacterFilter)
				collision_filter_mask |= btBroadphaseProxy::CharacterFilter;

			return collision_filter_mask;
		}

		PhysicsInterface::PhysicsInterface(gemini::Allocator& _allocator)
			: allocator(_allocator)
		{
		}

		PhysicsInterface::~PhysicsInterface()
		{
			// purge collision shapes
			for (auto& shape : collision_shapes)
			{
				MEMORY2_DELETE(allocator, shape);
			}
		}

		physics::ICollisionObject* PhysicsInterface::create_physics_object(ICollisionShape* shape, const glm::vec3& position, const glm::quat& orientation, ObjectProperties& properties)
		{
			BulletRigidBody* rigidbody = MEMORY2_NEW(allocator, BulletRigidBody);

			btScalar mass(properties.mass_kg);
			btVector3 local_inertia(0.0f, 0.0f, 0.0f);
			CustomMotionState* motion_state = new CustomMotionState(position, orientation);

			btCollisionShape* bullet_shape = ((BulletCollisionShape*)shape)->get_shape();

			rigidbody->set_motion_state(motion_state);

			// calculate local inertia
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

		btCompoundShape* compound_shape_from_geometry(Mesh* mesh, bool is_dynamic, float mass)
		{
			bool use_quantized_bvh_tree = true;

			btCompoundShape* compound = new btCompoundShape();

			// this shape's transform
			btTransform local_transform;
			local_transform.setIdentity();

			// setup shape for a dynamic object
			if (is_dynamic)
			{
				// create a box for now. this needs to be replaced
				btCollisionShape* shape = new btBoxShape(btVector3(0.5f, 0.5f, 0.5f));
				btVector3 local_inertia(0, 0, 0);

				// calculate local inertia
				shape->calculateLocalInertia(mass, local_inertia);
				compound->addChildShape(local_transform, shape);
			}
			else
			{
				// Use the collision geometry from the mesh.
				assert(mesh->collision_geometry);
				assert(mesh->collision_geometry->vertices);
				assert(mesh->collision_geometry->normals);
				assert(mesh->collision_geometry->indices);

				glm::vec3* vertices = mesh->collision_geometry->vertices;
				// NOTE: Triangle shapes can ONLY be static objects.
				// TODO: look into an alternative with btGImpactMeshShape or
				// btCompoundShape + convex decomposition.
				// Could also use btConvexHullShape.

				// NOTE: This does NOT make a copy of the data. Whatever you pass it
				// must persist for the life of the shape.
				void* indices = mesh->collision_geometry->indices;

				static_assert(sizeof(uint32_t) == sizeof(index_t), "physics assumes index_t is 32-bit");

				btTriangleIndexVertexArray* triangle_vertex_array = new btTriangleIndexVertexArray(
					mesh->collision_geometry->total_indices/3,
					(int*)indices,
					sizeof(index_t) * 3,
					mesh->collision_geometry->total_vertices,
					(btScalar*)vertices,
					sizeof(glm::vec3)
				);

				// when loading uint16_ts, must specify the index type.
				if (sizeof(index_t) == 2)
				{
					triangle_vertex_array->getIndexedMeshArray()[0].m_indexType = PHY_SHORT;
				}

				// use that to create a Bvh triangle mesh shape
				// TODO: use a btConvexTriangleMeshShape ?
				btBvhTriangleMeshShape* triangle_mesh = new btBvhTriangleMeshShape(triangle_vertex_array, use_quantized_bvh_tree);
				compound->addChildShape(local_transform, triangle_mesh);
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
			gemini::Mesh* mesh = gemini::mesh_from_handle(model_interface->asset_index());
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
				static_body = MEMORY2_NEW(allocator, BulletStaticBody);
				object = static_body;
			}
			else
			{
				rb = MEMORY2_NEW(allocator, BulletRigidBody);
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
			BulletCollisionObject* collision_object = MEMORY2_NEW(allocator, BulletCollisionObject);

			btPairCachingGhostObject* ghost = new btPairCachingGhostObject();
			ghost->setUserPointer(collision_object);

//			LOGV("TODO: set character object transform!\n");

			BulletCollisionShape* bullet_shape = static_cast<BulletCollisionShape*>(shape);
			assert(bullet_shape != 0);

			collision_object->set_collision_object(ghost);
//			collision_object->set_collision_ghost(ghost);
			collision_object->set_collision_shape(bullet_shape->get_shape());

			short collision_filter_mask = btBroadphaseProxy::StaticFilter | btBroadphaseProxy::DefaultFilter | btBroadphaseProxy::SensorTrigger | btBroadphaseProxy::CharacterFilter;

			// NOTE: you must set the collision shape before adding it to the world
			ghost->setCollisionShape(bullet_shape->get_shape());
			ghost->setCollisionFlags(btCollisionObject::CF_CHARACTER_OBJECT);
			ghost->setContactProcessingThreshold(0.0f);
			ghost->setCcdMotionThreshold(0.1f);
			ghost->setCcdSweptSphereRadius(0.1f);

			bullet::get_world()->addCollisionObject(ghost, btBroadphaseProxy::CharacterFilter, collision_filter_mask);

			return collision_object;
		}

		physics::ICollisionObject* PhysicsInterface::create_trigger_object(ICollisionShape* shape, const glm::vec3& position, const glm::quat& /*orientation*/, uint16_t collision_mask)
		{
			BulletCollisionObject* collision_object = MEMORY2_NEW(allocator, BulletCollisionObject);
			btPairCachingGhostObject* ghost = new btPairCachingGhostObject();
			ghost->setUserPointer(collision_object);
			collision_object->set_collision_object(ghost);

			BulletCollisionShape* bullet_shape = static_cast<BulletCollisionShape*>(shape);
			assert(bullet_shape != 0);
			collision_object->set_collision_shape(bullet_shape->get_shape());

			int flags = ghost->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE | btCollisionObject::CF_KINEMATIC_OBJECT;
			ghost->setCollisionShape(bullet_shape->get_shape());
			ghost->setCollisionFlags(flags);

			btTransform tr;
			tr.setIdentity();
			tr.setOrigin(btVector3(position.x, position.y, position.z));
			ghost->setWorldTransform(tr);

			// for now, triggers can only interact with character objects.
			// There was a fix in September of 2013 which fixed sensors and characters, see: https://code.google.com/p/bullet/issues/detail?id=719
			assert(collision_mask == physics::SensorTrigger);
			bullet::get_world()->addCollisionObject(ghost, btBroadphaseProxy::SensorTrigger, btBroadphaseProxy::CharacterFilter);

			return collision_object;
		}

		physics::ICollisionObject* PhysicsInterface::create_kinematic_object(gemini::physics::ICollisionShape* shape, const glm::vec3& position, const glm::quat& /*orientation*/, uint16_t collision_mask)
		{
			BulletCollisionObject* collision_object = MEMORY2_NEW(allocator, BulletCollisionObject);

			btPairCachingGhostObject* ghost = new btPairCachingGhostObject();
			ghost->setUserPointer(collision_object);
			collision_object->set_collision_object(ghost);

			BulletCollisionShape* bullet_shape = static_cast<BulletCollisionShape*>(shape);
			assert(bullet_shape != 0);
			collision_object->set_collision_shape(bullet_shape->get_shape());
			bullet_shape->get_shape()->setUserPointer(shape);

			int flags = ghost->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE;
			ghost->setCollisionShape(bullet_shape->get_shape());
			ghost->setCollisionFlags(flags);

			btTransform tr;
			tr.setIdentity();
			tr.setOrigin(btVector3(position.x, position.y, position.z));
			ghost->setWorldTransform(tr);

			short collision_filter_mask = physics_to_bullet_filter_mask(collision_mask);

			bullet::get_world()->addCollisionObject(ghost, btBroadphaseProxy::KinematicFilter | btBroadphaseProxy::SensorTrigger, collision_filter_mask);

			return collision_object;
		}

		physics::ICollisionShape* PhysicsInterface::create_capsule(float radius_meters, float height_meters)
		{
			assert(radius_meters >= FLT_EPSILON);
			assert(height_meters >= FLT_EPSILON);

			BulletCollisionShape* collision_shape = MEMORY2_NEW(allocator, BulletCollisionShape);
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

			BulletCollisionShape* collision_shape = MEMORY2_NEW(allocator, BulletCollisionShape);
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
			BulletCollisionShape* collision_shape = MEMORY2_NEW(allocator, BulletCollisionShape);
			collision_shapes.push_back(collision_shape);
			btVector3 half_extents(radius_meters*0.5f, height_meters*0.5f, radius_meters*0.5f);
			btCollisionShape* shape = new btCylinderShape(half_extents);
			collision_shape->set_shape(shape);
			return collision_shape;
		}

		physics::ICollisionShape* PhysicsInterface::create_sphere(float radius_meters)
		{
			BulletCollisionShape* collision_shape = MEMORY2_NEW(allocator, BulletCollisionShape);
			collision_shapes.push_back(collision_shape);
			btCollisionShape* sphere = new btSphereShape(radius_meters);
			collision_shape->set_shape(sphere);
			return collision_shape;
		}

		physics::ICollisionShape* PhysicsInterface::create_convex_shape(const glm::vec3* vertices, size_t total_vertices)
		{
			BulletCollisionShape* collision_shape = MEMORY2_NEW(allocator, BulletCollisionShape);
			collision_shapes.push_back(collision_shape);
			btConvexHullShape* shape = new btConvexHullShape((const btScalar*)vertices, total_vertices, sizeof(glm::vec3));
			collision_shape->set_shape(shape);
			return collision_shape;
		}

		void PhysicsInterface::destroy_object(ICollisionObject* object)
		{
			MEMORY2_DELETE(allocator, object);
		} // destroy_object

		void PhysicsInterface::step_simulation(float step_interval_seconds)
		{
			bullet::step(step_interval_seconds);
		}

		RaycastInfo PhysicsInterface::raycast(const RaycastQuery& query)
		{
			glm::vec3 destination = query.start + (query.direction * query.max_distance);
			btVector3 ray_start(query.start.x, query.start.y, query.start.z);
			btVector3 ray_end(destination.x, destination.y, destination.z);

			//// Ignored object must be valid!
			//assert(ignored_object);


			btCollisionObject* obj0 = nullptr;
			btCollisionObject* obj1 = nullptr;

			if (query.ignored_object0)
			{
				BulletCollisionObject* bullet_object = static_cast<BulletCollisionObject*>(query.ignored_object0);
				obj0 = bullet_object->get_collision_object();
			}

			if (query.ignored_object1)
			{
				BulletCollisionObject* bullet_object = static_cast<BulletCollisionObject*>(query.ignored_object1);
				obj1 = bullet_object->get_collision_object();
			}

			ClosestNotMeRayResultCallback callback(ray_start, ray_end, false, false, obj0, obj1);

			callback.m_collisionFilterMask = physics_to_bullet_filter_mask(query.collision_flags);

			// rayTest accepts a line segment from start to end
			bullet::get_world()->rayTest(ray_start, ray_end, callback);

			RaycastInfo info;
			if (callback.hasHit())
			{
				const btVector3& hit_point_world = callback.m_hitPointWorld;
				info.hit = glm::vec3(hit_point_world.x(), hit_point_world.y(), hit_point_world.z());
				info.object = static_cast<ICollisionObject*>(callback.m_collisionObject->getUserPointer());

				const btVector3& hit_normal_world = callback.m_hitNormalWorld;
				info.hit_normal = glm::vec3(hit_normal_world.x(), hit_normal_world.y(), hit_normal_world.z());

				info.closest_hit_fraction = callback.m_closestHitFraction;
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


		SweepTestResult PhysicsInterface::sweep(ICollisionObject* source_object, ICollisionShape* shape_object, const glm::vec3& start, const glm::vec3& end, float angle_threshold, const glm::vec3& angle)
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

			ClosestNotMeConvexResultCallback callback(ghost, fromglm(angle), angle_threshold);
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

				//btVector3 normal = callback.m_hitNormalWorld.normalize();
				//debugdraw::sphere(toglm(callback.m_hitPointWorld), gemini::Color(1.0f, 0.0f, 0.0f), 0.005f, 2.0f);
				//::renderer::debugdraw::sphere(toglm(callback.m_hitPointWorld), Color::from_rgba(255, 0, 0, 255), 0.005f, 2.0f);
				//::renderer::debugdraw::line(toglm(callback.m_hitPointWorld), toglm(callback.m_hitPointWorld+normal*0.1f), Color::from_rgba(0, 255, 255, 255), 2.0f);

				BulletCollisionObject* collision_object = static_cast<BulletCollisionObject*>(callback.m_hitCollisionObject->getUserPointer());
				result.hit_object = collision_object;
			}


			return result;
		} // sweep

		bool PhysicsInterface::update_shape_geometry(ICollisionShape* shape, const glm::vec3* vertices, size_t total_vertices)
		{
			BulletCollisionShape* bullet_shape = static_cast<BulletCollisionShape*>(shape);
			assert(bullet_shape != 0);

			// TODO: check this shape is correct! It will probably trash
			// the memory of a non convex hull shape.
			btConvexHullShape* hull_shape = static_cast<btConvexHullShape*>(bullet_shape->get_shape());
			assert(hull_shape != 0);

			hull_shape->updatePoints((const btScalar*)vertices, total_vertices, sizeof(glm::vec3));
			return true;
		} // update_shape_geometry


		uint32_t PhysicsInterface::count_overlapping_objects(ICollisionObject* object)
		{
			BulletCollisionObject* bullet_object = static_cast<BulletCollisionObject*>(object);
			assert(bullet_object);

			btGhostObject* ghost = reinterpret_cast<btGhostObject*>(bullet_object->get_collision_object());
			return ghost->getNumOverlappingObjects();
		} // count_overlapping_objects


		ICollisionObject* PhysicsInterface::overlapping_object(ICollisionObject* object, uint32_t index)
		{
			BulletCollisionObject* bullet_object = static_cast<BulletCollisionObject*>(object);
			assert(bullet_object);

			btGhostObject* ghost = reinterpret_cast<btGhostObject*>(bullet_object->get_collision_object());

			btCollisionObject* overlapping = ghost->getOverlappingObject(index);

			BulletCollisionObject* other = reinterpret_cast<BulletCollisionObject*>(overlapping->getUserPointer());
			return other;
		} // overlapping_object

	} // namespace physics
} // namespace gemini
