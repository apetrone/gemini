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

enum class CameraType
{
	DefaultCamera,
	FollowCamera,
	FixedCamera
};

const char* cameratype_to_string(CameraType type);

namespace gemini
{
	struct CameraState;
}

class GameCamera
{
public:
	virtual ~GameCamera();

	// returns the camera's 'eye' position
	virtual glm::vec3 get_origin() const = 0;
	virtual glm::vec3 get_target() const = 0;
	virtual glm::vec3 get_up() const = 0;
	virtual glm::vec3 get_right() const = 0;

	// return the vertical field of view for this camera
	virtual float get_fov() const = 0;

	// returns the discrete camera type
	virtual CameraType get_type() const = 0;

	// Called when the camera's should rotate by a delta
	virtual void move_view(float yaw, float pitch) = 0;

	// set absolute yaw and pitch
	virtual void set_yaw_pitch(float yaw, float pitch) = 0;

	// Called each frame
	virtual void tick(float step_interval_seconds) = 0;

	// set the vertical field of view
	virtual void set_fov(float new_fov) = 0;

	// Set the target object's moved direction.
	virtual void set_target_direction(const glm::vec3& direction) = 0;

	// Get the target object's facing direction.
	virtual glm::vec3 get_target_direction() const = 0;

	// Get the camera's facing / eye vector
	virtual glm::vec3 get_camera_direction() const = 0;

	// Reset this camera's view.
	virtual void reset_view() = 0;


	virtual void set_minimum_distance(float value) = 0;
	virtual void set_follow_distance(float value) = 0;

	// Copy CameraState to internal state
	virtual void set_initial_state(const gemini::CameraState& state) = 0;
	virtual void get_current_state(gemini::CameraState& state) = 0;

	// Sets the world position of the camera.
	// This allows the camera to perform its own collision detection and
	// response as well as allowing it to form the view matrix when parameters
	// are extracted from the camera.
	virtual void set_world_position(const glm::vec3& world_position) = 0;

	virtual void set_target_fov(float new_fov) = 0;
	virtual void set_horizontal_offset(float new_horizontal_offset) = 0;
	virtual void set_vertical_offset(float new_vertical_offset) = 0;

	virtual void collision_correct(float step_interval_seconds) = 0;
}; // GameCamera


// distance and field of view driven from pitch.

#include <sdk/physics_collisionobject.h>


template <class T>
struct AnimatedTargetValue
{
	T target_value;
	T original_value;
	T current_value;
	float target_time_seconds;
	float current_time_seconds;

	void set(const T& desired_value, float lerp_duration_seconds)
	{
		if (lerp_duration_seconds > 0.0f)
		{
			// lerp to this value over time
			target_value = desired_value;
			current_time_seconds = 0.0f;
			target_time_seconds = lerp_duration_seconds;
		}
		else
		{
			// snap to this value
			current_value = desired_value;
			target_value = desired_value;
			current_time_seconds = 0.0f;
			target_time_seconds = 0.0f;
		}
		original_value = current_value;
	}

	void update(float delta_seconds)
	{
#if 0
		if (target_time_seconds > 0.0f)
		{
			current_time_seconds += delta_seconds;
			float alpha = glm::clamp(current_time_seconds / target_time_seconds, 0.0f, 1.0f);
			if (current_time_seconds > target_time_seconds)
			{
				target_time_seconds = 0.0f;
			}

			current_value = gemini::lerp(original_value, target_value, alpha);
		}
#endif
		// exponential camera chase to desired_distance; gracefully zoom
		float vel = (current_value - target_value);
		//F = - k (p - r)

		current_value += -20.0 * (vel * delta_seconds);
		//current_value = target_value - (vel * exp(-delta_seconds / 0.45f));

		//float vel = (desired_distance - distance_to_target);
		//distance_to_target = desired_distance - (vel * exp(-step_interval_seconds / 0.45f));

		current_value = target_value;
	}

	T value() const
	{
		return current_value;
	}
};

class FixedCamera : public GameCamera
{
private:
	glm::vec3 origin;
	glm::vec3 target;
	float field_of_view;

public:
	FixedCamera(const glm::vec3& position, const glm::vec3& target, float fov);

	virtual glm::vec3 get_origin() const override;
	virtual glm::vec3 get_target() const override;
	virtual glm::vec3 get_up() const override;
	virtual glm::vec3 get_right() const override;
	virtual float get_fov() const override;
	virtual CameraType get_type() const override { return CameraType::FixedCamera; }
	virtual void move_view(float yaw, float pitch) override;
	virtual void set_yaw_pitch(float yaw, float pitch) override;
	virtual void tick(float step_interval_seconds) override;
	virtual void set_fov(float new_fov) override;
	virtual void set_target_direction(const glm::vec3& direction) override;
	virtual glm::vec3 get_target_direction() const override;
	virtual glm::vec3 get_camera_direction() const override;
	virtual void reset_view() override;
};

