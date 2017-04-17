// -------------------------------------------------------------
// Copyright (C) 2017- Adam Petrone
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
#include "camera2.h"

#include <core/mathlib.h>
#include <core/typedefs.h>
#include <core/interpolation.h>
#include <core/logging.h>

#include <renderer/debug_draw.h>

#include <sdk/game_api.h>
#include <sdk/physics_api.h>
#include <sdk/utils.h>

const float MIN_CAMERA_DISTANCE = 0.25f;

// --------------------------------------------------------
// QuaternionFollowCamera
// --------------------------------------------------------

#if 0
	- need to independently control yaw and pitch (right analog stick)
	- need to rotate back to 'ideal' position
	- What if we stored a view vector?
#endif


// Quaternion interpolation time.
const float kInterpolationTime = 1.0f;

// Time to wait while moving before initiating auto-orient.
const float kAutoOrientWaitTime = 0.5f;

const glm::vec3 YUP_DIRECTION(0.0f, 1.0f, 0.0f);

QuaternionFollowCamera::QuaternionFollowCamera()
	: yaw(0.0f)
	, pitch(0.0f)
	, dbg_distance_to_target("distance_to_target", &distance_to_target.current_value)
	, dbg_horizontal_offset("horizontal_offset", &horizontal_offset.current_value)
	, dbg_position("position", &position)
	, dbg_world_position("world_position", &world_position)
{
	position = glm::vec3(0.0f, 3.0f, 3.0f);
	target_facing_direction = glm::vec3(0.0f, 0.0f, -1.0f);

	move_sensitivity = glm::vec2(25.0f, 25.0f);

	const float DESIRED_DISTANCE = 4.0f;
	distance_to_target.set(DESIRED_DISTANCE, 0.0f);
	desired_distance_to_target = DESIRED_DISTANCE;
	minimum_distance = MIN_CAMERA_DISTANCE;

	field_of_view.set(70.0f, 0.0f);
	vertical_offset.set(0.0f, 0.0f);
	horizontal_offset.set(0.0f, 0.0f);
	desired_horizontal_offset = 0.0f;

	view_moved = 0;

	camera_direction = glm::vec3(0.0f, 0.0f, -1.0f);
	camera_right = glm::vec3(1.0f, 0.0f, 0.0f);
	interpolation_time = 0.0f;

	auto_orient_seconds = kAutoOrientWaitTime;
	auto_orienting = 0;

	//pitch = 0.0f;
	//pitch_min = 45.0f;
	//pitch_max = 80.0f;

	cam_vertices[0] = glm::vec3(-0.5f, 0.0f, 0.0f);
	cam_vertices[1] = glm::vec3(0.5f, 0.0f, 0.0f);
	cam_vertices[2] = glm::vec3(0.5f, 0.5f, 0.0f);
	cam_vertices[3] = glm::vec3(0.5f, 0.5f, 0.0f);
	cam_vertices[4] = glm::vec3(-0.5f, 0.5f, 0.0f);
	cam_vertices[5] = glm::vec3(-0.5f, 0.0f, 0.0f);
}

QuaternionFollowCamera::~QuaternionFollowCamera()
{
}

