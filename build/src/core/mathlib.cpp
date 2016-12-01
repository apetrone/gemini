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
#include "mathlib.h"

// Links
// http://forums.tigsource.com/index.php?topic=14154.0

// Point inside triangle
// http://www.blackpawn.com/texts/pointinpoly/default.html



//
// math functions
//



namespace mathlib
{
	void basis_vectors_from_pitch_yaw(float pitch, float yaw, glm::vec3& right, glm::vec3& view, bool invert_y_axis)
	{
		float _pitch;

		if (invert_y_axis)
		{
			_pitch = mathlib::degrees_to_radians(-pitch);
		}
		else
		{
			_pitch = mathlib::degrees_to_radians(pitch);
		}

		float _yaw = mathlib::degrees_to_radians(yaw);

		float sp = sin(_pitch);
		float cp = cos(_pitch);
		float sy = sin(_yaw);
		float cy = cos(_yaw);

		right = glm::vec3(cy, 0, sy);

		view[0] = sy * cp;
		view[1] = sp;
		view[2] = -cy * cp;
		view = glm::normalize(view);
	}

	glm::quat orientation_from_yaw_pitch(float yaw, float pitch, const glm::vec3& up, const glm::vec3& right)
	{
		glm::quat qyaw = glm::angleAxis(glm::radians(yaw), up);
		glm::quat qpitch = glm::angleAxis(glm::radians(pitch), right);
		return glm::normalize(qyaw * qpitch);
	}

	// Returns true if p0 is within radius units of p1
	bool point_in_radius(const glm::vec3& p0, const glm::vec3& p1, float radius)
	{

		/*
		 float dx = p1[0] - p0[0];
		 float dy = p1[1] - p0[1];
		 float dz = p1[2] - p0[2];

		 float len = sqrt( (dx*dx) + (dy*dy) + (dz*dz) );

		 if ( len <= radius )
		 return true;
		 else
		 return false;*/

		 // Use this method to save a sqrt.
		 // http://www.gamedev.net/community/forums/topic.asp?topic_id=221071
		glm::vec3 pt;
		pt = p1 - p0;
		float dot = glm::dot(pt, pt);
		float r2 = (radius*radius);
		return (dot < r2);
	} // point_in_radius

	glm::mat3 matrix_from_basis_vectors(const glm::vec3& right, const glm::vec3& up, const glm::vec3& forward)
	{
		glm::mat3 xform;
		xform[0] = right;
		xform[1] = up;
		xform[2] = forward;
		return xform;
	}

	glm::quat orientation_from_vectors(const glm::vec3& a, const glm::vec3& b)
	{
		// http://lolengine.net/blog/2013/09/18/beautiful-maths-quaternion-from-vectors
		assert(a.x >= -1.0f && a.x <= 1.0f);
		assert(a.y >= -1.0f && a.y <= 1.0f);
		assert(a.z >= -1.0f && a.z <= 1.0f);
		assert(b.x >= -1.0f && b.x <= 1.0f);
		assert(b.y >= -1.0f && b.y <= 1.0f);
		assert(b.z >= -1.0f && b.z <= 1.0f);

		float d = sqrt(2.0f + 2.0f * glm::dot(a, b));
		glm::vec3 result = (1.0f / d) * glm::cross(a, b);
		return glm::quat(0.5f * d, result.x, result.y, result.z);
	}

	void spherical_to_cartesian(float rho, float theta, float phi, glm::vec3& direction)
	{
		float pitch = (mathlib::PI) - phi;
		direction.x = rho * sinf(pitch) * cosf(theta);
		direction.z = rho * sinf(pitch) * sinf(theta);
		direction.y = rho * cos(pitch);
	}

	void cartesian_to_spherical(const glm::vec3& direction, float& rho, float& theta, float& phi)
	{
		rho = sqrt(glm::dot(direction, direction));
		theta = atan2(direction.z, direction.x);
		phi = acosf(direction.y / rho);
	}


