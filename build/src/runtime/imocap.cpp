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

#include <runtime/imocap.h>
#include <core/logging.h>
#include <core/mem.h>
#include <platform/platform.h>

using namespace platform;

namespace imocap
{
	bool net_listen_thread = true;

	net_socket data_socket;
	platform::Thread* sensor_thread_handle;
	MocapDevice* _current_device = nullptr;
	gemini::Allocator* _allocator = nullptr;

	void startup(gemini::Allocator& allocator)
	{
		_allocator = &allocator;
	}

	void shutdown()
	{
		_allocator = nullptr;
	}

	// Returns true if timeout_msec has passed since target_msec.
	// If true, sets target_msec to millis().
	bool msec_passed(uint64_t& target_msec, uint32_t timeout_msec)
	{
		uint64_t current_milliseconds = platform::microseconds() * MillisecondsPerMicrosecond;
		assert(current_milliseconds >= target_msec);
		if ((current_milliseconds - target_msec) > timeout_msec)
		{
			target_msec = current_milliseconds;
			return true;
		}

		return false;
	} // msec_passed

	void sensor_thread(platform::Thread* thread)
	{
		net_socket* sock = static_cast<net_socket*>(thread->user_data);
		LOGV("launched network listen thread.\n");

		const size_t PACKET_SIZE = sizeof(imocap_packet_t);
		LOGV("packet size is %i bytes\n", PACKET_SIZE);

		const size_t MAX_PACKET_DATA = 4 * PACKET_SIZE;
		char buffer[MAX_PACKET_DATA];
		size_t current_index = 0;

		// Waiting for a client to connect. Reply to broadcasts.
		const uint8_t STATE_WAITING = 1;

		// Waiting for the client to accept handshake.
		const uint8_t STATE_HANDSHAKE = 2;

		// Streaming with a client.
		const uint8_t STATE_STREAMING = 3;

		const uint32_t HANDSHAKE_VALUE = 1985;

		uint8_t current_state = STATE_WAITING;

		uint64_t last_client_ping_msec = 0;

		// Time to wait until sending a 'ping' back to the client to force
		// a keep-alive.
		const uint64_t CLIENT_PING_DELAY_MSEC = 1000;
		net_address client_address;

		const uint64_t CLIENT_TIMEOUT_MSEC = 3000;
		uint64_t last_client_contact_msec = 0;

		const uint32_t DISCONNECT_VALUE = 2005;


		while (net_listen_thread)
		{
			net_address source;

			uint64_t current_milliseconds = platform::microseconds() * MillisecondsPerMicrosecond;

			if (current_state == STATE_STREAMING)
			{
				if (msec_passed(last_client_contact_msec, CLIENT_TIMEOUT_MSEC))
				{
					LOGV("No communication from client in %i msec\n", CLIENT_TIMEOUT_MSEC);
					LOGV("Assuming the client is dead. Waiting for connections...\n");
					// Assume the client is dead.
					current_state = STATE_WAITING;
					continue;
				}

				if (msec_passed(last_client_ping_msec, CLIENT_PING_DELAY_MSEC))
				{
					uint32_t ping_value = 2000;
					net_socket_sendto(*sock, &client_address, (const char*)&ping_value, sizeof(uint32_t));
				}
			}

			timeval zero_timeval;
			zero_timeval.tv_sec = 0;
			zero_timeval.tv_usec = 1;

			fd_set receive;
			FD_ZERO(&receive);
			FD_SET(*sock, &receive);

			fd_set transmit;
			FD_ZERO(&transmit);
			FD_SET(*sock, &transmit);

			int select_result = select((*sock) + 1, &receive, &transmit, nullptr, &zero_timeval);
			assert(select_result >= 0);

			if (FD_ISSET(*sock, &receive))
			{
				int32_t bytes_available = net_socket_recvfrom(*sock, &source, buffer, PACKET_SIZE);
				if (bytes_available > 0)
				{
					//LOGV("-> received %i bytes...\n", bytes_available);
					last_client_contact_msec = current_milliseconds;
					if (current_state == STATE_STREAMING)
					{
						imocap_packet_t* packet = reinterpret_cast<imocap_packet_t*>(buffer);
						if (imocap_packet_is_valid(packet) && _current_device)
						{
							for (size_t index = 0; index < IMOCAP_TOTAL_SENSORS; ++index)
							{
								// Copy data from packet
								const double QUANTIZE = (1.0 / 16384.0);
								const uint8_t* buffer = imocap_packet_sensor_data_at(packet, index);

								// Construct Quaternion
								{
									int16_t x = 0;
									int16_t y = 0;
									int16_t z = 0;
									int16_t w = 0;

									// they are 16-bit LSB
									x = (((int16_t)buffer[3]) << 8) | ((int16_t)buffer[2]);
									y = (((int16_t)buffer[5]) << 8) | ((int16_t)buffer[4]);
									z = (((int16_t)buffer[7]) << 8) | ((int16_t)buffer[6]);
									w = (((int16_t)buffer[1]) << 8) | ((int16_t)buffer[0]);

									_current_device->sensors[index] = glm::quat(w * QUANTIZE, x * QUANTIZE, y * QUANTIZE, z * QUANTIZE);
									buffer += 8;
								}

								// seek the linear acceleration data.
								{
									int16_t x = (((int16_t)buffer[1]) << 8) | ((int16_t)buffer[0]);
									int16_t y = (((int16_t)buffer[3]) << 8) | ((int16_t)buffer[2]);
									int16_t z = (((int16_t)buffer[5]) << 8) | ((int16_t)buffer[4]);

									_current_device->linear_acceleration[index] = glm::vec3(x * QUANTIZE, y * QUANTIZE, z * QUANTIZE);
									buffer += 6;
								}

								// Seek the gravity data
								{
									int16_t x = (((int16_t)buffer[1]) << 8) | ((int16_t)buffer[0]);
									int16_t y = (((int16_t)buffer[3]) << 8) | ((int16_t)buffer[2]);
									int16_t z = (((int16_t)buffer[5]) << 8) | ((int16_t)buffer[4]);

									_current_device->gravity[index] = glm::vec3(x * QUANTIZE, y * QUANTIZE, z * QUANTIZE);
									buffer += 6;
								}
								//LOGV("q[%i]: %2.2f, %2.2f, %2.2f, %2.2f\n", index, q.x, q.y, q.z, q.w);

								//if (index == 2)
								//{
								//	LOGV("accel: %2.2f, %2.2f, %2.2f\n", gravity[index].x, gravity[index].y, gravity[index].z);
								//}
							}
						}
						else
						{
							LOGV("Received invalid packet! (header = %x, footer = %x)\n", packet->header, packet->footer);
						}
					}
					else if (current_state == STATE_WAITING)
					{
						uint32_t request = (*reinterpret_cast<uint32_t*>(buffer));
						if (request == 1983)
						{
							char ip[22] = { 0 };
							net_address_host(&source, ip, 22);
							uint16_t port = net_address_port(&source);
							LOGV("Mocap client at %s:%i; initiating handshake (%i)...\n", ip, port, HANDSHAKE_VALUE);
							current_state = STATE_HANDSHAKE;
							net_socket_sendto(*sock, &source, (const char*)&HANDSHAKE_VALUE, sizeof(uint32_t));
						}
						else
						{
							LOGW("Request value is invalid.\n");
						}
					}
					else if (current_state == STATE_HANDSHAKE)
					{
						LOGV("received data while handshaking: %i\n", bytes_available);
						uint32_t request = (*reinterpret_cast<uint32_t*>(buffer));
						if (request == HANDSHAKE_VALUE)
						{
							char ip[22] = { 0 };
							net_address_host(&source, ip, 22);
							uint16_t port = net_address_port(&source);
							LOGV("Connected with mocap client at %s:%i.\n", ip, port);
							uint32_t response = 65535;
							net_socket_sendto(*sock, &source, (const char*)&response, sizeof(uint32_t));

							net_address_set(&client_address, ip, port);
							last_client_contact_msec = platform::microseconds() * MillisecondsPerMicrosecond;

							current_state = STATE_STREAMING;
						}
						else
						{
							LOGV("handshake did not match! (received: %i, expected: %i)\n", request, HANDSHAKE_VALUE);
							current_state = STATE_WAITING;
						}
					}

					// This isn't received by the Feather on shutdown.
					//else if (current_state == STATE_SHUTDOWN)
					//{
					//	// send disconnect
					//	int32_t value = DISCONNECT_VALUE;
					//	net_socket_sendto(*sock, &client_address, (const char*)&value, sizeof(uint32_t));
					//	thread_sleep(500);
					//}
				}
			}
		}


	} // sensor_thread

