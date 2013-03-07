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
#include "typedefs.h"
#include "log.h"
#include "camera.hpp"


// --------------------------------------------------------
// Camera
// --------------------------------------------------------
Camera::Camera()
{
	yaw = pitch = 0;
	move_speed = 5.0;
	invert_y_axis = true;
	is_ortho = false;
	sensitivity = 0.15f;
	type = 0;
}


void Camera::move_view( int32 dx, int32 dy )
{
	yaw += (dx * sensitivity);
	pitch += (dy * sensitivity);
	
	// clamp yaw and pitch values
	if ( yaw > 360 )
	{
		yaw -= 360;
	}
	else if ( yaw < -360 )
	{
		yaw += 360;
	}
	
	if ( pitch > 360 )
	{
		pitch -= 360;
	}
	else if ( pitch < -360 )
	{
		pitch += 360;
	}

	// update camera matrix
	update_view();
}

void Camera::move_along_vector( const glm::vec3 & v, real dt )
{
	if ( type == 0 )
	{
		pos = pos + (v * (move_speed*dt));
	}
	else if ( type == 1 )
	{
		target_lookatOffset = target_lookatOffset + (v*move_speed*dt);
	}
	update_view();
}

void Camera::move_left( real dt )
{
	move_along_vector( side, -dt );
}

void Camera::move_right( real dt )
{
	move_along_vector( side, dt );
}

void Camera::move_forward( real dt )
{
	glm::vec3 v;
	
	if ( is_ortho )
	{
		v = glm::vec3( 0, 1, 0 );
		
		if ( invert_y_axis )
		{
			v[1] = -v[1];
		}
	}
	else
	{
		v = view;
	}
	
	move_along_vector( v, dt );
}

void Camera::move_backward( real dt )
{
	glm::vec3 v;
	
	if ( is_ortho )
	{
		v = glm::vec3( 0, 1, 0 );
		
		if ( invert_y_axis )
		{
			v[1] = -v[1];
		}
	}
	else
	{
		v = view;
	}
	
	move_along_vector( v, -dt );
}


void Camera::update_view()
{
	float _yaw = DegToRad( yaw );
	float _pitch;
	
	if ( invert_y_axis )
	{
		_pitch = DegToRad( -pitch );
	}
	else
	{
		_pitch = DegToRad( pitch );
	}
	
	float sy = sin(_yaw);
	float cy = cos(_yaw);
	float sp = sin(_pitch);
	float cp = cos(_pitch);
	
	glm::vec3 forward( sy, 0, -cy );
	side = glm::vec3( cy, 0, sy );
	
	
	view[0] = sy * cp;
	view[1] = sp;
	view[2] = -cy * cp;
	
	view = glm::normalize(view);

	glm::vec3 up( 0, 1, 0 );
	
	glm::vec3 world_pos = pos;
	if ( type == 0 )
	{
		glm::vec3 target = world_pos + view;
		matCam = glm::lookAt(world_pos, target, up );
		eye_position = world_pos;
	}
	else if ( type == 1 )
	{
		glm::mat4 inv_rotation;
		glm::mat4 inv_translation;
		glm::mat4 pivot;
		glm::mat4 inv_pivot;
		glm::vec3 inv_position;
		glm::vec3 target_position = world_pos;
		inv_position = target_position;
		
		// setup inverse rotation matrix for 'view' and 'up' vectors
		inv_rotation = glm::lookAt( glm::vec3(), view, up );
		
		// setup pivot, and inverse pivot matrices
		pivot = glm::translate( glm::mat4(1.0), target_position );
		inv_pivot = glm::translate( glm::mat4(1.0), -inv_position );
		
		// add an arbitrary viewing offset
		inv_position = target_position + target_lookatOffset;
		
		inv_translation = glm::translate( inv_translation, -inv_position );
		
		// target camera with position + orientation
		matCam = inv_pivot * inv_rotation * pivot * inv_translation;
		
		// now calculate eye pos
		eye_position = view * target_lookatOffset.z;
		
		// eye is Cam -> Target; so invert it. It now becomes a vector from the origin to the eye position
		eye_position = -eye_position + target_position;
	}
	
	matCamProj = matProj * matCam;
}

void Camera::set_absolute_position( const glm::vec3 & position )
{
	pos = position;
	update_view();
}


void Camera::perspective( real fovy, int32 width, int32 height, real nearz, real farz )
{
	matProj = glm::perspective(fovy, (width/(float)height), nearz, farz );
	is_ortho = false;
}

void Camera::ortho( real left, real right, real bottom, real top, real nearz, real farz )
{
	matProj = glm::ortho( left, right, bottom, top, nearz, farz );
	is_ortho = true;
}