	glm::vec3 rotate_vector(const glm::vec3& view, const glm::quat& rotation)
	{
		glm::quat qview(1.0f, view.x, view.y, view.z);

		glm::quat v = rotation * qview * glm::conjugate(rotation);
		return glm::vec3(v.x, v.y, v.z);
	}

	float unsigned_atan2(float y, float x)
	{
		float value = atan2(y, x);
		//if (value < 0.0f)
		//{
		//	value += mathlib::PI * 2;
		//}

		return value;
	}

	float wrap_euler_angle(float rads)
	{
		if (fabsf(rads) < mathlib::PI)
		{
			float iterations = (floor(rads + mathlib::PI) * (1.0f / mathlib::PI_2));
			rads -= iterations * mathlib::PI_2;
		}
		return rads;
	}

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
		const glm::vec3& right)
	{
		// extract camera frustum planes
		// given: aspect_ratio
		// fov
		// near plane distance.
		// - dimensions of the near plane are calculated as follows:
		float near_height = 2.0f * tan(field_of_view * 0.5f) * nearz;
		float near_width = (near_height * aspect_ratio);

		// far plane
		float far_height = 2.0f * tan(field_of_view * 0.5f) * farz;
		float far_width = (far_height * aspect_ratio);

		//camera_position = fc->get_origin();
		//target_position = fc->get_target();
		//up_vector = fc->get_up();
		//right_vector = fc->get_right();

		glm::vec3 near_point = (origin + view * nearz);
		glm::vec3 far_point = (origin + view * farz);

		// bottom left, bottom right, top right, top left; (ccw)
		near_corners[0] = (near_point - (up * near_height * 0.5f) - (right * near_width * 0.5f));
		near_corners[1] = (near_point - (up * near_height * 0.5f) + (right * near_width * 0.5f));
		near_corners[2] = (near_point + (up * near_height * 0.5f) + (right * near_width * 0.5f));
		near_corners[3] = (near_point + (up * near_height * 0.5f) - (right * near_width * 0.5f));

		front_corners[0] = (far_point - (up * far_height * 0.5f) - (right * far_width * 0.5f));
		front_corners[1] = (far_point - (up * far_height * 0.5f) + (right * far_width * 0.5f));
		front_corners[2] = (far_point + (up * far_height * 0.5f) + (right * far_width * 0.5f));
		front_corners[3] = (far_point + (up * far_height * 0.5f) - (right * far_width * 0.5f));
	} // compute_frustum_corners

	glm::vec3 transform_point(const glm::mat4& matrix, const glm::vec3& point)
	{
		return glm::vec3(matrix * glm::vec4(point, 1.0f));
	} // transform_point

	float covariance(size_t row, size_t column, float* values, size_t total_values, size_t stride)
	{
		// covariance(i, j) = 1/n * (sum[xi*xj] - (E[xi] * E[xj])

		float denom = 1.0f / static_cast<float>(total_values);

		float sum = 0.0f;
		uint8_t* row_pointer = reinterpret_cast<uint8_t*>(values + row);
		uint8_t* column_pointer = reinterpret_cast<uint8_t*>(values + column);
		float xi_sum = 0.0f;
		float xj_sum = 0.0f;
		for (size_t index = 0; index < total_values; ++index)
		{
			float* row_value = reinterpret_cast<float*>(row_pointer);
			float* column_value = reinterpret_cast<float*>(column_pointer);
			sum += (*row_value * *column_value);

			xi_sum += *row_value;
			xj_sum += *column_value;

			row_pointer += stride;
			column_pointer += stride;
		}

		xi_sum *= denom;
		xj_sum *= denom;

		return (sum * denom) - (xi_sum * xj_sum);
	} // covariance
}




namespace mathlib
{
	bool AABB2::overlaps( const AABB2 & other ) const
	{
		if ( this->left > other.right )
		{
			return false;
		}
		else if ( this->right < other.left )
		{
			return false;
		}

		if ( this->bottom < other.top )
		{
			return false;
		}
		else if ( this->top > other.bottom )
		{
			return false;
		}


		return true;
	} // overlaps
} // namespace mathlib

