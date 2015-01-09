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
#include <core/typedefs.h>
#include <slim/xlog.h>

#include <sdk/physics_api.h>
#include "physics_interface.h"

#include "BulletCollision/CollisionDispatch/btGhostObject.h"

#include "bullet/bullet_common.h"
#include "bullet/bullet_rigidbody.h"
#include "bullet/bullet_staticbody.h"
#include "bullet/bullet_motionstate.h"

#include "assets/asset_mesh.h"


#include <sdk/engine_api.h>
#include <sdk/model_api.h>


#include "bullet/bullet_charactercontroller.h"

using namespace gemini;
using namespace gemini::physics::bullet;


#include "input.h"

namespace gemini
{
	namespace physics
	{
		namespace bullet
		{
			class BulletPlayerController : public IPlayerController
			{
				ICollisionObject* target;
				KinematicCharacter* character;
				
				// view angles in degrees
				glm::vec2 view_angles;
				
			private:
				void move_player(KinematicCharacter* character, const MovementCommand& command)
				{
					if (character)
					{
						glm::vec3 cam_dir, cam_right;
						mathlib::basis_vectors_from_pitch_yaw(view_angles.x, view_angles.y, cam_right, cam_dir);
						
						const float QUOTIENT = (1.0f/input::AxisValueMaximum);
						
						if (character)
						{
							character->setFacingDirections(
														   btVector3(cam_dir.x, cam_dir.y, cam_dir.z),
														   btVector3(cam_right.x, cam_right.y, cam_right.z)
														   );
							
							character->setMovementWeight(
														 command.forward * QUOTIENT,
														 command.back * QUOTIENT,
														 command.left * QUOTIENT,
														 command.right * QUOTIENT
														 );
							
							bool movement_is_zero = (
													 command.forward == 0 &&
													 command.right == 0 &&
													 command.left == 0 &&
													 command.back == 0
													 );
							
							character->enableDamping(movement_is_zero);
						}
					}
				} // player_move
				
				
			public:
				BulletPlayerController() : target(0), character(0)
				{
				}
				
				virtual ~BulletPlayerController()
				{
					set_controlled_object(0);
				}
				
				virtual void set_controlled_object(ICollisionObject* collision_object)
				{
					target = collision_object;
					
					if (collision_object != 0)
					{
						// TODO: handle hot swapping of controlled object
						assert(character == 0);
						
						BulletCollisionObject* bullet_object = static_cast<BulletCollisionObject*>(collision_object);

						
						btScalar step_height = btScalar(.36);
						character = new KinematicCharacter((btPairCachingGhostObject*)bullet_object->get_collision_object(), (btConvexShape*)bullet_object->get_collision_shape(), step_height);
//						bullet::get_world()->addAction(character);
					}
					else
					{
//						bullet::get_world()->removeAction(character);
						delete character;
					}
				}
				
				virtual ICollisionObject* get_controlled_object() const
				{
					return target;
				}
				
				virtual void simulate(float delta_seconds)
				{
					if (character)
					{
						character->updateAction(bullet::get_world(), delta_seconds);
					}
				}
				
				virtual void apply_movement_command(const MovementCommand& command)
				{
					if (character)
					{
						move_player(character, command);
						
						// rotate physics body based on camera yaw
						btTransform worldTrans = character->getGhostObject()->getWorldTransform();
//						LOGV("TODO: get angles from camera\n");
//						btQuaternion rotation(btVector3(0,1,0), mathlib::degrees_to_radians(-camera->yaw));
//						worldTrans.setRotation(rotation);

						character->getGhostObject()->setWorldTransform(worldTrans);
						
						// sync the camera to the ghost object
						btGhostObject* ghost = character->getGhostObject();
						btTransform tr = ghost->getWorldTransform();
						btVector3 origin = tr.getOrigin();
						origin += btVector3(0, .9, 0);
						
//						LOGV("TODO: sync with camera\n");
//						camera->pos = glm::vec3(origin.x(), origin.y(), origin.z());
					}
				}
				
				virtual void set_view_angles(const glm::vec2& view_angles_degrees)
				{
					view_angles = view_angles_degrees;
				}
			};
		}





		PhysicsInterface::~PhysicsInterface()
		{
			// purge collision shapes
			for (auto& shape : collision_shapes)
			{
				DESTROY(ICollisionShape, shape);
			}
		}
		
