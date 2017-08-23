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
#include <core/logging.h>

#include <renderer/debug_draw.h>

#include <sdk/game_api.h>
#include <sdk/physics_api.h>
#include <sdk/utils.h>


const char* cameratype_to_string(CameraType type)
{
	switch (type)
	{
		case CameraType::DefaultCamera: return "DefaultCamera";
		case CameraType::FollowCamera: return "FollowCamera";
		case CameraType::FixedCamera: return "FixedCamera";
		default: break;
	}

	return "UnknownCamera";
}

// --------------------------------------------------------
// GameCamera
// --------------------------------------------------------
GameCamera::~GameCamera()
{
}

// --------------------------------------------------------
// DefaultCamera
// --------------------------------------------------------
class DefaultCamera : public GameCamera
{
public:
	virtual glm::vec3 get_origin() const override
	{
		glm::vec3 o(0.0f, 12.0f, 5.0f);
		return o;
	}

	virtual glm::vec3 get_target() const override
	{
		glm::vec3 view(0.0f, 0.0f, -1.0f);
		return view;
	}

	virtual glm::vec3 get_up() const override
	{
		return glm::vec3(0.0f, 1.0f, 0.0f);
	}

	virtual glm::vec3 get_right() const override
	{
		return glm::vec3(1.0f, 0.0f, 0.0f);
	}

	virtual float get_fov() const override
	{
		return 50.0f;
	}

	virtual CameraType get_type() const override { return CameraType::DefaultCamera; }

	virtual void move_view(float yaw, float pitch) override
	{
	}

	virtual void set_yaw_pitch(float yaw, float pitch) override
	{
	}

	virtual void tick(float step_interval_seconds) override
	{
	}

	virtual void set_fov(float new_fov) override
	{
	}

	virtual void set_target_direction(const glm::vec3& direction) override
	{
	}

	virtual glm::vec3 get_target_direction() const override
	{
		return glm::vec3(0.0f, 0.0f, 0.0f);
	}

	virtual glm::vec3 get_camera_direction() const override
	{
		return glm::vec3(0.0f, 0.0f, 0.0f);
	}

	virtual void reset_view() override
	{
	}
};

// --------------------------------------------------------
// FixedCamera
// --------------------------------------------------------
FixedCamera::FixedCamera(const glm::vec3& position, const glm::vec3& target_position, float fov)
	: origin(position)
	, target(target_position)
	, field_of_view(fov)
{
}

glm::vec3 FixedCamera::get_origin() const
{
	return origin;
}

glm::vec3 FixedCamera::get_target() const
{
	return target;
}

glm::vec3 FixedCamera::get_up() const
{
	return glm::vec3(0.0f, 1.0f, 0.0f);
}

glm::vec3 FixedCamera::get_right() const
{
	return glm::vec3(1.0f, 0.0f, 0.0f);
}

float FixedCamera::get_fov() const
{
	return field_of_view;
}

void FixedCamera::move_view(float yaw, float pitch)
{
}

void FixedCamera::set_yaw_pitch(float yaw, float pitch)
{
}


void FixedCamera::tick(float step_interval_seconds)
{
}

void FixedCamera::set_fov(float new_fov)
{
}

void FixedCamera::set_target_direction(const glm::vec3& direction)
{
}

glm::vec3 FixedCamera::get_target_direction() const
{
	return glm::vec3(0.0f, 0.0f, 0.0f);
}

glm::vec3 FixedCamera::get_camera_direction() const
{
	return glm::vec3(0.0f, 0.0f, -1.0f);
}

void FixedCamera::reset_view()
{
}

// --------------------------------------------------------
// CameraMixer
// --------------------------------------------------------
void CameraMixer::normalize_weights(float top_weight)
{
	// This does NOT correctly normalize the weights of cameras.
	// It's unclear how I want this to behave -- so for now
	// it mainly works to push and pop a single camera on a stack
	// with full weight.
	float weight_remaining = top_weight;
	const size_t total_cameras = cameras.size();
	for (size_t index = 0; index < total_cameras; ++index)
	{
		CameraBlend& blend = cameras[index];
		blend.weight = weight_remaining;

		weight_remaining -= blend.weight;
		weight_remaining = glm::clamp(weight_remaining, 0.0f, 1.0f);
	}
}

