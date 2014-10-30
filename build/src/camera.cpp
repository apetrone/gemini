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
#include <gemini/typedefs.h>
#include <slim/xlog.h>
#include "camera.h"


// --------------------------------------------------------
// Camera
// --------------------------------------------------------
Camera::Camera(CameraType _type)
{
	yaw = pitch = 0;
	move_speed = 0.5f;
	invert_y_axis = true;
	is_ortho = false;
	sensitivity = 0.15f;
	type = _type;
}


void Camera::move_view( int32_t dx, int32_t dy )
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
	pos = pos + (v * (move_speed*dt));
	
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
	float _yaw = mathlib::degrees_to_radians(yaw);
	float _pitch;
	
	if ( invert_y_axis )
	{
		_pitch = mathlib::degrees_to_radians(-pitch);
	}
	else
	{
		_pitch = mathlib::degrees_to_radians(pitch);
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
	if ( type == FIRST_PERSON )
	{
		glm::vec3 target = world_pos + view;
		inverse_rotation = glm::lookAt(glm::vec3(), view, up);
		matCam = glm::lookAt(world_pos, target, up );
		eye_position = world_pos;
		
		inverse_world_transform = matCam;
	}
	else if ( type == TARGET )
	{
//		glm::mat4 inv_rotation;
		glm::mat4 inv_translation;
		glm::mat4 pivot;
		glm::mat4 inv_pivot;
		glm::vec3 inv_position;
		glm::vec3 target_position = world_pos;
		inv_position = target_position;
		
		// setup inverse rotation matrix for 'view' and 'up' vectors
		inverse_rotation = glm::lookAt( glm::vec3(), view, up );
		
		
		glm::vec3 pivot_point = target_lookatOffset;
		
		// setup pivot, and inverse pivot matrices
		inv_pivot = glm::translate( glm::mat4(1.0), -pivot_point );
		pivot = glm::translate( glm::mat4(1.0), pivot_point );

		
		// add an arbitrary viewing offset
		inv_position = target_position + target_lookatOffset;

		inv_translation = glm::translate( inv_translation, -inv_position );
		
		// target camera with position + orientation
		matCam = inv_pivot * inverse_rotation * pivot * inv_translation;
		
		// now calculate eye pos
		eye_position = view * target_lookatOffset.z;
		
		// eye is Cam -> Target; so invert it. It now becomes a vector from the origin to the eye position
		eye_position = -eye_position + target_position;
		
		inverse_world_transform = matCam;
	}
}

void Camera::set_absolute_position( const glm::vec3 & position )
{
	pos = position;
	update_view();
}


void Camera::perspective( real fovy, int32_t width, int32_t height, real nearz, real farz )
{
	// This MUST be greater than 0, otherwise the view will be inverted or something.
	// Basically, you won't see anything.
	assert( nearz > 0.0f );
	this->fovy = fovy;
	this->aspect_ratio = (float)(width/(float)height);
	this->near_clip = nearz;
	this->far_clip = farz;
	matProj = glm::perspective(glm::radians(fovy), (width/(float)height), nearz, farz );
	is_ortho = false;
}

void Camera::ortho( real left, real right, real bottom, real top, real nearz, real farz )
{
	this->near_clip = nearz;
	this->far_clip = farz;
	this->aspect_ratio = 1.0f;
	matProj = glm::ortho( left, right, bottom, top, nearz, farz );
	is_ortho = true;
}