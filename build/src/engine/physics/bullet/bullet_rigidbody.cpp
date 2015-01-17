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

#include <platform/mem.h>
#include <core/logging.h>

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
			
			void BulletRigidBody::set_parent(ICollisionObject* first, ICollisionObject* second)
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