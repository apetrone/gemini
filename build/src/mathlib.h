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

#if _WIN32
#pragma warning( disable: 4305 ) // warning C4305: 'initializing' : truncation from 'double' to 'const aengine::real'
#pragma warning( disable: 4244 ) //warning C4244: 'initializing' : conversion from 'double' to 'const aengine::real', possible loss of data
#endif


// show output at build time regarding glm
//#define GLM_MESSAGES 1
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


const real PI = 3.14159265358979323846;
const real D2R_PI180 = (PI/180.0);
const real R2D_180PI = (180.0/PI);

inline real DegToRad( real degrees )
{
	return degrees * D2R_PI180;
}

inline real RadToDeg( real radians )
{
	return radians * R2D_180PI;
}


#if 0
struct Segment
{
	aengine::vec3 start, end;
};



//
// math utility functions
void TriangleCentroid( const vec3 & a, const vec3 & b, const vec3 & c, vec3 & center );
bool IsInRadius( const vec3 & p0, const vec3 & p1, float radius = 3.0f );

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
