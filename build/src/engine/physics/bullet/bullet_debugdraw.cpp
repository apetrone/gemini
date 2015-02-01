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

#include "bullet_debugdraw.h"

#include <core/logging.h>
#include <core/color.h>
#include "debugdraw.h"

#define BTVECTOR3_TO_VEC3( v ) glm::vec3( v.x(), v.y(), v.z() )

using namespace core;

namespace gemini
{
	namespace physics
	{
		namespace bullet
		{
			void DebugPhysicsRenderer::drawLine( const btVector3 & from, const btVector3 & to, const btVector3 & color )
			{
				Color c = Color::fromFloatPointer( &color[0], 3 );
				debugdraw::line( BTVECTOR3_TO_VEC3( from ), BTVECTOR3_TO_VEC3( to ), c, 0 );
			}
			
			void DebugPhysicsRenderer::drawLine( const btVector3& from, const btVector3& to, const btVector3& fromColor, const btVector3& color )
			{
				Color c = Color::fromFloatPointer( &color[0], 3 );
				debugdraw::line( BTVECTOR3_TO_VEC3( from ), BTVECTOR3_TO_VEC3( to ), c, 0 );
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
				return (btIDebugDraw::DBG_DrawWireframe); // | btIDebugDraw::DBG_DrawAabb;
			}
		} // namespace bullet
	} // namespace physics
} // namespace gemini