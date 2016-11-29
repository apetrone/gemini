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

#include <core/typedefs.h>
#include <core/mathlib.h>
#include <platform/platform.h>

namespace imocap
{
	// inertial motion capture
	// imc library.
	// header (on connect)
	// version
	// # sensors

	// each packet contains data for N sensors
	// plus a 4-byte sequence id.

	// 1. visualization 3d scene
	// 2. record sensor data
	// 3. play back sensor data

	const size_t TOTAL_SENSORS = 3;

	struct MocapDevice
	{
		glm::quat zeroed_orientations[imocap::TOTAL_SENSORS];
	};

	const size_t BNO055_PACKET_SIZE = 20;

	struct bno055_packet_t
	{
		uint8_t header;
		uint8_t data[BNO055_PACKET_SIZE * TOTAL_SENSORS];
		uint8_t footer;

		bno055_packet_t();
		bool is_valid() const;
		uint8_t get_header() const;
		uint8_t get_footer() const;
	}; // bno055_packet_t

	struct mocap_frame_t
	{
		uint32_t frame_index;
		glm::quat poses[imocap::TOTAL_SENSORS];
	};

	// Returns true if timeout_msec has passed since target_msec.
	// If true, sets target_msec to millis().
	bool msec_passed(uint64_t& target_msec, uint32_t timeout_msec);

	void sensor_thread(platform::Thread* thread);

	glm::quat transform_sensor_rotation(const glm::quat& q);

	void zero_rotations(MocapDevice* device);

	glm::quat device_sensor_orientation(MocapDevice* device, size_t sensor_index);
	glm::quat device_sensor_local_orientation(MocapDevice* device, size_t sensor_index);
	glm::vec3 device_sensor_linear_acceleration(MocapDevice* device, size_t sensor_index);
	glm::vec3 device_sensor_gravity(MocapDevice* device, size_t sensor_index);

	MocapDevice* device_create();
	void device_destroy(MocapDevice* device);
} // namespace imocap