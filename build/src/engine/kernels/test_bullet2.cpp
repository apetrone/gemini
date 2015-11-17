// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone
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
#include "kernel.h"
#include <stdio.h>
#include <slim/xlog.h>
#include "mathlib.h"
#include "debugdraw.h"
#include "input.h"
#include "renderer.h"
#include "renderstream.h"

#include "camera.h"

#include "assets/asset_mesh.h"

#include "btBulletDynamicsCommon.h"

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



namespace physics
{
	btDefaultCollisionConfiguration * collision_config;
	btCollisionDispatcher * dispatcher;
	btSequentialImpulseConstraintSolver * constraint_solver;
	btDiscreteDynamicsWorld * dynamics_world;
	btOverlappingPairCache * pair_cache;
	btAlignedObjectArray<btCollisionShape*> collision_shapes;
	btBroadphaseInterface * broadphase;


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


		//btAlignedAllocSetCustom( bullet2_custom_alloc, bullet2_custom_free );

		// instance and set the debug renderer
		//		dynamics_world->setDebugDrawer(0);
	}

	void shutdown()
	{
		//
		// Cleanup
		//
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
		for ( int j = 0; j < collision_shapes.size(); j++ )
		{
			btCollisionShape* shape = collision_shapes[j];
			collision_shapes[j] = 0;
			delete shape;
		}

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
	} // shutdown


	void step( float seconds )
	{
		assert( dynamics_world != 0 );

		dynamics_world->stepSimulation( seconds, 1, 1/60.0f );
	} // step
};



void DebugPhysicsRenderer::drawLine( const btVector3 & from, const btVector3 & to, const btVector3 & color )
{
	Color c = Color::fromFloatPointer( &color[0], 3 );
	debugdraw::line( BTVECTOR3_TO_VEC3( from ), BTVECTOR3_TO_VEC3( to ), c );
}

void DebugPhysicsRenderer::drawLine( const btVector3& from, const btVector3& to, const btVector3& fromColor, const btVector3& color )
{
	Color c = Color::fromFloatPointer( &color[0], 3 );
	debugdraw::line( BTVECTOR3_TO_VEC3( from ), BTVECTOR3_TO_VEC3( to ), c );
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
	return (btIDebugDraw::DBG_DrawWireframe | btIDebugDraw::DBG_DrawAabb);
}

class TestBullet2 : public kernel::IApplication,
public kernel::IEventListener<kernel::KeyboardEvent>,
public kernel::IEventListener<kernel::MouseEvent>,
public kernel::IEventListener<kernel::SystemEvent>
{

public:
	DECLARE_APPLICATION( TestBullet2 );
	assets::Mesh * plane_mesh;
	Camera camera;

	virtual void event( kernel::KeyboardEvent & event )
	{
		if (event.is_down)
		{
			if (event.key == input::KEY_ESCAPE)
			{
				kernel::instance()->set_active(false);
			}
			else if (event.key == input::KEY_J)
			{
				LOGV("check controllers\n");
			}
		}
	}

	virtual void event( kernel::MouseEvent & event )
	{
//		switch( event.subtype )
//		{
//			case kernel::MouseMoved:
//			{
//				if ( input::state()->mouse().is_down( input::MOUSE_LEFT ) )
//				{
//					int lastx, lasty;
//					input::state()->mouse().last_mouse_position( lastx, lasty );
//
//					camera.move_view( event.mx-lastx, event.my-lasty );
//				}
//                break;
//			}
//			default: break;
//		}
	}

	virtual void event( kernel::SystemEvent & event )
	{

	}

	virtual kernel::ApplicationResult config( kernel::Params & params )
	{
		params.window_title = "bullet2";
		params.window_width = 800;
		params.window_height = 600;
		return kernel::Application_Success;
	}

	virtual kernel::ApplicationResult startup( kernel::Params & params )
	{
		physics::startup();


		// load in the plane mesh
		plane_mesh = assets::meshes()->load_from_path("models/construct");
		if (plane_mesh)
		{
			plane_mesh->prepare_geometry();
		}

		return kernel::Application_Success;
	}

	virtual void step( kernel::Params & params )
	{
		physics::step( params.step_interval_seconds );
	}

	virtual void tick( kernel::Params & params )
	{
		camera.perspective( 60.0f, params.render_width, params.render_height, 0.1f, 128.0f );
		// This is appropriate for drawing 3D models, but not sprites
		camera.set_absolute_position( glm::vec3(8, 5, 8.0f) );
		camera.yaw = -45;
		camera.pitch = 30;
		camera.update_view();

		RenderStream rs;
		renderer::GeneralParameters gp;

		gp.camera_position = &camera.pos;
		gp.modelview_matrix = &camera.matCam;
		gp.projection_project = &camera.matProj;

		glm::mat4 ident;
		gp.object_matrix = &ident;

		rs.add_clear( renderer::CLEAR_COLOR_BUFFER | renderer::CLEAR_DEPTH_BUFFER );
		rs.add_viewport( 0, 0, params.render_width, params.render_height );
		rs.add_clearcolor( 0.1, 0.1, 0.1, 1.0f );

		for( unsigned short i = 0; i < plane_mesh->total_geometry; ++i )
		{
			render_utilities::stream_geometry( rs, &plane_mesh->geometry[i], gp );
		}


		rs.run_commands();
	}

	virtual void shutdown( kernel::Params & params )
	{
		physics::shutdown();
	}
};

IMPLEMENT_APPLICATION( TestBullet2 );
