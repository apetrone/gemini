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
#include "follow_camera.h"

#include <core/mathlib.h>
#include <core/typedefs.h>
#include <core/interpolation.h>
#include <core/logging.h>

#include <renderer/debug_draw.h>

#include <sdk/utils.h>

// --------------------------------------------------------
// FollowCamera
// --------------------------------------------------------
void FollowCamera::update_desired_position(const glm::vec3& desired_direction)
{
	desired_position = (desired_direction * follow_distance) + target_position;
	//view = glm::normalize(position - target_position);
}

void FollowCamera::direction_to_spherical(const glm::vec3& direction)
{
	float rho, theta, phi;
	mathlib::cartesian_to_spherical(direction, rho, theta, phi);

	// We store these values in degrees: convert them.
	yaw = glm::degrees(theta);

	// clamp the pitch range
	float new_pitch = glm::degrees(phi);
	new_pitch = glm::clamp(new_pitch, pitch_min, pitch_max);
	pitch = new_pitch;
}

const glm::vec3 position_offset(0.0f, 3.0f, 3.0f);

FollowCamera::FollowCamera()
{
	pitch_min = 20.0f;
	pitch_max = 160.0f;

	target_position = glm::vec3(0.0f, 0.0f, 0.0f);
	position = glm::vec3(0.0f, 3.0f, 3.0f);
	desired_position = position;
	view = glm::vec3(0.0f, 0.0f, -1.0f);

	follow_distance = 5.0f;

	view_moved = 0;

	// should be CW offset from positive X axis
	yaw = 0.0f;

	// should be CW offset from positive y axis (up)
	pitch = 90.0f;

	//update_desired_position(glm::vec3(0.0f, 0.0f, -1.0f));
}

FollowCamera::~FollowCamera()
{
}

glm::vec3 FollowCamera::get_origin() const
{
	return position;
}

glm::vec3 FollowCamera::get_target() const
{
	return target_position;
}

glm::vec3 FollowCamera::get_up() const
{
	return glm::vec3(0.0f, 1.0f, 0.0f);
}

glm::vec3 FollowCamera::get_right() const
{
	return glm::vec3(1.0f, 0.0f, 0.0f);
}

float FollowCamera::get_fov() const
{
	return 50.0f;
}

void FollowCamera::move_view(float yaw_delta, float pitch_delta)
{
	yaw += -yaw_delta;

	// need to clamp the pitch until we make this a spline
	float new_pitch = (pitch + -pitch_delta);
	new_pitch = glm::clamp(new_pitch, pitch_min, pitch_max);
	pitch = new_pitch;

	glm::vec3 new_position;
	glm::vec3 direction;
	mathlib::spherical_to_cartesian(1.0f, glm::radians(yaw), glm::radians(pitch), direction);
	update_desired_position(direction);

	// This either syncs the positions immediately, or allows it to be spring driven.
	//position = desired_position;

	// flag view moved this tick
	view_moved = 1;

	// TODO: update the view orientation?
}

void FollowCamera::set_yaw_pitch(float _yaw, float _pitch)
{
	yaw = _yaw;
	pitch = _pitch;
}

void FollowCamera::tick(float step_interval_seconds)
{
	if (view_moved == 0)
	{
		// The view didn't move this tick.
		// Calculate a new desired position.

		glm::vec3 target_to_camera = (position - target_position);
		glm::vec3 direction = glm::normalize(target_to_camera);

		update_desired_position(direction);

		direction_to_spherical(direction);
	}

	view_moved = 0;

	// critically damped spring system
	// t = (kD / (2 * sqrt(kS)))
	const float kDamping = 12.0f;
	const float kSpring = 36.0f;
	glm::vec3 displacement = desired_position - position;
	glm::vec3 spring_acceleration = (kSpring * displacement) - (kDamping * velocity);

	// TODO: This feels a bit funky because of lerp -- when the camera returns
	// to rest, it seems to extend further than the lerp'd position (which was between positions, prior).
	position += velocity * step_interval_seconds;
	velocity += spring_acceleration * step_interval_seconds;

#if 0
	renderer::debugdraw::point(
		desired_position,
		gemini::Color(0.0f, 0.0f, 1.0f),
		0.25f
	);
#endif

	glm::vec3 direction;
	mathlib::spherical_to_cartesian(
		1.0f,
		glm::radians(yaw),
		glm::radians(pitch),
		direction
	);

#if 0
	debugdraw::line(
		target_position,
		(target_position + (view * 1.0f)),
		gemini::Color(0.0f, 0.0f, 1.0f)
	);
#endif

	debugdraw::camera(
		position,
		view,
		0.0f
	);

#if 0
	debugdraw::point(target_position, gemini::Color(1.0f, 0.0f, 1.0f), 0.1f);
#endif
}

//void FollowCamera::set_target_position(const glm::vec3& target_worldspace_position)
//{
//	target_position = target_worldspace_position;
//	// TODO: update camera position
//
//	update_desired_position(view);
//}

void FollowCamera::set_target_direction(const glm::vec3& direction)
{
}

glm::vec3 FollowCamera::get_target_direction() const
{
	return glm::vec3(0.0f, 0.0f, 0.0f);
}

void FollowCamera::reset_view()
{
	velocity = glm::vec3(0.0f);
	glm::vec3 direction(0.0f, 0.0f, -1.0f);
	update_desired_position(direction);
	mathlib::spherical_to_cartesian(1.0f, glm::radians(yaw), glm::radians(pitch), direction);

	yaw = 0.0f;
	pitch = 90.0f;
}