#if 0

// given vertices a, b, and c, find the center of the triangle
void TriangleCentroid( const vec3 & a, const vec3 & b, const vec3 & c, vec3 & center )
{
	// g = 1/3 (a + b + c)
	center.x = ( a.x + b.x + c.x ) / 3;
	center.y = ( a.y + b.y + c.y ) / 3;
	center.z = ( a.z + b.z + c.z ) / 3;
}



// Convert World Coordinates to Screen Coordinates
// TODO: Allow specification of Depth Range
// http://www.songho.ca/opengl/gl_transform.html
void WorldToScreenCoords( const vec3 & worldCoords, const mat4 & mvp, unsigned int vpWidth, unsigned int vpHeight, vec3 & screenCoords )
{
	// http://www.flipcode.com/archives/Object_To_Screen_Space.shtml
	// homogenous coordinates, w = 1
	vec4 result( worldCoords );

	// convert to Eye Coordinates (modelview) then to Clip Coordinates (projection)
	mat4::mul( mvp, result, result );

	double rhw = (1 / result[3]);

	double depthRange[2];
	depthRange[0] = 0;
	depthRange[1] = 1;

	//glGetDoublev( GL_DEPTH_RANGE, depthRange );

	// calculate normalized device coordinates
	aengine::vec3 ndc( result[0], result[1], result[2] );

	//printf( "4D Clip: %g %g %g %g\n", result[0], result[1], result[2], result[3] );


	aengine::vec3::scale( ndc, rhw, ndc );
	//printf( "ndc: %g %g %g\n", ndc[0], ndc[1], ndc[2] );

	// viewport transformation
	screenCoords[0] = (1 + ndc[0]) * vpWidth / 2.0;
	screenCoords[1] = (1 - ndc[1]) * vpHeight / 2.0;
	screenCoords[2] = ndc[2] * (depthRange[1] - depthRange[0]) + depthRange[0];
} // WorldToScreenCoordinates

/// @brief Convert window coordinates to world space coordinates
/// @details A replacement for OpenGL's gluUnProject - http://pyopengl.sourceforge.net/documentation/manual/gluUnProject.3G.xml
/// @params screenCoords Window Coordinates (relative to the origin in the upper left)
/// @params modelview * projection matrix
/// @params viewport width
/// @params viewport height
/// @params worldCoords An out vector in world coordinates
/// @returns true if the calculation was successful, false if the matrix did not have an inverse
bool ScreenToWorldCoords( const vec3 & screenCoords, const mat4 & modelViewProjection, unsigned int vpWidth, unsigned int vpHeight, vec3 & worldCoords )
{
	bool success = false;

	vec4 src;
	vec4 world;

	// (viewport[3]-windowCoords.y) inverts the y-coordinate of the windowCoords because OpenGL's origin is in the lower left

	// I believe this places the coordinates into clip space...
	src[0] = ((2 * screenCoords[0])/vpWidth) - 1;
	src[1] = ((2 * (vpHeight-screenCoords[1]))/vpHeight) - 1;
	src[2] = (2 * screenCoords[2]) - 1;
	src[3] = 1;

	// invert the modelview projection matrix
	mat4 invmvproj;
	success = mat4::inverse( modelViewProjection, invmvproj );

	if ( !success )
	{
		// no inverse for modelview_projection
		return success;
	}

	// then we multiply the clip-space coordinate by the inverse modelview projection matrix
	// multiply the point by the inverse modelview projection matrix
	mat4::mul( invmvproj, src, world );

	// finally, scale by the inverse projection divider
	// scale by the weighted value
	vec4::scale( world, (1/world[3]), world );

	worldCoords.set( world[0], world[1], world[2] );

	return success;
} // ScreenToWorldCoords

