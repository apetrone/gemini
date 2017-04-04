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

#include <sdk/utils.h>
#include <sdk/physics_api.h>



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
	, dbg_distance_to_target("distance_to_target", &distance_to_target)
	, dbg_horizontal_offset("horizontal_offset", &horizontal_offset.current_value)
	, dbg_position("position", &position)
	, dbg_target_position("target_position", &target_position)
	, dbg_desired_distance("desired_distance", &desired_distance)
	, dbg_desired_distance_to_target("desired_distance_to_target", &desired_distance_to_target)
	, dbg_current_pivot_offset("current_pivot_offset", &current_pivot_offset)
{
	target_position = glm::vec3(0.0f, 0.0f, 0.0f);
	position = glm::vec3(0.0f, 3.0f, 3.0f);
	target_facing_direction = glm::vec3(0.0f, 0.0f, -1.0f);

	move_sensitivity = glm::vec2(25.0f, 25.0f);
	// height above the player where camera should be
	//splayer_height = 1.0f;

	distance_to_target = 5.0f;
	desired_distance = distance_to_target;
	desired_distance_to_target = distance_to_target;
	distance_truncated = 0;
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

	collision_shape = gemini::physics::instance()->create_convex_shape(cam_vertices, 6);
	collision_shape = gemini::physics::instance()->create_sphere(0.5f);
	uint16_t collision_mask = (gemini::physics::StaticFilter | gemini::physics::KinematicFilter);
	collision_object = gemini::physics::instance()->create_kinematic_object(collision_shape, glm::vec3(0.0f, 0.0f, 0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), collision_mask);
	LOGV("created collision object for camera %x\n", collision_object);
	//// I don't know if I can use the same shape...
	//offset_collision_shape = gemini::physics::instance()->create_sphere(0.5f);
	//offset_collision_object = gemini::physics::instance()->create_kinematic_object(offset_collision_shape, glm::vec3(0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), collision_mask);
}

QuaternionFollowCamera::~QuaternionFollowCamera()
{
	gemini::physics::instance()->destroy_object(collision_object);
	//gemini::physics::instance()->destroy_object(offset_collision_object);
}

glm::vec3 QuaternionFollowCamera::get_origin() const
{
	glm::vec3 distance(0.0f, 0.0f, distance_to_target);

	return target_position + get_rotated_pivot_offset() + mathlib::rotate_vector(distance, orientation);
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

	position = (-camera_direction * distance_to_target);
}

