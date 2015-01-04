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

#include <platform/mem.h>
#include <slim/xlog.h>

#include "bullet_rigidbody.h"
#include "bullet_common.h"
#include "bullet_constraint.h"

namespace gemini
{
	namespace physics
	{	
		namespace bullet
		{
			BulletRigidBody::BulletRigidBody()
			{
				this->collision_type = physics::CollisionType_Dynamic;
			}
			
			BulletRigidBody::~BulletRigidBody()
			{
				remove_constraints();
				
				btRigidBody* body = get_bullet_body();

				if (body)
				{
					if (body->getMotionState())
					{
						delete body->getMotionState();
					}
					
					bullet::get_world()->removeCollisionObject(body);
				}
			}


			btRigidBody* BulletRigidBody::get_bullet_body() const { return  btRigidBody::upcast(object); }
			
			void BulletRigidBody::apply_force(const glm::vec3& force, const glm::vec3& local_position)
			{
				btRigidBody* body = get_bullet_body();
				if (body)
				{
					body->activate();
					body->applyForce(btVector3(force.x, force.y, force.z), btVector3(local_position.x, local_position.y, local_position.z));
				}
			}
			
			void BulletRigidBody::apply_central_force(const glm::vec3& force)
			{
				btRigidBody* body = get_bullet_body();
				if (body)
				{
					body->activate();
					body->applyCentralForce(btVector3(force.x, force.y, force.z));
				}
			}
			
			void BulletRigidBody::set_mass(float mass)
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
			
			void BulletRigidBody::set_parent(CollisionObject* first, CollisionObject* second)
			{
				LOGV("TODO: implement set_parent\n");
//				btRigidBody* rb0 = get_bullet_body();
//				btRigidBody* rb1 = (static_cast<BulletRigidBody*>(second))->get_bullet_body();
//				
//				btTypedConstraint* joint = new btPoint2PointConstraint(*rb0, *rb1, btVector3(0, 1, 0), btVector3(0, -1, 0));
//				bullet::get_world()->addConstraint(joint);
//				joint->setDbgDrawSize(btScalar(5.0f));
//				
//				BulletConstraint* constraint = CREATE(BulletConstraint, joint);
//				first->add_constraint(constraint);
//				second->add_constraint(constraint);
			}

		} // namespace bullet
	} // namespace physics
} // namespace gemini