CameraMixer::CameraMixer(gemini::Allocator& _allocator)
	: allocator(_allocator)
	, action(CameraMixAction::Idle)
	, blend_alpha(0.0f)
	, current_time_sec(0.0f)
	, total_time_sec(0.0f)
{
	// default camera
	//DefaultCamera* camera = MEMORY2_NEW(allocator, DefaultCamera);
	//push_camera(camera, 1.0f);

	origin = glm::vec3(0.0f, 0.0f, 0.0f);
	view = glm::vec3(0.0f, 0.0f, -1.0f);
	offset = glm::vec3(0.0f, 0.0f, 0.0f);
}

CameraMixer::~CameraMixer()
{
	const size_t total_cameras = cameras.size();
	for (size_t index = 0; index < total_cameras; ++index)
	{
		CameraBlend blend = cameras.pop();
		MEMORY2_DELETE(allocator, blend.camera);
	}
}

void CameraMixer::push_camera(GameCamera* camera, float delay_sec)
{
	// set interval to delay_msec
	CameraBlend blend(camera, 1.0f);
	if (!cameras.empty())
	{
		// If there's a prior camera, copy its state
		// to the new camera as a basis for interpolation.
		gemini::CameraState camera_state;
		GameCamera* top_camera = get_top_camera();
		top_camera->get_current_state(camera_state);
		camera->set_initial_state(camera_state);
	}

	cameras.push_back(blend);

	total_time_sec = delay_sec;
	current_time_sec = 0.0f;
	blend_alpha = 0.0f;
	action = CameraMixAction::Blend_Push;

	//normalize_weights(weight);

	// force an update of the origin/view
	//tick(0.0f, 0.0f);
}

void CameraMixer::pop_camera(float delay_sec)
{
	total_time_sec = delay_sec;
	current_time_sec = 0.0f;
	blend_alpha = 0.0f;
	action = CameraMixAction::Blend_Pop;
	//CameraBlend blend = cameras.pop();
	//MEMORY2_DELETE(allocator, blend.camera);

	//normalize_weights(weight);

	// re-normalize
	//tick(0.0f, 0.0f);
}

GameCamera* CameraMixer::get_top_camera()
{
	return cameras.top().camera;
}

glm::vec3 CameraMixer::get_origin() const
{
	return cameras.top().camera->get_origin();
}

glm::vec3 CameraMixer::get_target() const
{
	return cameras.top().camera->get_target();
}

glm::vec3 CameraMixer::get_up() const
{
	return cameras.top().camera->get_up();
}

glm::vec3 CameraMixer::get_right() const
{
	return cameras.top().camera->get_right();
}

float CameraMixer::get_field_of_view() const
{
	assert(cameras.size() > 0);
	return cameras.top().camera->get_fov();
	//return gemini::lerp(cameras[0].camera->get_fov(), cameras[1].camera->get_fov(), cameras[1].weight);
	//return cameras.top().camera->get_fov();
}

void CameraMixer::tick(float step_interval_seconds, float step_alpha)
{
#if 0
	// aggregate values for origin and view.
	origin = glm::vec3(0.0f, 0.0f, 0.0f);
	view = glm::vec3(0.0f, 0.0f, 0.0f);

	bool finished_transition = false;

	if (action != CameraMixAction::Idle)
	{
		current_time_sec += step_interval_seconds;
		blend_alpha = (current_time_sec / total_time_sec);
		if (blend_alpha >= 1.0f)
		{
			finished_transition = true;
		}
		//else
		//{
		//	LOGV("blend_alpha is %2.2f\n", blend_alpha);
		//}
	}

	if (action == CameraMixAction::Blend_Push)
	{
		// blend the current top camera in; blend second out.

		CameraBlend& top = cameras[0];
		top.weight = blend_alpha;

		CameraBlend& second = cameras[1];
		second.weight = 1.0f - blend_alpha;

		//LOGV("weights: %2.2f, %2.f\n", top.weight, second.weight);
	}
	else if (action == CameraMixAction::Blend_Pop)
	{
		CameraBlend& second = cameras[1];
		second.weight = blend_alpha;

		CameraBlend& top = cameras[0];
		top.weight = 1.0f - blend_alpha;

		//LOGV("weights: %2.2f, %2.f\n", top.weight, second.weight);
		if (finished_transition)
		{
			CameraBlend blend = cameras.pop();
			MEMORY2_DELETE(allocator, blend.camera);
		}
	}

	if (finished_transition)
	{
		current_time_sec = 0.0f;
		total_time_sec = 0.0f;
		action = CameraMixAction::Idle;
	}

	const size_t total_cameras = cameras.size();
	assert(total_cameras > 0);
	for (size_t index = 0; index < total_cameras; ++index)
	{
		CameraBlend& blend = cameras[index];
		GameCamera* camera = blend.camera;
		camera->tick(step_interval_seconds);
		origin += (blend.weight * camera->get_origin());
		view += (blend.weight * camera->get_target());
	}
#endif


	if (cameras.empty())
	{
		return;
	}

	CameraBlend& blend = cameras[0];
	blend.camera->set_world_position(world_position);
	blend.camera->tick(step_interval_seconds);
}

