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
#include "kernel.hpp"
#include <stdio.h>
#include <slim/xlog.h>
#include "mathlib.h"
#include "debugdraw.hpp"
#include "input.hpp"

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


void DebugPhysicsRenderer::drawLine( const btVector3 & from, const btVector3 & to, const btVector3 & color )
{
#if 0
	glLineWidth( 2.0f );
	glColor3f( color.x(), color.y(), color.z() );
	glBegin( GL_LINES );
	glVertex3f( from.x(), from.y(), from.z() );
	glVertex3f( to.x(), to.y(), to.z() );
	glEnd();
	glLineWidth( 1.0f );
#endif
	Color c = Color::fromFloatPointer( &color[0], 3 );
	debugdraw::line( BTVECTOR3_TO_VEC3( from ), BTVECTOR3_TO_VEC3( to ), c );
}

void DebugPhysicsRenderer::drawLine( const btVector3& from, const btVector3& to, const btVector3& fromColor, const btVector3& color )
{
#if 0
	glLineWidth( 2.0f );
	glBegin( GL_LINES );
	glColor3f( fromColor.x(), fromColor.y(), fromColor.z() );
	glVertex3f( from.x(), from.y(), from.z() );
	glColor3f( toColor.x(), toColor.y(), toColor.z() );
	glVertex3f( to.x(), to.y(), to.z() );
	glEnd();
	glLineWidth( 1.0f );
#endif
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
	btDefaultCollisionConfiguration * collision_config;
	btCollisionDispatcher * dispatcher;
	btSequentialImpulseConstraintSolver * constraint_solver;
	btDiscreteDynamicsWorld * dynamics_world;
	btOverlappingPairCache * pair_cache;
	btAlignedObjectArray<btCollisionShape*> collision_shapes;
	btBroadphaseInterface * broadphase;
public:
	DECLARE_APPLICATION( TestBullet2 );

	
	virtual void event( kernel::KeyboardEvent & event )
	{
		if (event.is_down)
		{
			if (event.key == input::KEY_ESCAPE)
			{
				kernel::instance()->set_active(false);
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
		
		collision_config = CREATE(btDefaultCollisionConfiguration);
		dispatcher = CREATE(btCollisionDispatcher, collision_config );
		
		btVector3 worldAabbMin(-1000,-1000,-1000);
		btVector3 worldAabbMax(1000,1000,1000);

		constraint_solver = CREATE(btSequentialImpulseConstraintSolver);
		
//		broadphase = new btDbvtBroadphase();
		broadphase = CREATE(btAxisSweep3, worldAabbMin, worldAabbMax);
		
		pair_cache = broadphase->getOverlappingPairCache();
		dynamics_world = CREATE(btDiscreteDynamicsWorld, dispatcher, (btBroadphaseInterface*)broadphase, constraint_solver, collision_config);
		dynamics_world->setGravity( btVector3( 0, -10, 0 ) );
		dynamics_world->getDispatchInfo().m_useConvexConservativeDistanceUtil = true;
		dynamics_world->getDispatchInfo().m_convexConservativeDistanceThreshold = 0.01;
		dynamics_world->getDispatchInfo().m_allowedCcdPenetration = 0.0;
		
		// instance and set the debug renderer
//		dynamics_world->setDebugDrawer(0);

		return kernel::Application_Success;
	}

	virtual void step( kernel::Params & params )
	{
	}

	virtual void tick( kernel::Params & params )
	{
	}
	
	virtual void shutdown( kernel::Params & params )
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
				btMotionState * ms = body->getMotionState();
				DESTROY(btMotionState, ms);
			}
			dynamics_world->removeCollisionObject( obj );

			DESTROY(btCollisionObject, obj);
		}
		
		//delete collision shapes
		for ( int j = 0; j < collision_shapes.size(); j++ )
		{
			btCollisionShape* shape = collision_shapes[j];
			collision_shapes[j] = 0;
			DESTROY(btCollisionShape, shape);
		}
		
		//delete dynamics world
		DESTROY(btDiscreteDynamicsWorld, dynamics_world);
		
		//delete solver
		DESTROY(btSequentialImpulseConstraintSolver, constraint_solver);
		
		//delete broadphase
		//delete mPairCache;
		DESTROY(btBroadphaseInterface, broadphase);
		
		//delete dispatcher
		DESTROY(btCollisionDispatcher, dispatcher);
		
		// delete the collision configuration
		DESTROY(btDefaultCollisionConfiguration, collision_config);
		
		//next line is optional: it will be cleared by the destructor when the array goes out of scope
		collision_shapes.clear();
	}
};

IMPLEMENT_APPLICATION( TestBullet2 );