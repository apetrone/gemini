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
#pragma once

#include <btBulletDynamicsCommon.h>

#include <platform/mem.h>

#include "camera.h"

#include "physics_collisionobject.h"
#include "physics_rigidbody.h"
#include "physics_trigger.h"



#include "charactercontroller.h"


#include "assets/asset_mesh.h"

namespace physics
{
	struct MovementCommand
	{
		unsigned int time;
		bool left;
		bool right;
		bool forward;
		bool back;
		
		MovementCommand()
		{
			memset(this, 0, sizeof(MovementCommand));
		}
	};

	
	class PhysicsMotionInterface
	{
	public:
		virtual ~PhysicsMotionInterface() {};
		
		// called when the physics body's motion state needs to be retrieved
		virtual void get_transform(glm::vec3& position, const glm::quat& orientation) = 0;
		
		// called when the physics body's motion state has been set
		virtual void set_transform(const glm::vec3& position, const glm::quat& orientation, const glm::vec3& mass_center_offset) = 0;
	};

#define BTVECTOR3_TO_VEC3( v ) glm::vec3( v.x(), v.y(), v.z() )
	class DebugPhysicsRenderer : public btIDebugDraw
	{
	public:
		virtual void drawLine( const btVector3 & from, const btVector3 & to, const btVector3 & color );
		virtual void    drawLine( const btVector3& from, const btVector3& to, const btVector3& fromColor, const btVector3& toColor );
		virtual void	drawContactPoint(const btVector3& PointOnB,const btVector3& normalOnB,btScalar distance,int lifeTime,const btVector3& color);
		virtual void	reportErrorWarning(const char* warningString);
		virtual void	draw3dText(const btVector3& location,const char* textString);
		virtual void	setDebugMode(int debugMode);
		virtual int		getDebugMode() const;
	}; // DebugPhysicsRenderer

	void startup();
	void shutdown();
	void step(float seconds);
	void debug_draw();
	
	CharacterController* create_character_controller(const btVector3& spawnLocation, bool addActionToWorld);
	void copy_ghost_to_camera(btPairCachingGhostObject* ghost, Camera& cam);
	void player_move(CharacterController* character, Camera& camera, const MovementCommand& command);
	CollisionObject* create_physics_for_mesh(assets::Mesh* mesh, float mass_kg = 0.0f, PhysicsMotionInterface* motion = nullptr, const glm::vec3& mass_center_offset = glm::vec3(0, 0, 0));
	CollisionObject* create_trigger(const glm::vec3& size);
}; // namespace physics