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

#include <core/mathlib.h>
#include <core/typedefs.h>
#include <renderer/color.h>
#include <core/interpolation.h>
#include <runtime/debugvar.h>
#include <runtime/runtime.h>


// REFERENCE LINKS

//
// TUTORIALS / REFERENCE
//
//http://gpwiki.org/index.php/OpenGL:Tutorials:Using_Quaternions_to_represent_rotation
//http://www.gamedev.net/reference/articles/article1927.asp - Camera Class Tutorial
//http://www.flipcode.com/cgi-bin/fcarticles.cgi?show=64156 - OpenGL camera
//http://www.gamedev.net/reference/articles/article1997.asp - A Simple vec4-based Camera
//http://www.gamedev.net/reference/articles/article2160.asp - Creating a Useful Camera Class
//http://www.devmaster.net/articles/viewing-systems/ - Cameras for 3D engines (UVN)
//http://www.flipcode.com/documents/matrfaq.html - Matrix FAQ (huge)
//http://www.gamasutra.com/features/19980703/quaternions_01.htm - rotating objects using quaternions
//http://www.gamedev.net/reference/articles/article2023.asp - virtual camera position
//http://www.gamedev.net/reference/articles/article1591.asp - a simple third person camera using the polar coordinate system


//
// FORUM THREADS
//
//http://www.gamedev.net/community/forums/topic.asp?topic_id=321402 - last post by jyk; his code works.
//http://www.gamedev.net/community/forums/topic.asp?topic_id=421529 - manual alternative to gluLookAt
//http://www.gamedev.net/community/forums/topic.asp?topic_id=419777 - reconstruct viewing matrix
//http://www.gamedev.net/community/forums/topic.asp?topic_id=418443 - Odd Quaternion Camera, jyk posts helpful info on cameras
//http://www.gamedev.net/community/forums/topic.asp?topic_id=321442 - Pitch/Yaw problem
//http://www.gamedev.net/community/forums/topic.asp?topic_id=432017 - Pitch / Yaw around mesh while matching roll
//http://www.gamedev.net/community/forums/topic.asp?topic_id=431476 - Quaternions, standard X Y Z rotation
//http://www.gamedev.net/community/forums/topic.asp?topic_id=413071 - from direction vector to complete matrix
//http://www.gamedev.net/community/forums/topic.asp?topic_id=418024 - Quaternions step by step
//http://www.gamedev.net/community/forums/topic.asp?topic_id=311086 - Use Quaternions for spaceship orientations
//http://www.gamedev.net/community/forums/topic.asp?topic_id=334484 - Camera question (Space Sim)
//http://www.gamedev.net/community/forums/topic.asp?topic_id=428775 - Camera for spaceship games
//http://www.gamedev.net/community/forums/topic.asp?topic_id=414736 - 3D Vector Rotation Issue
//http://www.gamedev.net/community/forums/topic.asp?topic_id=313276 - Vector rotation + trigonometry question
//http://www.gamedev.net/community/forums/topic.asp?topic_id=423165 - Rotation values from Tangent to Curve (vector)
//http://www.gamedev.net/community/forums/topic.asp?topic_id=146259 - Camera Code?
//http://www.gamedev.net/community/forums/topic.asp?topic_id=449625 - camera class
//http://www.scotboyd.net/90percent/2007/05/quaternion-camera-in-xna.html - Quaternion Camera in XNA
//http://www.gamedev.net/community/forums/topic.asp?topic_id=376956 - direction vector in FPS style games
//http://www.gamedev.net/community/forums/topic.asp?topic_id=437766 - How to rotate a vector (relative)
//http://www.gamedev.net/community/forums/topic.asp?topic_id=261028 - camera problems

// TargetCamera References
// http://www.gamedev.net/community/forums/topic.asp?topic_id=473371 - cursor moving faster than object
// http://www.gamedev.net/community/forums/topic.asp?topic_id=480789 - Rotating a group of points around a pivot
// http://www.gamedev.net/topic/655639-orbit-quaternion-camera/?hl=%2Bspherical+%2Bcoordinates+%2Bcamera - polar coordinates
// https://blog.nobel-joergensen.com/2010/10/22/spherical-coordinates-in-unity/ - polar coordinates
// https://en.wikipedia.org/wiki/Spherical_coordinate_system - spherical coordinates
// http://www.mathworks.com/help/matlab/ref/cart2sph.html - spherical coordinate system transformation
// http://www.gamasutra.com/blogs/YoannPignole/20150928/249412/Third_person_camera_design_with_free_move_zone.php