class CameraMixer
{
private:
	struct CameraBlend
	{
		GameCamera* camera;
		float weight;

		CameraBlend(GameCamera* game_camera = nullptr, float blend_weight = 0.0f)
			: camera(game_camera)
			, weight(blend_weight)
		{
		}
	};

	enum class CameraMixAction
	{
		Idle,
		Blend_Push,
		Blend_Pop
	};

	FixedSizeQueue<CameraBlend, 4> cameras;

	glm::vec3 origin;
	glm::vec3 view;
	glm::vec3 offset;
	glm::vec3 world_position;

	gemini::Allocator& allocator;

	float blend_alpha;
	float current_time_sec;
	float total_time_sec;
	CameraMixAction action;

	void normalize_weights(float top_weight);
public:
	CameraMixer(gemini::Allocator& allocator);
	~CameraMixer();

	// push a new camera onto the stack
	void push_camera(GameCamera* new_camera, float delay_sec);

	// pop the current camera off the stack
	void pop_camera(float delay_sec);

	GameCamera* get_top_camera();

	// get origin position
	glm::vec3 get_origin() const;

	// return camera's view vector
	glm::vec3 get_target() const;

	// return camera's up vector
	glm::vec3 get_up() const;

	// return camera's right vector
	glm::vec3 get_right() const;

	// get field of view
	float get_field_of_view() const;

	void tick(float step_interval_seconds, float step_alpha);
	void move_view(float yaw, float pitch);
	void set_yaw_pitch(float yaw, float pitch);

	// Set the direction of travel for the target object.
	void set_target_direction(const glm::vec3& direction);

	glm::vec3 get_target_direction() const;
	glm::vec3 get_camera_direction() const;

	// sets the world position of the camera mixer "node"
	void set_world_position(const glm::vec3& world_position);

	void collision_correct(float step_interval_seconds);
};

struct Camera
{
	enum CameraType
	{
		FIRST_PERSON,	// classic first-person camera (Doom/Quake)
		THIRD_PERSON,	// third-person camera (cover-based shooters)
		CHASE,			// third-person chase camera (3D platformers)
	};

	Camera(CameraType _type = FIRST_PERSON);

	// sets world position of the camera

	void invert_yaxis( bool invert ) { invert_y_axis = invert; }

	// called when the mouse moves to update the view
	void move_view( int32_t dx, int32_t dy );

	// position-based movement
	void move_left( real dt );
	void move_right( real dt );
	void move_forward( real dt );
	void move_backward( real dt );
	void move_along_vector( const glm::vec3 & v, real dt );

	// internal functions
	virtual void update_view();

	// projection type functions
	void perspective(real fovy, int32_t width, int32_t height, real nearz, real farz);
	void ortho(real left, real right, real bottom, real top, real nearz, real farz);

	const glm::mat4& get_inverse_world_transform() const { return inverse_world_transform; }
	const glm::mat4& get_inverse_rotation() const { return inverse_rotation; }


	void set_yaw(real y) { yaw = y; }
	void set_pitch(real p) { pitch = p; }
	void set_position(const glm::vec3& position);
	const glm::vec3& get_position() const { return pos; }

	void set_view(const glm::vec3& view_direction) { view = view_direction; }
	void set_type(const CameraType cameratype) { type = cameratype; }
	void set_target_offset(const glm::vec3& in_target_offset) { target_offset = in_target_offset; }

	const glm::mat4& get_modelview() const { return modelview; }
	const glm::mat4& get_projection() const { return projection; }
	const glm::vec3& get_view() const { return view; }
	const glm::vec3& get_eye_position() const { return eye_position; }

	float get_yaw() const { return yaw; }
	float get_pitch() const { return pitch; }

private:
	// camera variables
	real yaw;
	real pitch;
	glm::vec3 pos;

	glm::mat4 projection;
	glm::mat4 modelview;

	// components as matrices
	glm::mat4 inverse_rotation;
	glm::mat4 inverse_translation;

	// final inverted world matrix
	glm::mat4 inverse_world_transform;

	// directional vectors
	glm::vec3 view;
	glm::vec3 side;

	glm::vec3 target_offset;
	glm::vec3 eye_position;

	CameraType type;

	//
	// options
	real move_speed;
	real sensitivity;
	bool invert_y_axis;
	bool is_ortho;
	float aspect_ratio;
	float fovy;

	float near_clip;
	float far_clip;
}; // Camera