void ClipSpaceToScreenCoords( const vec4 & clipCoords, unsigned int vpWidth, unsigned int vpHeight, vec3 & screenCoords )
{
	aengine::vec3 depthRange( 0, 1 );

	// calculate normalized device coordinates
	aengine::vec3 ndc( clipCoords[0], clipCoords[1], clipCoords[2] );

	// perspective divide
	aengine::vec3::scale( ndc, (1 / clipCoords[3]), ndc );

	// viewport transformation
	screenCoords[0] = (1 + ndc[0]) * vpWidth / 2.0;
	screenCoords[1] = (1 - ndc[1]) * vpHeight / 2.0;
	screenCoords[2] = ndc[2] * (depthRange[1] - depthRange[0]) + depthRange[0];
} // ClipSpaceToScreenCoords

// Find closest point on AB from P
void ClosestPoint( const aengine::Segment & s, const aengine::vec3 & p, aengine::vec3 & closestPoint )
{
	// http://www.gamedev.net/community/forums/topic.asp?topic_id=221071

	aengine::vec3 ab;
	aengine::vec3::sub( s.end, s.start, ab );

	aengine::vec3 ap;
	aengine::vec3::sub( p, s.start, ap );

	// taking the dot product of a vector with itself is like finding the square length of the vector
	float ab_dot_ab = aengine::vec3::dot( ab, ab );
	float ap_dot_ab = aengine::vec3::dot( ap, ab );
	float t = (ap_dot_ab / ab_dot_ab);

	if ( t < 0.0 )
	{
		t = 0.0;
	}
	else if ( t > 1.0 )
	{
		t = 1.0;
	}

	aengine::vec3::scale( ab, t, closestPoint );
	aengine::vec3::add( closestPoint, s.start, closestPoint );
} // ClosestPoint

// Segment_Cast vs Sphere
bool Segment_CastSphere( const aengine::Segment & s, const aengine::vec3 & spherePos, aengine::real radius, aengine::vec3 & pt )
{
	// http://www.gamedev.net/community/forums/topic.asp?topic_id=221071

	// 1) Find the closest point on segment start->end from sphere pos
	aengine::ClosestPoint( s, spherePos, pt );

	// 2) See if that pt is inside the sphere
	return aengine::IsInRadius( spherePos, pt, radius );
} // Segment_CastSphere

