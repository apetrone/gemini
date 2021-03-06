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

#if defined(PLATFORM_COMPILER_MSVC)
	#pragma warning(push)
	#pragma warning(disable: 4305) // 'initializing' : truncation from 'double' to 'const aengine::real'
	#pragma warning(disable: 4244) // 'initializing' : conversion from 'double' to 'const aengine::real', possible loss of data
	#pragma warning(disable: 4668) // '_M_IX86_FP' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
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

#if defined(PLATFORM_COMPILER_MSVC)
	#pragma warning(pop)
#endif

//
// math defines
//

typedef float real;
//typedef double real;


#define PRINT_MAT4(m) \
	LOGV("%s:\n[%2.2f, %2.2f, %2.2f, %2.2f]\n[%2.2f, %2.2f, %2.2f, %2.2f]\n[%2.2f, %2.2f, %2.2f, %2.2f]\n[%2.2f, %2.2f, %2.2f, %2.2f]\n\n",\
	#m,\
	m[0][0], m[0][1], m[0][2], m[0][3], m[1][0], m[1][1], m[1][2], m[1][3], m[2][0], m[2][1], m[2][2], m[2][3], m[3][0], m[3][1], m[3][2], m[3][3]\
	);

#define MAT4_POSITION(mat)\
	{\
		glm::vec3 pos = glm::vec3(glm::column(mat, 3));\
		LOGV("%s pos: %2.2f, %2.2f, %2.2f\n", #mat, pos.x, pos.y, pos.z);\
	}

#define PRINT_VEC4(v) \
	LOGV("%s: [%2.2f, %2.2f, %2.2f, %2.2f]\n", \
	#v,\
	v.x, v.y, v.z, v.w);

#define PRINT_VEC3(v) \
	LOGV("%s:[%2.2f, %2.2f, %2.2f]\n", \
	#v,\
	v.x, v.y, v.z);

#define PRINT_VEC2(v) \
	LOGV("%s:[%2.2f, %2.2f]\n", \
	#v,\
	v.x, v.y);


namespace mathlib
{
	const real PI = (float)3.14159265358979323846f;
	const real PI_2 = (PI * 2);
	const real D2R_PI180 = (PI / 180.0f);
	const real R2D_180PI = (180.0f / PI);
	const real EPSILON = FLT_EPSILON;

	// this class assumes an origin in the upper left hand corner and assumes bottom > top, right > left
	template <class _Type>
	struct Rect
	{
		_Type left, top, right, bottom;

		Rect() : left(_Type(0)),
			top(_Type(0)),
			right(_Type(0)),
			bottom(_Type(0))
		{
		}

		Rect(_Type _left, _Type _top, _Type _right, _Type _bottom) :
			left(_left),
			top(_top),
			right(_right),
			bottom(_bottom)
		{
		}

		_Type width() const
		{
			return (right - left);
		}

		_Type height() const
		{
			return (bottom - top);
		}

		Rect<_Type> operator- (const Rect<_Type> & other) const
		{
			return Rect<_Type>(left - other.left, top - other.top, right - other.top, bottom - other.bottom);
		}

		Rect<_Type> operator- (const Rect<_Type> & other)
		{
			return Rect<_Type>(left - other.left, top - other.top, right - other.top, bottom - other.bottom);
		}

		Rect<_Type> operator+ (const Rect<_Type> & other) const
		{
			return Rect<_Type>(left + other.left, top + other.top, right + other.top, bottom + other.bottom);
		}

		bool fits_inside(const Rect<_Type> & other) const
		{
			if (width() <= other.width() && height() <= other.height())
				return true;

			return false;
		}

		bool is_null() const
		{
			return (width() == 0 && height() == 0);
		}
	}; // Rect

	typedef Rect<float> Rectf;
	typedef Rect<int> Recti;

	inline real degrees_to_radians(real degrees)
	{
		return degrees * D2R_PI180;
	}

	inline real radians_to_degrees(real radians)
	{
		return radians * R2D_180PI;
	}


	// pitch and yaw are in degrees; internally converted to radians
	void basis_vectors_from_pitch_yaw(float pitch, float yaw, glm::vec3& right, glm::vec3& view, bool invert_y_axis = true);

	// assumes yaw's rotational axis is +Y; and pitch's rotational axis is +X.
	glm::quat orientation_from_yaw_pitch(float yaw, float pitch, const glm::vec3& up, const glm::vec3& right);

	bool point_in_radius(const glm::vec3& p0, const glm::vec3& p1, float radius = 3.0f);

	glm::mat3 matrix_from_basis_vectors(const glm::vec3& right, const glm::vec3& up, const glm::vec3& forward);

	// this only accepts unit vectors
	glm::quat orientation_from_vectors(const glm::vec3& a, const glm::vec3& b);

	void spherical_to_cartesian(float rho, float theta, float phi, glm::vec3& direction);
	void cartesian_to_spherical(const glm::vec3& direction, float& rho, float& theta, float& phi);


	glm::vec3 rotate_vector(const glm::vec3& view, const glm::quat& rotation);

	// Like atan2, but return 0,2*PI as unsigned values.
	// Clockwise ascending.
	float unsigned_atan2(float y, float x);

	float wrap_euler_angle(float rads);


	void compute_frustum_corners(
		glm::vec3* near_corners,
		glm::vec3* front_corners,
		float nearz,
		float farz,
		float aspect_ratio,
		float field_of_view,
		const glm::vec3& origin,
		const glm::vec3& view,
		const glm::vec3& up,
		const glm::vec3& right);

	glm::vec3 transform_point(const glm::mat4& matrix, const glm::vec3& point);
} // namespace mathlib

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