// https://en.wikipedia.org/wiki/Spherical_trigonometry - spherical trig

#include <core/fixedsizequeue.h>
#include <sdk/camera.h>

class QuaternionFollowCamera : public GameCamera
{
public:
	glm::quat orientation;


	glm::quat yaw_rot;
	glm::quat pitch_rot;
	float yaw;
	float pitch;

	gemini::physics::ICollisionShape* collision_shape;
	gemini::physics::ICollisionObject* collision_object;
private:

	// truck: up and down
	// tilt; from camera origin (up/down)
	// pan: left/right from camera origin
	// field of view

#if 0
	// TODO: replace this with a spline
	float pitch;
	float pitch_min;
	float pitch_max;
#endif

	AnimatedTargetValue<float> field_of_view;
	AnimatedTargetValue<float> vertical_offset;
	AnimatedTargetValue<float> horizontal_offset;
	AnimatedTargetValue<float> distance_to_target;
	gemini::DebugVar<float> dbg_distance_to_target;
	gemini::DebugVar<float> dbg_horizontal_offset;
	gemini::DebugVar<glm::vec3> dbg_position;
	gemini::DebugVar<glm::vec3> dbg_world_position;
	//gemini::DebugVar<float> dbg_desired_distance;
	//gemini::DebugVar<float> dbg_desired_distance_to_target;
	//gemini::DebugVar<glm::vec2> dbg_current_pivot_offset;

	//float desired_horizontal_offset;

	glm::vec3 position;
	glm::vec3 target_facing_direction;

	// did the view move this tick?
	size_t view_moved;

	//float follow_distance;
	//float player_height;

	//float distance_to_target;
	float desired_distance_to_target;
	glm::vec2 desired_pivot_offset;

	//float desired_distance;
	float minimum_distance;
	//uint32_t distance_truncated; // set to 1 if the camera ran into something

	//glm::vec2 desired_pivot_offset;
	//glm::vec2 current_pivot_offset;

	glm::vec3 camera_direction;
	glm::vec3 camera_right;

	float interpolation_time;
	glm::quat interpolation_rotation;
	glm::vec3 interpolation_vector;

	glm::vec3 world_position;

	float auto_orient_seconds;
	size_t auto_orienting;

	glm::vec3 near_plane[4];
	glm::vec3 far_plane[4];

	glm::vec3 perform_raycast(const glm::vec3& start, const glm::vec3& direction, float max_distance, bool* hit_object);

	void collision_pivot_offset();
	void collision_follow_distance();

public:
	QuaternionFollowCamera();
	virtual ~QuaternionFollowCamera();

	const glm::quat& get_rotation() const
	{
		return orientation;
	}

	virtual glm::vec3 get_origin() const override;
	virtual glm::vec3 get_target() const override;
	virtual glm::vec3 get_up() const override;
	virtual glm::vec3 get_right() const override;
	virtual float get_fov() const override;
	virtual CameraType get_type() const override { return CameraType::FollowCamera; }
	virtual void move_view(float yaw, float pitch) override;
	virtual void set_yaw_pitch(float yaw, float pitch) override;
	virtual void tick(float step_interval_seconds) override;
	virtual void set_fov(float new_fov) override;
	virtual void set_target_direction(const glm::vec3& direction) override;
	virtual glm::vec3 get_target_direction() const override;
	virtual glm::vec3 get_camera_direction() const override;
	virtual void reset_view() override;

	virtual glm::vec3 get_eye_position() const;

	void set_follow_distance(float target_distance);
	void set_view(const glm::vec3& view_direction);
	void update_view_orientation();

	virtual void set_target_fov(float new_fov) override;
	float get_vertical_offset() const;
	virtual void set_vertical_offset(float new_offset) override;

	float get_horizontal_offset() const;
	void set_horizontal_offset(float new_offset);

	float get_distance_from_pivot() const;

	void set_minimum_distance(float min_distance);
	virtual void set_initial_state(const gemini::CameraState& state);
	virtual void get_current_state(gemini::CameraState& state);
	virtual void set_world_position(const glm::vec3& world_position);

	// correct camera position by testing collision
	virtual void collision_correct(float step_interval_seconds) override;

private:
	glm::mat4 compute_view_matrix() const;

	glm::vec3 get_rotated_pivot_offset() const;


	glm::vec2 move_sensitivity;
};