void CameraMixer::move_view(float yaw_delta, float pitch_delta)
{
	const size_t total_cameras = cameras.size();
	for (size_t index = 0; index < total_cameras; ++index)
	{
		CameraBlend& blend = cameras[index];
		blend.camera->move_view(yaw_delta, pitch_delta);
	}
}

void CameraMixer::set_yaw_pitch(float yaw, float pitch)
{
	const size_t total_cameras = cameras.size();
	for (size_t index = 0; index < total_cameras; ++index)
	{
		CameraBlend& blend = cameras[index];
		blend.camera->set_yaw_pitch(yaw, pitch);
	}
}

//void CameraMixer::set_target_position(const glm::vec3& target_worldspace_position)
//{
//	const size_t total_cameras = cameras.size();
//	for (size_t index = 0; index < total_cameras; ++index)
//	{
//		CameraBlend& blend = cameras[index];
//		blend.camera->set_target_position(target_worldspace_position);
//	}
//}

void CameraMixer::set_target_direction(const glm::vec3& direction)
{
	const size_t total_cameras = cameras.size();
	for (size_t index = 0; index < total_cameras; ++index)
	{
		CameraBlend& blend = cameras[index];
		blend.camera->set_target_direction(direction);
	}
}

glm::vec3 CameraMixer::get_target_direction() const
{
	const CameraBlend& blend = cameras.top();
	return blend.camera->get_target_direction();
}

glm::vec3 CameraMixer::get_camera_direction() const
{
	const CameraBlend& blend = cameras.top();
	return blend.camera->get_camera_direction();
}

void CameraMixer::set_world_position(const glm::vec3& new_world_position)
{
	world_position = new_world_position;
}

void CameraMixer::collision_correct(float step_interval_seconds)
{
	CameraBlend& blend = cameras.top();
	blend.camera->collision_correct(step_interval_seconds);
}

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
	if (type == FIRST_PERSON)
	{
		glm::vec3 target = world_pos + view;
		inverse_rotation = glm::lookAt(glm::vec3(), view, up);
		modelview = glm::lookAt(world_pos, target, up );
		eye_position = world_pos;
		inverse_world_transform = modelview;
	}
	else if (type == THIRD_PERSON)
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
	else if (type == CHASE)
	{
		inverse_rotation = glm::lookAt(pos, pos+view, up);
		modelview = inverse_rotation;
		pos = (world_pos + target_offset);

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

void Camera::perspective(real fov_y, int32_t width, int32_t height, real nearz, real farz)
{
	// This MUST be greater than 0, otherwise the view will be inverted or something.
	// Basically, you won't see anything.
	assert( nearz > 0.0f );
	fovy = fov_y;
	aspect_ratio = (float)(width/(float)height);
	near_clip = nearz;
	far_clip = farz;
	projection = glm::perspective(glm::radians(fovy), (width/(float)height), nearz, farz);
	is_ortho = false;
}

void Camera::ortho(real left, real right, real bottom, real top, real nearz, real farz)
{
	this->near_clip = nearz;
	this->far_clip = farz;
	this->aspect_ratio = 1.0f;
	projection = glm::ortho(left, right, bottom, top, nearz, farz);
	is_ortho = true;
}

#if 0
glm::quat ChaseCamera::from_yaw_pitch(float yaw, float pitch)
{
	glm::quat qyaw = glm::angleAxis(glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::quat qpitch = glm::angleAxis(glm::radians(-pitch), glm::vec3(1.0f, 0.0f, 0.0f));
	return glm::normalize(qyaw * qpitch);
}

#endif