// This function is from Christer Ericson's Real Time Collision Detection (page 197)
int Segment_CastCylinder( const vec3 & P, const vec3 & Q, aengine::real radius, const vec3 & A, const vec3 & B, aengine::real & t )
{
	// Other links:
	// point sphere/cylinder collision - http://www.gamedev.net/community/forums/topic.asp?topic_id=436258
	// Ray/Cylinder intersection - http://www.gamedev.net/community/forums/topic.asp?topic_id=467789
	// Quadratic Equation Calculator - http://www.1728.com/quadratc.htm
	// Intersection notes - http://www.cs.toronto.edu/~smalik/418/tutorial8_ray_primitive_intersections.pdf

	t = 0.0f;

	vec3 d;
	vec3 m;
	vec3 n;

	vec3::sub( Q, P, d );
	vec3::sub( A, P, m );
	vec3::sub( B, A, n );

	aengine::real md = aengine::vec3::dot( m, d );
	aengine::real nd = aengine::vec3::dot( n, d );
	aengine::real dd = aengine::vec3::dot( d, d );

	if ( md < 0.0f && (md + nd < 0.0f) )
	{
		//printf( "Segment outside of P end cap.\n" );
		return 0; // segment outside P end cap of the cylinder
	}
	if ( md > dd && (md + nd > dd) )
	{
		//printf( "Segment outside of Q end cap.\n" );
		return 0; // segment outside Q end cap of the cylinder
	}

	aengine::real nn = vec3::dot( n, n );
	aengine::real mn = vec3::dot( m, n );
	aengine::real mm = vec3::dot( m, m );

	aengine::real a = dd * nn - nd * nd;
	aengine::real k = mm - radius * radius;
	aengine::real c = dd * k - md * md;

	if ( fabs(a) < FLT_EPSILON )
	{
		if ( c > 0.0f )
		{
			// a and thus the segment lie outside cylinder
			//printf( "Segment lies outside of cylinder.\n" );
			return 0;
		}

		// Now known that segment intersects cylinder; figure out how it intersects
		if ( md < 0.0f )
			t = -mn / nn; // intersect segment against 'p' endcap
		else if ( md > dd )
			t = (nd - mn) / nn; // intersect segment against 'q' endcap
		else
			t = 0.0f; // a lies inside the cylinder

		return 1;
	}

	aengine::real b = dd * mn - nd * md;
	aengine::real discr = b * b - a * c;

	if ( discr < 0.0f )
	{
		//printf( "No real roots.\n" );
		return 0; // no real roots, no intersection
	}

	t = (-b - sqrt(discr)) / a;

	if ( t < 0.0f || t > 1.0f )
	{
		//printf( "Intersection lies outside segment!\n" );
		return 0; // intersection lies outside segment
	}

	if ( md + t * nd < 0.0f )
	{
		// intersection outside cylinder on 'p' side
		if ( nd <= 0.0f )
		{
			//printf( "Segment pointing away from P end cap.\n" );
			return 0; // segment pointing away from endcap
		}

		t = -md / nd;

		//printf( "Testing whether to keep P intersection...\n" );
		// keep intersection if Dot(S(t) - p, S(t) - p) <= r^2
		float r = k + 2 * t * (mn + t * nn);

		//if ( r > 0.0f )
		//printf( "r is: %g, t is: %g, k is: %g, mn is: %g, nn is: %g\n", r, t, k, mn, nn );

		return r <= 0.0f;
	}
	else if ( md + t * nd > dd )
	{
		// intersection outside cylinder on 'q' side
		if ( nd >= 0.0f )
		{
			//printf( "Segment pointing away from Q end cap.\n" );
			return 0; // segment pointing away from endcap
		}

		t = (dd - md) / nd;

		//printf( "Testing whether to keep Q intersection...\n" );
		// keep intersection if Dot(S(t) - q, S(t) - q) <= r^2
		return k + dd - 2 * md + t * (2* (mn-nd) + t * nn) <= 0.0f;
	}

	// segment intersects cylinder between the endcaps; t is correct
	return 1;
} // Segment_CastCylinder


// returns 1 if intersection is within range of (0 - 1)
// 0 if intersection is beyond (0-1) range
// D is the negative distance (as its on left side of equation) to the origin along the normal
int Segment_CastPlane( const vec3 & plPoint, const vec3 & plNormal, const vec3 & rOrigin, const vec3 & rDir, vec3 & point )
{
	// distance to origin
	aengine::real D = - aengine::vec3::dot( plNormal, plPoint );

	aengine::real ndir = aengine::vec3::dot( plNormal, rDir );

	if ( ndir == 1 )
	{
		// ray is pointing in the same direction as the plane
		return -1;
	}

	aengine::real norg = ( aengine::vec3::dot( plNormal, rOrigin ) + D );
	aengine::real t = (-norg / ndir);


	// P = rOrigin + t*rDir
	point[0] = (rOrigin[0] + (rDir[0] * t));
	point[1] = (rOrigin[1] + (rDir[1] * t));
	point[2] = (rOrigin[2] + (rDir[2] * t));


	if ( (t < 0) || (t > 1) )
	{
		// ray and normal are backfacing
		return 0;
	}
	else
	{
		return 1;
	}

} // Segment_CastPlane

