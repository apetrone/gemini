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
const float SPHERE_COLLISION_RADIUS = 0.25f;

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
	desired_pivot_offset = glm::vec2(0.0f, 0.0f);

	view_moved = 0;

	camera_direction = glm::vec3(0.0f, 0.0f, -1.0f);
	camera_right = glm::vec3(1.0f, 0.0f, 0.0f);
	interpolation_time = 0.0f;

	auto_orient_seconds = kAutoOrientWaitTime;
	auto_orienting = 1;

	collision_shape = gemini::physics::instance()->create_sphere(0.125f);
	uint16_t collision_mask = (gemini::physics::StaticFilter | gemini::physics::KinematicFilter);
	collision_object = gemini::physics::instance()->create_kinematic_object(collision_shape, glm::vec3(0.0f, 0.0f, 0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), collision_mask);
}

QuaternionFollowCamera::~QuaternionFollowCamera()
{
	gemini::physics::instance()->destroy_object(collision_object);
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
	collision_correct(0.0f);
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
	//float a = glm::length(position);

	//float xpos = (world_position + position).x;
	//assert(xpos < 5.02f);

	// I broke Auto-Orienting at some point.
#if 0
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


	if ((view_moved == 0) && (auto_orienting == 1) && (interpolation_time > 0.0f))
	{
		float t = (kInterpolationTime - interpolation_time) / kInterpolationTime;
		glm::quat rot = gemini::slerp(glm::quat(1.0f, 0.0f, 0.0f, 0.0f), interpolation_rotation, t);
		camera_direction = mathlib::rotate_vector(interpolation_vector, rot);

		collision_correct(step_interval_seconds);

		interpolation_time -= step_interval_seconds;

		if (interpolation_time <= 0.0f)
		{
			auto_orienting = 0;
			auto_orient_seconds = kAutoOrientWaitTime;
		}
	}

	view_moved = 0;
#endif
}

void QuaternionFollowCamera::set_fov(float new_fov)
{
	field_of_view.set(new_fov, 0.0f);
}

glm::vec3 QuaternionFollowCamera::perform_raycast(const glm::vec3& start, const glm::vec3& direction, float max_distance, bool* hit_object)
{
#if 0
	gemini::physics::SweepTestResult result = gemini::physics::instance()->sweep(collision_object, collision_shape, start, start + (direction * max_distance), glm::radians(0.0f), -direction);

	glm::vec3 point;
	if (result.hit_items() > 0)
	{
		if (hit_object)
		{
			*hit_object = 1;
		}
		point = result.hit_point_world;
		//LOGV("hit: %2.2f, %2.2f, %2.2f\n", point.x, point.y, point.z);
	}
	else
	{
		if (hit_object)
		{
			*hit_object = 0;
		}
		point = start + (direction * max_distance);
		//LOGV("no hit: %2.2f, %2.2f, %2.2f\n", point.x, point.y, point.z);
	}

	return point;
#else

	glm::vec3 point;
	gemini::physics::RaycastInfo result = gemini::physics::instance()->raycast(start, direction, max_distance, collision_object);
	if (result.object)
	{
		if (hit_object)
		{
			*hit_object = true;
		}

		point = result.hit;
		//LOGV("hit: %2.2f, %2.2f, %2.2f\n", point.x, point.y, point.z);
	}
	else
	{
		if (hit_object)
		{
			*hit_object = false;
		}
		point = start + (direction * max_distance);
		//LOGV("no hit: %2.2f, %2.2f, %2.2f\n", point.x, point.y, point.z);
	}

	return point;
#endif
}