glm::vec3 QuaternionFollowCamera::get_origin() const
{
	glm::mat4 view_matrix = compute_view_matrix();
	return world_position + glm::vec3(view_matrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
}

glm::vec3 QuaternionFollowCamera::get_target() const
{
	return camera_direction;
}

glm::vec3 QuaternionFollowCamera::get_up() const
{
	return glm::vec3(0.0f, 1.0f, 0.0f);
}

glm::vec3 QuaternionFollowCamera::get_right() const
{
	return camera_right;
}

float QuaternionFollowCamera::get_fov() const
{
	return field_of_view.value();
}

void QuaternionFollowCamera::move_view(float yaw_delta, float pitch_delta)
{
	// flag view moved this tick
	view_moved = 1;

#if 0
	float old_pitch = pitch;
	// need to clamp the pitch until we make this a spline
	float new_pitch = (pitch + -pitch_delta);
	LOGV("new pitch: %2.2f\n", new_pitch);
	//new_pitch = glm::clamp(new_pitch, pitch_min, pitch_max);
	pitch = new_pitch;

	float delta_pitch = (old_pitch - pitch);
	LOGV("pitch: %2.2f\n", pitch);
#endif

	// yaw and pitch can be inverted here depending on axis inversions.
	float scaled_yaw = (move_sensitivity.x * yaw_delta);
	float scaled_pitch = (move_sensitivity.y * -pitch_delta);

	const float MIN_PITCH = -85.0f;
	const float MAX_PITCH = 35.0f;

	if (pitch + scaled_pitch <= MIN_PITCH)
	{
		scaled_pitch = MIN_PITCH - pitch;
	}
	else if (pitch + scaled_pitch >= MAX_PITCH)
	{
		scaled_pitch = MAX_PITCH - pitch;
	}

	yaw += scaled_yaw;
	pitch += scaled_pitch;

	glm::quat rotation = mathlib::orientation_from_yaw_pitch(scaled_yaw, scaled_pitch, YUP_DIRECTION, camera_right);
	camera_direction = glm::normalize(mathlib::rotate_vector(camera_direction, rotation));

	update_view_orientation();
}

void QuaternionFollowCamera::set_yaw_pitch(float yaw, float pitch)
{
	if (interpolation_time > 0.0f)
		return;

	glm::quat rotation = mathlib::orientation_from_yaw_pitch(yaw, -pitch, YUP_DIRECTION, camera_right);
	camera_direction = glm::normalize(mathlib::rotate_vector(glm::vec3(0.0f, 0.0f, -1.0f), rotation));
	camera_right = glm::normalize(glm::cross(camera_direction, YUP_DIRECTION));

	position = (-camera_direction * distance_to_target.current_value);
}

void QuaternionFollowCamera::tick(float step_interval_seconds)
{
	collision_correct();

	if (view_moved == 0)
	{
		// decrement the wait time
		if (auto_orienting == 0)
		{
			//LOGV("decrementing auto orient time\n");
			//auto_orient_seconds -= step_interval_seconds;

			if (auto_orient_seconds <= 0.0f)
			{
				// start auto-orienting...
				auto_orienting = 1;

				reset_view();
				auto_orient_seconds = kAutoOrientWaitTime;
			}
		}
	}
	else
	{
		// reset the wait timer
		auto_orient_seconds = kAutoOrientWaitTime;

		interpolation_time = 0.0f;
		auto_orienting = 0;
	}

	position = (-camera_direction * distance_to_target.current_value) + get_rotated_pivot_offset();

	//debugdraw::camera(
	//	world_position + position,
	//	glm::normalize(camera_direction), // facing direction
	//	0.0f
	//);

	//debugdraw::line(
	//	target_position,
	//	(target_position + (target_facing_direction * 1.0f)),
	//	gemini::Color(0.0f, 1.0f, 1.0f)
	//);

	//debugdraw::line(
	//	target_position,
	//	(target_position + (target_facing_direction * 1.0f)),
	//	gemini::Color(0.0f, 1.0f, 1.0f)
	//);

	//debugdraw::line(
	//	target_position,
	//	(target_position + (camera_direction * 1.0f)),
	//	gemini::Color(0.0f, 0.0f, 1.0f)
	//);

	//debugdraw::line(
	//	target_position,
	//	(target_position + (camera_right * 1.0f)),
	//	gemini::Color(1.0f, 0.0f, 0.0f)
	//);

	// adjust fov
	field_of_view.update(step_interval_seconds);
	vertical_offset.update(step_interval_seconds);
	horizontal_offset.update(step_interval_seconds);
	distance_to_target.update(step_interval_seconds);

	if ((view_moved == 0) && (auto_orienting == 1) && (interpolation_time > 0.0f))
	{
		float t = (kInterpolationTime - interpolation_time) / kInterpolationTime;
		glm::quat rot = gemini::slerp(glm::quat(1.0f, 0.0f, 0.0f, 0.0f), interpolation_rotation, t);
		camera_direction = mathlib::rotate_vector(interpolation_vector, rot);

		collision_correct();

		interpolation_time -= step_interval_seconds;

		if (interpolation_time <= 0.0f)
		{
			auto_orienting = 0;
			auto_orient_seconds = kAutoOrientWaitTime;
		}
	}

	view_moved = 0;
}

void QuaternionFollowCamera::set_fov(float new_fov)
{
	field_of_view.set(new_fov, 0.0f);
}

glm::vec3 QuaternionFollowCamera::perform_raycast(const glm::vec3& start, const glm::vec3& direction, const gemini::Color& color, float max_distance, bool* hit_object)
{
	glm::vec3 point;
	gemini::physics::RaycastInfo result = gemini::physics::instance()->raycast(start, direction, max_distance, nullptr);
	if (result.object)
	{
		if (hit_object)
		{
			*hit_object = true;
		}

		point = result.hit;
	}
	else
	{
		if (hit_object)
		{
			*hit_object = false;
		}
		point = (direction * max_distance);
	}

	return point;
}

void QuaternionFollowCamera::collision_correct()
{
	const float COLLISION_RADIUS = 0.10f;
	const float SPHERE_RADIUS = 0.125f;
	glm::vec3 raycast_result;

	// 1. Try to raycast from the pivot offset to the world first.
	glm::vec3 desired_pivot_offset(desired_horizontal_offset, 0.0f, 0.0f);
	float offset_length = glm::length(desired_pivot_offset);
	glm::vec3 offset_vector;
	if (offset_length > 0.0f)
	{
		offset_vector = desired_pivot_offset;

		// rotate offset_vector to be in the coordinate frame of the camera
		offset_vector = mathlib::rotate_vector(offset_vector, orientation);

		debugdraw::point(world_position + offset_vector, gemini::Color(1.0f, 1.0f, 1.0f), 0.1f, 0.2f);
		glm::vec3 offset_direction = glm::normalize(offset_vector);
		glm::vec3 test_pivot_point;

		/*debugdraw::line(world_position, world_position + (offset_length * offset_direction), gemini::Color(1.0f, 0.0f, 0.0f), 0.5f);*/

		gemini::physics::RaycastInfo result = gemini::physics::instance()->raycast(
			world_position,
			offset_direction,
			offset_length,
			nullptr
		);

		//debugdraw::sphere(world_position, gemini::Color(0.5f, 0.5f, 0.5f), SPHERE_RADIUS, 0.5f);

		float test_len;

		if (result.object)
		{
			glm::vec3 corrected_position = result.hit;// +(-offset_direction * COLLISION_RADIUS);
			test_pivot_point = (corrected_position - world_position);

			//debugdraw::sphere(corrected_position, gemini::Color(1.0f, 0.5f, 0.0f), SPHERE_RADIUS, 0.5f);
			//debugdraw::line(world_position, corrected_position, gemini::Color(1.0f, 0.0f, 0.0f), 0.5f);
		}
		else
		{
			test_pivot_point = offset_length * offset_direction;
			//debugdraw::sphere(world_position + (offset_direction * offset_length), gemini::Color(0.0f, 1.0f, 0.0f), SPHERE_RADIUS, 0.5f);
			//debugdraw::line(world_position, world_position + (offset_direction * offset_length), gemini::Color(0.0f, 1.0f, 0.0f), 0.5f);
		}

		test_len = glm::length(test_pivot_point);
		//LOGV("length is: %2.2f\n", test_len);

		offset_vector = test_len * glm::normalize(desired_pivot_offset);

		horizontal_offset.target_value = offset_vector.x;
	}
	else
	{
		horizontal_offset.target_value = desired_pivot_offset.x;
	}

	// 2. Raycast from the pivot to the desired target distance.

#if 1

	// raycast to desired distance from pivot
	bool hit_object = false;
	glm::vec3 far_point = perform_raycast(
		world_position + offset_vector,
		-camera_direction,
		gemini::Color(0.0f, 1.0f, 0.0f),
		distance_to_target.target_value,
		&hit_object
	);

	debugdraw::sphere(world_position + offset_vector, gemini::Color(0.5f, 0.5f, 0.5f), SPHERE_RADIUS, 0.5f);

	uint32_t distance_truncated = false;
	float desired_distance = glm::length(far_point);

	//debugdraw::line(target_position, target_position + far_point, gemini::Color(1.0f, 0.0f, 0.0f), 3.0f);

	if (desired_distance < MIN_CAMERA_DISTANCE)
	{
		desired_distance = MIN_CAMERA_DISTANCE;
		distance_truncated = 1;
	}


	//glm::vec3 furthest_distance;
	gemini::physics::RaycastInfo result = gemini::physics::instance()->raycast(
		world_position + offset_vector,
		-camera_direction,
		distance_to_target.target_value,
		nullptr
	);
	if (result.object)
	{
		glm::vec3 corrected_position = result.hit;// +(camera_direction * 0.25f);

		debugdraw::sphere(corrected_position, gemini::Color(1.0f, 0.5f, 0.0f), SPHERE_RADIUS, 0.5f);
		desired_distance = glm::length(corrected_position - (world_position + offset_vector));
		distance_truncated = 1;
	}
	else
	{
		debugdraw::sphere(world_position + offset_vector + (-camera_direction * desired_distance_to_target), gemini::Color(0.0f, 1.0f, 0.0f), SPHERE_RADIUS, 0.5f);
		desired_distance = desired_distance_to_target;
	}

	if (desired_distance < minimum_distance)
	{
		desired_distance = minimum_distance;
		distance_truncated = 1;
	}

	if (distance_truncated)
	{
		// snap to the new distance
		distance_to_target.set(desired_distance, 0.0f);
	}
	else
	{
		distance_to_target.target_value = desired_distance;
	}
#endif
}

void QuaternionFollowCamera::set_target_direction(const glm::vec3& direction)
{
	target_facing_direction = glm::normalize(direction);
}

glm::vec3 QuaternionFollowCamera::get_target_direction() const
{
	return target_facing_direction;
}

glm::vec3 QuaternionFollowCamera::get_camera_direction() const
{
	return camera_direction;
}

void QuaternionFollowCamera::reset_view()
{
	glm::vec3 new_direction = target_facing_direction;

	float difference = glm::length(new_direction - camera_direction);
	if (difference > FLT_EPSILON)
	{
		glm::quat rot = mathlib::orientation_from_vectors(glm::normalize(camera_direction), glm::normalize(new_direction));

		interpolation_rotation = rot;
		//camera_direction = mathlib::rotate_vector(new_direction, rot);

		interpolation_time = kInterpolationTime;
		interpolation_vector = camera_direction;
	}
	else
	{
		//LOGV("skipping auto-orient; difference too small\n");
		auto_orienting = 0;
	}
}

glm::vec3 QuaternionFollowCamera::get_eye_position() const
{
	glm::mat4 view_matrix = compute_view_matrix();
	return world_position + glm::vec3(view_matrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
}

void QuaternionFollowCamera::set_follow_distance(float target_distance)
{
	desired_distance_to_target = target_distance;
	distance_to_target.set(target_distance, 1.0f);
}

void QuaternionFollowCamera::set_view(const glm::vec3& view_direction)
{
	camera_direction = view_direction;

	float new_yaw = glm::dot(glm::vec3(1.0f, 0.0f, 0.0f), view_direction);
	float new_pitch = glm::dot(glm::vec3(0.0f, 1.0f, 0.0f), view_direction);

	yaw = glm::degrees(new_yaw);
	pitch = glm::degrees(new_pitch);

	update_view_orientation();
}

void QuaternionFollowCamera::update_view_orientation()
{
	glm::quat qyaw;
	glm::quat qpitch;
	qyaw = glm::angleAxis(glm::radians(yaw), YUP_DIRECTION);

	camera_right = glm::normalize(glm::cross(camera_direction, YUP_DIRECTION));

	qpitch = glm::angleAxis(glm::radians(pitch), camera_right);
	orientation = glm::normalize(qpitch * qyaw);

	collision_correct();
}

void QuaternionFollowCamera::set_target_fov(float new_fov)
{
	field_of_view.set(new_fov, 0.15f);
}

float QuaternionFollowCamera::get_vertical_offset() const
{
	return vertical_offset.value();
}

void QuaternionFollowCamera::set_vertical_offset(float new_offset)
{
	vertical_offset.set(new_offset, 0.15f);
}

float QuaternionFollowCamera::get_horizontal_offset() const
{
	return horizontal_offset.value();
}

void QuaternionFollowCamera::set_horizontal_offset(float new_offset)
{
	horizontal_offset.set(new_offset, 0.55f);
	desired_horizontal_offset = new_offset;
}

float QuaternionFollowCamera::get_distance_from_pivot() const
{
	return distance_to_target.current_value;
}

void QuaternionFollowCamera::set_minimum_distance(float min_distance)
{
	minimum_distance = min_distance;
}

void QuaternionFollowCamera::set_initial_state(const gemini::CameraState& state)
{
	distance_to_target.set(state.distance_from_pivot, 0.0f);
	desired_distance_to_target = state.distance_from_pivot;
}

void QuaternionFollowCamera::get_current_state(gemini::CameraState& state)
{
	state.world_position = world_position;
	state.position = get_origin();
	state.rotation = get_rotation();
	state.view = get_camera_direction();
	state.distance_from_pivot = get_distance_from_pivot();
	state.field_of_view = get_fov();
	state.vertical_offset = get_vertical_offset();
	state.horizontal_offset = get_horizontal_offset();
}

void QuaternionFollowCamera::set_world_position(const glm::vec3& new_world_position)
{
	world_position = new_world_position;
}

glm::mat4 QuaternionFollowCamera::compute_view_matrix() const
{
	glm::mat4 view;

	const glm::vec3 local_offset = mathlib::rotate_vector(glm::vec3(horizontal_offset.current_value, vertical_offset.current_value, 0.0f), orientation);
	const glm::mat4 world_tx = glm::translate(glm::mat4(1.0f), position + local_offset);
	glm::mat4 rot = glm::toMat4(orientation);
	view = glm::translate(glm::mat4(1.0f), local_offset) * world_tx * rot;

	return view;
}

glm::vec3 QuaternionFollowCamera::get_rotated_pivot_offset() const
{
	glm::vec3 offset_vector(horizontal_offset.current_value, vertical_offset.current_value, 0.0f);
	return mathlib::rotate_vector(offset_vector, orientation);
} // get_rotated_pivot_offset