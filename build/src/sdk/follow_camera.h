// -------------------------------------------------------------
// Copyright (C) 2016- Adam Petrone
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

#include "camera.h"

// Inspired by and taking heavy design influence from Yoann Pignole's
// article on third person camera design.
// http://www.gamasutra.com/blogs/YoannPignole/20150928/249412/Third_person_camera_design_with_free_move_zone.php
class FollowCamera : public GameCamera
{
private:
	float yaw;
	float pitch;

	// TODO: replace this with a spline
	float pitch_min;
	float pitch_max;

	glm::vec3 position;
	glm::vec3 desired_position;
	glm::vec3 view;
	glm::vec3 target_position;
	glm::vec3 velocity;

	// did the view move this tick?
	size_t view_moved;

	float follow_distance;

	void update_desired_position(const glm::vec3& desired_direction);
	void direction_to_spherical(const glm::vec3& direction);

public:
	FollowCamera();
	virtual ~FollowCamera();

	virtual glm::vec3 get_origin() const override;
	virtual glm::vec3 get_target() const override;
	virtual glm::vec3 get_up() const override;
	virtual glm::vec3 get_right() const override;
	virtual float get_fov() const override;
	virtual CameraType get_type() const override { return CameraType::FollowCamera; }
	virtual void move_view(float yaw, float pitch) override;
	virtual void set_yaw_pitch(float yaw, float pitch) override;
	virtual void tick(float step_interval_seconds) override;
	virtual void set_target_position(const glm::vec3& target_worldspace_position) override;
	virtual void set_target_direction(const glm::vec3& direction) override;
	virtual glm::vec3 get_target_direction() const override;
	virtual void reset_view() override;
};