		physics::ICollisionObject* PhysicsInterface::create_physics_object(ICollisionShape* shape, const glm::vec3& position, const glm::quat& orientation, ObjectProperties& properties)
		{
			BulletRigidBody* rigidbody = CREATE(BulletRigidBody);
			
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
			
			bullet:get_world()->addRigidBody(body);
			bullet::get_world()->addCollisionObject(ghost, btBroadphaseProxy::SensorTrigger, btBroadphaseProxy::CharacterFilter);
		
			return rigidbody;
		}
		

		physics::ICollisionObject* PhysicsInterface::create_physics_model(int32_t model_index, ObjectProperties& properties)
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
			
			ICollisionObject* object = 0;
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
				
				core::FixedArray<glm::vec3>& vertices = geo->vertices;
				
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
//					rb->set_mass_center_offset(mass_center_offset);
					body->setUserPointer(rb);
					
					body->setRestitution(properties.restitution);
					body->setFriction(properties.friction);
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
					btBvhTriangleMeshShape * shape_mesh = new btBvhTriangleMeshShape( mesh, use_quantized_bvh_tree );
//					btConvexTriangleMeshShape* shape_mesh = new btConvexTriangleMeshShape(mesh);
					
					btRigidBody::btRigidBodyConstructionInfo rigid_body_info(0.0f, motion_state, shape_mesh, local_inertia );
					body = new btRigidBody(rigid_body_info);
					static_body->set_collision_object(body);
					static_body->add_shape(shape_mesh);
					
					body->setUserPointer(static_body);
					
					int body_flags = body->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT;
					body->setCollisionFlags( body_flags );
					body->setFriction(0.75f);
					body->setWorldTransform(local_transform);
				}
				
				bullet:get_world()->addRigidBody(body);
			}
			
			return object;
		} // create_physics_model
		
		physics::ICollisionObject* PhysicsInterface::create_character_object(ICollisionShape* shape)
		{
			BulletCollisionObject* collision_object = CREATE(BulletCollisionObject);
			
			btPairCachingGhostObject* ghost = new btPairCachingGhostObject();
			ghost->setUserPointer(collision_object);
			
//			LOGV("TODO: set character object transform!\n");
			
			BulletCollisionShape* bullet_shape = static_cast<BulletCollisionShape*>(shape);
			assert(bullet_shape != 0);
			
			collision_object->set_collision_object(ghost);
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
			BulletCollisionObject* collision_object = CREATE(BulletCollisionObject);
			btPairCachingGhostObject* ghost = new btPairCachingGhostObject();
			ghost->setUserPointer(collision_object);
			collision_object->set_collision_object(ghost);
			
			BulletCollisionShape* bullet_shape = static_cast<BulletCollisionShape*>(shape);
			assert(bullet_shape != 0);
			
			ghost->setCollisionShape(bullet_shape->get_shape());
			ghost->setCollisionFlags(ghost->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
			
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
			BulletCollisionShape* collision_shape = CREATE(BulletCollisionShape);
			collision_shapes.push_back(collision_shape);
						
			btCollisionShape* capsule = new btCapsuleShape(radius_meters, height_meters);
			collision_shape->set_shape(capsule);
			return collision_shape;
		}
		
		
		physics::ICollisionShape* PhysicsInterface::create_box(const glm::vec3& dimensions)
		{
			BulletCollisionShape* collision_shape = CREATE(BulletCollisionShape);
			collision_shapes.push_back(collision_shape);
			
			btVector3 half_extents(dimensions.x*0.5f, dimensions.y*0.5f, dimensions.z*0.5f);
			btCollisionShape* box = new btBoxShape(half_extents);
			collision_shape->set_shape(box);
			
			return collision_shape;
		}
		
		void PhysicsInterface::destroy_object(ICollisionObject* object)
		{
			DESTROY(ICollisionObject, object);
		} // destroy_object


		IPlayerController* PhysicsInterface::create_player_controller(ICollisionObject* object)
		{
			IPlayerController* controller = CREATE(BulletPlayerController);
			controller->set_controlled_object(object);
			return controller;
		}
		
		void PhysicsInterface::destroy_player_controller(IPlayerController* controller)
		{
			DESTROY(IPlayerController, controller);
		}
		
		void PhysicsInterface::step_simulation(float delta_seconds)
		{
			bullet::step(delta_seconds);
		}
		
	} // namespace physics
} // namespace gemini