	glm::quat transform_sensor_rotation(const glm::quat& q)
	{
		// this bit of code converts the coordinate system of the BNO055
		// to that of gemini.
		glm::quat flipped(q.w, q.x, -q.z, -q.y);
		glm::quat y = glm::quat(glm::vec3(0, mathlib::degrees_to_radians(180), 0));
		return glm::inverse(y * flipped);
	}

	void zero_rotations(MocapDevice* device)
	{
		for (size_t index = 0; index < IMOCAP_TOTAL_SENSORS; ++index)
		{
			device->zeroed_orientations[index] = transform_sensor_rotation(device->sensors[index]);

			device->zeroed_accelerations[index] = device_sensor_linear_acceleration(device, index);
		}
	}

	glm::quat device_sensor_orientation(MocapDevice* device, size_t sensor_index)
	{
		return transform_sensor_rotation(device->sensors[sensor_index]);
	}

	glm::quat device_sensor_local_orientation(MocapDevice* device, size_t sensor_index)
	{
		return glm::inverse(device->zeroed_orientations[sensor_index]) * transform_sensor_rotation(device->sensors[sensor_index]);
	}

	glm::vec3 device_sensor_linear_acceleration(MocapDevice* device, size_t sensor_index)
	{
		return device->linear_acceleration[sensor_index];
	}

	glm::vec3 device_sensor_local_acceleration(MocapDevice* device, size_t sensor_index)
	{
		return device->linear_acceleration[sensor_index] - device->zeroed_accelerations[sensor_index];
	}

	glm::vec3 device_sensor_gravity(MocapDevice* device, size_t sensor_index)
	{
		return device->gravity[sensor_index];
	}

	MocapDevice* device_create()
	{
		sensor_thread_handle = nullptr;
		data_socket = -1;

		data_socket = net_socket_open(net_socket_type::UDP);
		assert(net_socket_is_valid(data_socket));

		net_address addr;
		net_address_init(&addr);
		net_address_set(&addr, "0.0.0.0", 27015);

		int32_t bind_result = net_socket_bind(data_socket, &addr);
		assert(bind_result == 0);

		net_listen_thread = true;
		sensor_thread_handle = platform::thread_create(sensor_thread, &data_socket);

		assert(_current_device == nullptr);

		MocapDevice* device = MEMORY2_NEW((*_allocator), MocapDevice);
		_current_device = device;

		return device;
	} // device_create

	void device_destroy(MocapDevice* device)
	{
		MEMORY2_DELETE((*_allocator), device);
		_current_device = nullptr;
		net_listen_thread = false;
		platform::thread_join(sensor_thread_handle, 1000);
		platform::thread_destroy(sensor_thread_handle);
	} // device_destroy
} // namespace imocap