void QuaternionFollowCamera::collision_correct(float step_interval_seconds)
{
	field_of_view.update(step_interval_seconds);
	vertical_offset.update(step_interval_seconds);
	horizontal_offset.update(step_interval_seconds);
	distance_to_target.update(step_interval_seconds);

	//collision_pivot_offset();
	collision_follow_distance();
	position = (-glm::normalize(camera_direction) * distance_to_target.current_value);

	collision_object->set_world_transform(world_position + position + get_rotated_pivot_offset(), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
}

void QuaternionFollowCamera::collision_pivot_offset()
{
	glm::vec3 raycast_result;

	// 1. Try to raycast from the pivot offset to the world first.
	glm::vec3 pivot_offset(horizontal_offset.target_value, 0.0f, 0.0f);
	float offset_length = glm::length(pivot_offset);
	glm::vec3 offset_vector;

	bool hit_object = false;

	const float MINIMUM_PIVOT_OFFSET = 0.3f;

	if (offset_length > 0.0f)
	{
		offset_vector = pivot_offset;

		// rotate offset_vector to be in the coordinate frame of the camera
		offset_vector = mathlib::rotate_vector(offset_vector, orientation);

		glm::vec3 offset_direction = glm::normalize(offset_vector);
		glm::vec3 test_pivot_point;

		const float DEBUG_SPHERE_RADIUS = 0.0125f;
		/*debugdraw::line(world_position, world_position + (offset_length * offset_direction), gemini::Color(1.0f, 0.0f, 0.0f), 0.5f);*/

		glm::vec3 result = perform_raycast(
			world_position,
			offset_direction,
			offset_length + MINIMUM_PIVOT_OFFSET,
			&hit_object
		);

		gemini::physics::SweepTestResult sweep = gemini::physics::instance()->sweep(
			collision_object,
			collision_shape,
			world_position,
			world_position + (offset_direction * offset_length),
			glm::radians(0.0f),
			glm::vec3(0.0f, 0.0f, 0.0f)
		);

		if (sweep.hit_items() > 0)
		{
			debugdraw::sphere(sweep.hit_point_world, gemini::Color(1.0f, 0.0f, 0.0f), DEBUG_SPHERE_RADIUS, 0.5f);
		}

		//debugdraw::sphere(world_position, gemini::Color(0.5f, 0.5f, 0.5f), SPHERE_RADIUS, 0.5f);

		float test_len;

		// There are three cases to handle here:
		// 1. Raycast hit with enough distance for offset
		// 2. Raycast hit without enough distance for pivot
		// 3. The raycast hit nothing

		if (hit_object)
		{
			glm::vec3 player_to_hit = result - world_position;

			test_len = glm::length(player_to_hit);
			LOGV("hit length: %2.2f\n", test_len);

			glm::vec3 adjust;

			if (test_len > offset_length)
			{
				// 1. Raycast hit with enough distance for offset; clamp length.
				test_len = offset_length;
			}
			else
			{
				// 2. Raycast hit without enough distance for pivot
				//else
				//{
				//	test_len -= MINIMUM_PIVOT_OFFSET;
				//}

				float ratio = (MINIMUM_PIVOT_OFFSET / test_len);
				float pct = 1.0 - ratio;

				test_len = (test_len * pct);
				// scale back by the normal;

				//const glm::vec3 normal(-1.0f, 0.0f, 0.0f);
				//player_to_hit = (result + (normal * MINIMUM_PIVOT_OFFSET)) - world_position;
				//test_len = glm::length(player_to_hit);

				//adjust = (normal * MINIMUM_PIVOT_OFFSET);
			}



			LOGV("truncate: %2.2f\n", test_len);
			test_pivot_point = glm::normalize(player_to_hit) * test_len;


			//debugdraw::line(world_position, world_position + player_to_hit, gemini::Color(1.0f, 0.5f, 0.0f), 0.2f);

			//glm::vec3 corrected_position = result - (world_position + desired_pivot_offset + SPHERE_COLLISION_RADIUS);
			//test_pivot_point = corrected_position;

			//glm::vec3 corrected_position = result - (world_position);
			//float len = glm::length(corrected_position);
			//const float DOUBLE_RADIUS = 2.0 * SPHERE_COLLISION_RADIUS;
			//float ratio = 1.0f;
			//if (len < DOUBLE_RADIUS)
			//{
			//	ratio = (len / DOUBLE_RADIUS);
			//}

			//if (len >= (2 * SPHERE_COLLISION_RADIUS))
			//LOGV("length: %2.2f\n", len);
			//const float pct = (SPHERE_COLLISION_RADIUS / len);
			//test_pivot_point = (1.0f - pct) * len * corrected_position;
			//test_pivot_point = ratio * corrected_position;

			//debugdraw::sphere(world_position + test_pivot_point, gemini::Color(1.0f, 0.0f, 0.0f), DEBUG_SPHERE_RADIUS, 0.5f);
			debugdraw::line(world_position, world_position + test_pivot_point, gemini::Color(1.0f, 0.0f, 0.0f), 0.5f);
		}
		else
		{
			// 3. The raycast hit nothing.
			const float corrected_length = offset_length;
			test_pivot_point = corrected_length * offset_direction;
			//debugdraw::sphere(world_position + (offset_direction * offset_length), gemini::Color(0.0f, 1.0f, 0.0f), DEBUG_SPHERE_RADIUS, 0.5f);
			debugdraw::line(world_position, world_position + (offset_direction * offset_length), gemini::Color(0.0f, 1.0f, 0.0f), 0.5f);
		}

		test_len = glm::length(test_pivot_point);
		offset_vector = test_len * glm::normalize(pivot_offset);
		//offset_vector.y = vertical_offset.target_value;

		horizontal_offset.target_value = offset_vector.x;
		vertical_offset.target_value = offset_vector.y;

	}
	else
	{
		horizontal_offset.target_value = pivot_offset.x;
		vertical_offset.target_value = pivot_offset.y;
	}

	LOGV("h: %2.2f, v: %2.2f\n", horizontal_offset.target_value, vertical_offset.target_value);

	//glm::vec3 corrected_offset_vector(horizontal_offset.target_value, 0.0f, 0.0f);
	//corrected_offset_vector = mathlib::rotate_vector(corrected_offset_vector, orientation);
	//debugdraw::point(world_position + corrected_offset_vector, gemini::Color(1.0f, 1.0f, 1.0f), 0.1f, 0.2f);
}

void QuaternionFollowCamera::collision_follow_distance()
{
	glm::vec3 desired_pivot_offset(horizontal_offset.target_value, vertical_offset.target_value, 0.0f);
	float offset_length = glm::length(desired_pivot_offset);
	desired_pivot_offset = mathlib::rotate_vector(desired_pivot_offset, orientation);
	glm::vec3 world_pos = world_position + desired_pivot_offset;

	//debugdraw::point(world_position + desired_pivot_offset, gemini::Color(1.0f, 1.0f, 1.0f), 0.1f, 0.2f);

	// 1. Try to raycast from the pivot offset to the world first.
	glm::vec3 offset_vector;
	glm::vec3 far_point;

	bool hit_object = false;
	uint32_t distance_truncated = false;
	float desired_distance = desired_distance_to_target;

	float average = 0.0f;
	glm::vec3 closest_world_point = world_pos + (-camera_direction * distance_to_target.current_value);
	float shortest_length = desired_distance_to_target;
#if 0
	// grab the corners of the frustum
	mathlib::compute_frustum_corners(
		near_plane,
		far_plane,
		0.1f,
		128.0f,
		1.77f,
		field_of_view.current_value,
		world_position + position,
		get_camera_direction(),
		glm::vec3(0.0f, 1.0f, 0.0f),
		camera_right
	);

	debugdraw::line(near_plane[0], far_plane[0], gemini::Color(0.0f, 0.0f, 1.0f), 0.25f);
	debugdraw::line(near_plane[1], far_plane[1], gemini::Color(0.0f, 0.0f, 1.0f), 0.25f);
	debugdraw::line(near_plane[2], far_plane[2], gemini::Color(0.0f, 0.0f, 1.0f), 0.25f);
	debugdraw::line(near_plane[3], far_plane[3], gemini::Color(0.0f, 0.0f, 1.0f), 0.25f);

	//debugdraw::line(near_plane[0], world_pos, gemini::Color(0.0f, 0.0f, 1.0f), 0.25f);
	//debugdraw::line(near_plane[1], world_pos, gemini::Color(0.0f, 0.0f, 1.0f), 0.25f);
	//debugdraw::line(near_plane[2], world_pos, gemini::Color(0.0f, 0.0f, 1.0f), 0.25f);
	//debugdraw::line(near_plane[3], world_pos, gemini::Color(0.0f, 0.0f, 1.0f), 0.25f);

	// raycast all four points of the near plane (or optionally, sweep test the entire plane)
	// and choose which one is the closest.
	for (size_t index = 0; index < 4; ++index)
	{
		// rotate offset_vector to be in the coordinate frame of the camera

		glm::vec3 world_space_hit = perform_raycast(world_pos,
			glm::normalize(near_plane[index] - world_position + desired_pivot_offset),
			desired_distance_to_target,
			&hit_object
		);

		if (hit_object)
		{
			float test_length = glm::length(world_space_hit - world_pos);
			if (test_length < shortest_length)
			{
				//const float margin = 0.1f;
				//float pct = (margin / test_length);
				//test_length *= (1.0f - pct);
				average += test_length;
				shortest_length = test_length;
				closest_world_point = world_space_hit;
				distance_truncated = 1;
				desired_distance = shortest_length;
			}
		}
	}
#else


	glm::vec3 world_space_hit = perform_raycast(world_pos,
		glm::normalize(-camera_direction),
		desired_distance_to_target + SPHERE_COLLISION_RADIUS,
		&hit_object
	);
	if (hit_object)
	{
		closest_world_point = world_space_hit;
		glm::vec3 collision_vector = world_space_hit - world_pos;
		float len = glm::length(collision_vector);

		const float pct = (SPHERE_COLLISION_RADIUS / len);
		const float full_size = len * (1.0 - pct);
		//LOGV("full_size is %2.2f\n", full_size);
		shortest_length = full_size;
		desired_distance = shortest_length;
		distance_truncated = 1;
	}
#endif

	//if (average > 0.0f)
	//{
	//	//average *= 0.25f;
	//	desired_distance = average;
	//}

	if (desired_distance < 0.0f)
	{
		desired_distance = 0.0f;
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

	//glm::vec3 dir = (closest_world_point - world_position);
	//float length = glm::length(dir);

	//float margin = 0.25f;

	//float percent = (margin / length);
	//LOGV("length: %2.2f, percent: %2.2f\n", length, percent);

	//debugdraw::line(world_pos, closest_world_point, gemini::Color(1.0f, 0.0f, 0.0f), 0.1f);
	//debugdraw::line(world_pos, world_pos + (-camera_direction * desired_distance), gemini::Color(0.5f, 0.0f, 0.5f), 0.1f);
	//debugdraw::line(world_position, world_position + (glm::normalize(dir) * length * (1.0f - percent)), gemini::Color(1.0f, 0.5f, 0.0f), 0.2f);
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
	desired_pivot_offset.y = new_offset;
}

float QuaternionFollowCamera::get_horizontal_offset() const
{
	return horizontal_offset.value();
}

void QuaternionFollowCamera::set_horizontal_offset(float new_offset)
{
	horizontal_offset.set(new_offset, 0.55f);
	desired_pivot_offset.x = new_offset;

	//collision_correct(0.0f);
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
	collision_correct(0.0f);
}

glm::mat4 QuaternionFollowCamera::compute_view_matrix() const
{
	glm::mat4 view;

	const glm::vec3 local_offset = mathlib::rotate_vector(glm::vec3(horizontal_offset.current_value, vertical_offset.current_value, 0.0f), orientation);
	const glm::mat4 world_tx = glm::translate(glm::mat4(1.0f), position + local_offset);
	glm::mat4 rot = glm::toMat4(orientation);
	view = /*glm::translate(glm::mat4(1.0), local_offset) * */world_tx * rot;

	return view;
}

glm::vec3 QuaternionFollowCamera::get_rotated_pivot_offset() const
{
	glm::vec3 offset_vector(horizontal_offset.current_value, vertical_offset.current_value, 0.0f);
	return mathlib::rotate_vector(offset_vector, orientation);
} // get_rotated_pivot_offset