int Segment_CastAABB( const aengine::Segment & s, const aengine::vec3 & mins, const aengine::vec3 & maxs, aengine::vec3 & pt )
{
	// http://www.siggraph.org/education/materials/HyperGraph/raytrace/rtinter3.htm
	// Slab test

	// http://www.gamedev.net/community/forums/topic.asp?topic_id=309689

	aengine::vec3 d;
	aengine::vec3::sub( s.end, s.start, d );

	float n = 0;
	float f = 1.0f;
	// check len of d is not zero

	for( int i = 0; i < 3; ++i )
	{

		float t0 = 0;
		float t1 = 0;

		// compute intersection of X slab
		t0 = (mins[i] - s.start[i]) / d[i];
		t1 = (maxs[i] - s.start[i]) / d[i];

		if ( t0 > t1 )
		{
			//printf( "SWAP\n" );
			float temp = t0;
			t0 = t1;
			t1 = temp;
		}

		if ( t0 > n )
			n = t0;

		if ( t1 < f )
			f = t1;

		if ( n > f )
		{
			// missed this slab
			return 0;
		}

		if ( f < 0 )
		{
			// box behind ray
			return 0;
		}

	}

	pt[0] = s.start[0] + (d[0] * n);
	pt[1] = s.start[1] + (d[1] * n);
	pt[2] = s.start[2] + (d[2] * n);

	// could also calculate exit point by multiplying * f instead of n

	return 1;
}

// http://www.blackpawn.com/texts/pointinpoly/default.html
bool PointInTriangle( const vec3 & a, const vec3 & b, const vec3 & c, const vec3 & pt )
{
	vec3 v0, v1, v2;

	vec3::sub( c, a, v0 );
	vec3::sub( b, a, v1 );
	vec3::sub( pt, a, v2 );

	// dot products
	aengine::real d00 = vec3::dot( v0, v0 );
	aengine::real d01 = vec3::dot( v0, v1 );
	aengine::real d02 = vec3::dot( v0, v2 );
	aengine::real d11 = vec3::dot( v1, v1 );
	aengine::real d12 = vec3::dot( v1, v2 );

	aengine::real invDenom = 1 / ( d00 * d11 - d01 * d01 );
	aengine::real u = (d11 * d02 - d01 * d12) * invDenom;
	aengine::real v = (d00 * d12 - d01 * d02) * invDenom;

	return (u > 0) && (v > 0) && (u+v < 1);
} // PointInTriangle

//http://local.wasp.uwa.edu.au/~pbourke/geometry/pointline/
// returns 1 if an intersection was found
// returns 0 if no intersection
int Intersect_Line_Segment( const aengine::vec3 & p1, const aengine::vec3 & p2, const aengine::vec3 & p3, const aengine::vec3 p4, aengine::real * ua, aengine::real * ub, aengine::real * x, aengine::real * y )
{
	aengine::real denom = ((p4[1] - p3[1]) * (p2[0] - p1[0])) - ((p4[0] - p3[0]) * (p2[1] - p1[1]));

	*ua = ((p4[0] - p3[0]) * (p1[1] - p3[1])) - ((p4[1] - p3[1]) * (p1[0] - p3[0]));
	*ub = ((p2[0] - p1[0]) * (p1[1] - p3[1])) - ((p2[1] - p1[1]) * (p1[0] - p3[0]));

	if ( denom == 0 )
	{
		//printf( "lines are coincidental\n" );
		return 0;
	}

	*ua /= denom;
	*ub /= denom;

	// ua and ub must be within the 0,1 range
	// otherwise, the line segments would need to be extended in order for an intersection
	if ( *ua >= 0.0 && *ua <= 1.0 && *ub >= 0.0 && *ub <= 1.0 )
	{
		*x = p1[0] + (*ua) * (p2[0] - p1[0]);
		*y = p1[1] + (*ub) * (p2[1] - p1[1]);
		return 1;
	}

	return 0;
} // Intersect_Line_Segment

aengine::real Segment_Slope( const aengine::Segment & segment )
{
	return (segment.start[1] - segment.end[1]) / (segment.start[0] - segment.end[0]);
} // Segment_Slope

//http://local.wasp.uwa.edu.au/~pbourke/geometry/pointline/
// returns 1 if an intersection was found
// returns 0 if no intersection
int Segment_Intersection( const aengine::Segment & line1, const aengine::Segment & line2, aengine::vec3 & pt )
{
	aengine::real ua, ub;
	return Intersect_Line_Segment( line1.start, line1.end, line2.start, line2.end, &ua, &ub, &pt[0], &pt[1] );
} // Segment_Intersection

#endif
