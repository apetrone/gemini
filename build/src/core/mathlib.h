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
#pragma once

#include "typedefs.h"

#if PLATFORM_WINDOWS
#pragma warning( disable: 4305 ) // warning C4305: 'initializing' : truncation from 'double' to 'const aengine::real'
#pragma warning( disable: 4244 ) //warning C4244: 'initializing' : conversion from 'double' to 'const aengine::real', possible loss of data
#endif


// show output at build time regarding glm
//#define GLM_MESSAGES 1
// acknowledge (and accept) the conversion to radians
#define GLM_FORCE_RADIANS 1
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
//#include <glm/gtc/noise.hpp>
//#include <glm/gtc/random.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

//
// math defines
//

typedef float real;
//typedef double real;


const real PI = 3.1415927f;
const real D2R_PI180 = (PI/180.0f);
const real R2D_180PI = (180.0f/PI);


#define PRINT_MAT4(m) \
	LOGV("%s:\n[%2.2f, %2.2f, %2.2f, %2.2f]\n[%2.2f, %2.2f, %2.2f, %2.2f]\n[%2.2f, %2.2f, %2.2f, %2.2f]\n[%2.2f, %2.2f, %2.2f, %2.2f]\n\n",\
	#m,\
	m[0][0], m[0][1], m[0][2], m[0][3], m[1][0], m[1][1], m[1][2], m[1][3], m[2][0], m[2][1], m[2][2], m[2][3], m[3][0], m[3][1], m[3][2], m[3][3]\
	);

namespace mathlib
{
	LIBRARY_EXPORT inline real degrees_to_radians(real degrees)
	{
		return degrees * D2R_PI180;
	}

	LIBRARY_EXPORT inline real radians_to_degrees(real radians)
	{
		return radians * R2D_180PI;
	}
	
	
	// pitch and yaw are in degrees; internally converted to radians
	LIBRARY_EXPORT void basis_vectors_from_pitch_yaw(float pitch, float yaw, glm::vec3& right, glm::vec3& view, bool invert_y_axis = true);
	
	
	LIBRARY_EXPORT bool point_in_radius(const glm::vec3& p0, const glm::vec3& p1, float radius = 3.0f);
}

#if 0
struct Segment
{
	aengine::vec3 start, end;
};



//
// math utility functions
void TriangleCentroid( const vec3 & a, const vec3 & b, const vec3 & c, vec3 & center );


#if WORLD_TO_SCREEN_GL
void WorldToScreenCoords( const vec3 & worldCoords, const mat4 & mv, const mat4 & proj, unsigned int vpWidth, unsigned int vpHeight, vec3 & screenCoords );
#else
void WorldToScreenCoords( const vec3 & worldCoords, const mat4 & mvp, unsigned int vpWidth, unsigned int vpHeight, vec3 & screenCoords );
#endif
bool ScreenToWorldCoords( const vec3 & screenCoords, const mat4 & modelViewProjection, unsigned int vpWidth, unsigned int vpHeight, vec3 & worldCoords );

void ClipSpaceToScreenCoords( const vec4 & clipCoords, unsigned int vpWidth, unsigned int vpHeight, vec3 & screenCoords );

void ClosestPoint( const aengine::Segment & s, const aengine::vec3 & p, aengine::vec3 & closestPoint );

bool Segment_CastSphere( const aengine::Segment & s, const aengine::vec3 & spherePosition, aengine::real radius, aengine::vec3 & p );
int Segment_CastCylinder( const vec3 & P, const vec3 & Q, aengine::real radius, const vec3 & A, const vec3 & B, aengine::real & t );
int Segment_CastPlane( const vec3 & planePoint, const vec3 & planeNormal, const vec3 & rayOrigin, const vec3 & rayDir, vec3 & point );
int Segment_CastAABB( const aengine::Segment & s, const aengine::vec3 & mins, const aengine::vec3 & maxs, aengine::vec3 & pt );

bool PointInTriangle( const aengine::vec3 & a, const aengine::vec3 & b, const aengine::vec3 & c, const aengine::vec3 & pt );

//http://local.wasp.uwa.edu.au/~pbourke/geometry/pointline/
// returns 1 if an intersection was found
// returns 0 if no intersection
int Intersect_Line_Segment( const aengine::vec3 & p1, const aengine::vec3 & p2, const aengine::vec3 & p3, const aengine::vec3 p4, aengine::real * ua, aengine::real * ub, aengine::real * x, aengine::real * y );

aengine::real Segment_Slope( const aengine::Segment & segment );
int Segment_Intersection( const aengine::Segment & line1, const aengine::Segment & line2, aengine::vec3 & pt );
#endif

namespace mathlib
{
	// this class assumes an origin in the upper left hand corner and assumes bottom > top, right > left
	template <class _Type>
	struct Rect
	{
		_Type left, top, right, bottom;
		
		Rect() : left( _Type(0) ),
				top( _Type(0) ),
				right( _Type(0) ),
				bottom( _Type(0) )
		{
		}
		
		Rect( _Type _left, _Type _top, _Type _right, _Type _bottom ) :
				left(_left),
				top(_top),
				right(_right),
				bottom(_bottom)
		{
		}
		
		_Type width() const
		{
			return ( right - left );
		}
		
		_Type height() const
		{
			return ( bottom - top );
		}
		
		Rect<_Type> operator- ( const Rect<_Type> & other ) const
		{
			return Rect<_Type> (left-other.left, top-other.top, right-other.top, bottom-other.bottom );
		}
		
		Rect<_Type> operator- ( const Rect<_Type> & other )
		{
			return Rect<_Type> (left-other.left, top-other.top, right-other.top, bottom-other.bottom );
		}
		
		Rect<_Type> operator+ ( const Rect<_Type> & other ) const
		{
			return Rect<_Type> (left+other.left, top+other.top, right+other.top, bottom+other.bottom );
		}
		
		bool fits_inside( const Rect<_Type> & other ) const
		{
			if ( width() <= other.width() && height() <= other.height() )
				return true;
			
			return false;
		}
		
	}; // Rect

	typedef Rect<float> Rectf;
	typedef Rect<int> Recti;


	struct AABB2
	{
		float left;
		float right;
		float top;
		float bottom;
		
		bool overlaps( const AABB2 & other ) const;
	}; // AABB2

} // namespace mathlib