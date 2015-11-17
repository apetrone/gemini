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
#include "camera.h"

#include <core/mathlib.h>
#include <core/typedefs.h>

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
	mathlib::basis_vectors_from_pitch_yaw(pitch, yaw, side, view/*, invert_y_axis*/);

	glm::vec3 up( 0, 1, 0 );

	glm::vec3 world_pos = pos;
	if ( type == FIRST_PERSON )
	{
		glm::vec3 target = world_pos + view;
		inverse_rotation = glm::lookAt(glm::vec3(), view, up);
		modelview = glm::lookAt(world_pos, target, up );
		eye_position = world_pos;
		inverse_world_transform = modelview;
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


		glm::vec3 pivot_point = target_offset;

		// setup pivot, and inverse pivot matrices
		inv_pivot = glm::translate( glm::mat4(1.0), -pivot_point );
		pivot = glm::translate( glm::mat4(1.0), pivot_point );


		// add an arbitrary viewing offset
		inv_position = target_position + target_offset;

		inv_translation = glm::translate( inv_translation, -inv_position );

		// target camera with position + orientation
		modelview = inv_pivot * inverse_rotation * pivot * inv_translation;

		// now calculate eye pos
		eye_position = view * target_offset.z;

		// eye is Cam -> Target; so invert it. It now becomes a vector from the origin to the eye position
		eye_position = -eye_position + target_position;

		inverse_world_transform = modelview;
	}
}

void Camera::set_position(const glm::vec3& position)
{
	pos = position;
	if (type == Camera::FIRST_PERSON)
	{
		eye_position = position;
	}
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
	projection = glm::perspective(glm::radians(fovy), (width/(float)height), nearz, farz );
	is_ortho = false;
}

void Camera::ortho( real left, real right, real bottom, real top, real nearz, real farz )
{
	this->near_clip = nearz;
	this->far_clip = farz;
	this->aspect_ratio = 1.0f;
	projection = glm::ortho( left, right, bottom, top, nearz, farz );
	is_ortho = true;
}