void QuaternionFollowCamera::tick(float step_interval_seconds)
{
	if (distance_truncated)
	{
		// snap to collision
		distance_to_target = desired_distance;
	}
	else
	{
		// exponential camera chase to desired_distance; gracefully zoom
		float vel = (desired_distance - distance_to_target);
		distance_to_target = desired_distance - (vel * exp(-step_interval_seconds / 0.45f));
	}

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

				//LOGV("start auto orienting\n");
				reset_view();
				auto_orient_seconds = kAutoOrientWaitTime;
			}
		}
	}
	else
	{
		// reset the wait timer
		auto_orient_seconds = kAutoOrientWaitTime;
		//LOGV("reset auto orient timer\n");

		interpolation_time = 0.0f;
		auto_orienting = 0;
	}

	position = (-camera_direction * distance_to_target);// +glm::vec3(horizontal_offset.current_value, vertical_offset.current_value, 0.0f);
	//const glm::vec3 camera_world_position = target_position + position;


	//glm::vec3 offset_vector(desired_horizontal_offset, vertical_offset.value(), 0.0f);

	////float offset_length = glm::length(offset_vector);
	//position += mathlib::rotate_vector(offset_vector, orientation);


	position += get_rotated_pivot_offset();

	////collision_object->set_world_transform(camera_world_position + offset_vector, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));


	//debugdraw::camera(
	//	camera_world_position + glm::vec3(0.0f, 1.0f, 0.0),
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

	if ((view_moved == 0) && (auto_orienting == 1) && (interpolation_time > 0.0f))
	{
		float t = (kInterpolationTime - interpolation_time) / kInterpolationTime;
		glm::quat rot = gemini::slerp(glm::quat(1.0f, 0.0f, 0.0f, 0.0f), interpolation_rotation, t);
		camera_direction = mathlib::rotate_vector(interpolation_vector, rot);

		collision_correct();

		//LOGV("t = %2.2f %2.2f\n", t, interpolation_time);
		interpolation_time -= step_interval_seconds;

		if (interpolation_time <= 0.0f)
		{
			LOGV("finished auto-orienting camera\n");
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

glm::vec3 QuaternionFollowCamera::perform_raycast(const glm::vec3& start, const glm::vec3& direction, const gemini::Color& color, float max_distance)
{
	glm::vec3 point;
	gemini::physics::RaycastInfo result = gemini::physics::instance()->raycast(start, direction, max_distance, collision_object);
	if (result.object)
	{
		// We should never hit the camera physics ghost.
		assert(result.object != collision_object);

		// we hit something
		//debugdraw::line(start, result.hit, color, 3.0f);
		point = direction * (glm::length(result.hit - start) * 0.85f);
		distance_truncated = 1;
		debugdraw::line(start, start + point, color, 3.0f);
	}
	else
	{
		point = (direction * max_distance);
		distance_truncated = 0;
		//debugdraw::line(start, start + (direction * max_distance), color);
	}

	return point;
}

void QuaternionFollowCamera::collision_correct()
{
	const float MIN_CAMERA_DISTANCE = 0.25f;

	//debugdraw::line(target_position, glm::vec3(0.0f), gemini::Color(0.0f, 0.0f, 1.0f));
	//debugdraw::line(target_position, target_position + (-camera_direction * glm::vec3(1.0f)), gemini::Color(0.0f, 0.0f, 1.0f), 1.0);




	glm::vec3 raycast_result;

	// step 1: Raycast to desired camera offset from pivot
	//glm::vec3 offset_vector(desired_horizontal_offset, vertical_offset.value(), 0.0f);

	//float offset_length = glm::length(offset_vector);
	//offset_vector = mathlib::rotate_vector(offset_vector, orientation);
	//if (offset_length > FLT_EPSILON)
	//{

	//	raycast_result = perform_raycast(target_position, glm::normalize(offset_vector), gemini::Color(1.0f, 0.5f, 0.0f), offset_length);

	//	offset_length = glm::length(raycast_result);

	//	float lerp_time = 0.25f;
	//	if (offset_length < MIN_CAMERA_DISTANCE)
	//	{
	//		offset_length = MIN_CAMERA_DISTANCE;
	//		lerp_time = 0.0f;
	//	}

	//	horizontal_offset.set(offset_length, lerp_time);
	//}

	// step 2: raycast to desired distance from pivot
	//glm::vec3 far_point = perform_raycast(target_position, -camera_direction, gemini::Color(0.0f, 1.0f, 0.0f), desired_distance_to_target);
	//desired_distance = glm::length(far_point);

	////debugdraw::line(target_position, target_position + far_point, gemini::Color(1.0f, 0.0f, 0.0f), 3.0f);

	//if (desired_distance < MIN_CAMERA_DISTANCE)
	//{
	//	desired_distance = MIN_CAMERA_DISTANCE;
	//	distance_truncated = 1;
	//}

	float offset_length = glm::length(desired_pivot_offset);
	glm::vec3 offset_vector;
	if (offset_length > 0.0f)
	{
		offset_vector = glm::vec3(desired_pivot_offset, 0.0f);

		// rotate offset_vector to be in the coordinate frame of the camera
		offset_vector = mathlib::rotate_vector(offset_vector, orientation);

		glm::vec3 test_pivot_point = perform_raycast(target_position, glm::normalize(offset_vector), gemini::Color(0.0f, 1.0f, 0.0f), offset_length);
		LOGV("len: %2.2f\n", glm::length(test_pivot_point));
		current_pivot_offset = glm::length(test_pivot_point) * desired_pivot_offset;
		// 1. desired horizontal offset (shouldn't change frame by frame)
		// 2. current_target_horizontal_offset (varies frame by frame)
	}
	else
	{
		desired_pivot_offset = glm::vec2(0.0f);
		current_pivot_offset = desired_pivot_offset;
	}

	// step 2: raycast to desired distance from pivot
	offset_vector = get_rotated_pivot_offset();
	debugdraw::point(target_position + offset_vector, gemini::Color(1.0f, 1.0f, 1.0f), 0.1f, 0.2f);
	glm::vec3 far_point = perform_raycast(target_position + offset_vector, -camera_direction, gemini::Color(0.0f, 1.0f, 1.0f), desired_distance_to_target);
	desired_distance = glm::length(far_point);

	debugdraw::line(target_position + offset_vector, target_position + offset_vector + far_point, gemini::Color(1.0f, 0.0f, 0.0f), 3.0f);

	if (desired_distance < MIN_CAMERA_DISTANCE)
	{
		desired_distance = MIN_CAMERA_DISTANCE;
		distance_truncated = 1;
	}
}

void QuaternionFollowCamera::set_target_position(const glm::vec3& target_worldspace_position)
{
	target_position = target_worldspace_position;
	collision_correct();
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
	glm::vec3 new_direction = target_facing_direction; //glm::vec3(0.0f, 0.0f, -1.0f); //

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
	return target_position + position;
}

void QuaternionFollowCamera::set_follow_distance(float target_distance)
{
	distance_to_target = target_distance;
	desired_distance_to_target = target_distance;
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
	desired_horizontal_offset = new_offset;
	horizontal_offset.set(new_offset, 0.15f);

	desired_pivot_offset.x = new_offset;
}

float QuaternionFollowCamera::get_distance_from_pivot() const
{
	return distance_to_target;
}

void QuaternionFollowCamera::simulate(float delta_time_seconds)
{
	// use the offset vector and rotate it by the camera rotation

	float remaining_time = delta_time_seconds;

	glm::vec3 camera_world_position = target_position + position;


	//int i;
	//for (i = 0; (i < 3) && (remaining_time > 0.0f); ++i)
	//{
	//	gemini::physics::SweepTestResult sweep = gemini::physics::instance()->sweep(collision_object, collision_shape, pos1, pos1 + offset_vector, 0.0f);
	//	if (sweep.hit_items() > 0)
	//	{
	//		LOGV("sweep hit something at %2.2f\n", sweep.closest_hit_fraction);
	//		offset_vector *= sweep.closest_hit_fraction;
	//		camera_world_position = pos1 + offset_vector;
	//		break;
	//	}
	//	else
	//	{
	//		/*target_position = new_position;*/

	//		break;
	//	}
	//}
	//if (i == 3)
	//{
	//	LOGV("max iterations hit\n");
	//}

	collision_object->set_world_transform(camera_world_position, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
}

gemini::physics::ICollisionObject* QuaternionFollowCamera::get_collider()
{
	return collision_object;
}

glm::vec3 QuaternionFollowCamera::get_rotated_pivot_offset() const
{
	//glm::vec3 offset_vector(desired_horizontal_offset, vertical_offset.value(), distance_to_target);
	glm::vec3 offset_vector(current_pivot_offset, 0.0f);
	return mathlib::rotate_vector(offset_vector, orientation);
} // get_rotated_pivot_offset