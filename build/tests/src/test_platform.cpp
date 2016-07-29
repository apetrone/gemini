// -------------------------------------------------------------
// Copyright (C) 2015- Adam Petrone
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
#include "unit_test.h"

#include <core/core.h>
#include <core/logging.h>
#include <core/typedefs.h>

#include <platform/platform.h>

#include <platform/audio.h>

using namespace platform;

#if defined(GEMINI_ENABLE_AUDIO)
// ---------------------------------------------------------------------
// audio
// ---------------------------------------------------------------------

#include <core/mathlib.h>
struct audio_generator_data
{
	uint32_t time_period;
	float t_sin;
	uint32_t wave_type; // 0 for square wave, 1 for sine wave
};

void test_callback(void* data, size_t frames_available, size_t sample_rate_hz, void* context)
{
	uint32_t max_index = 0;
	audio_generator_data* agd = static_cast<audio_generator_data*>(context);
	if (agd->wave_type == 0)
	{
		// generate a square wave
		const float volume = 0.5f;

		const uint32_t frames_per_period = sample_rate_hz / 256;
		const uint32_t half_period = frames_per_period / 2;
		const uint32_t total_frames = frames_available;

		int16_t* buffer = reinterpret_cast<int16_t*>(data);
		for (uint32_t frame = 0; frame < total_frames; ++frame)
		{
			short value = ((agd->time_period / half_period) % 2) ? 3000 : -3000;
			uint32_t index = frame * 2;
			buffer[index + 0] = static_cast<short>(value * volume);
			buffer[index + 1] = static_cast<short>(value * volume);
			max_index = index + 1;
			agd->time_period += 1;
			if (agd->time_period > frames_per_period)
			{
				agd->time_period -= frames_per_period;
			}
		}
	}
	else if (agd->wave_type == 1)
	{
		// generate a sin wave
		const float volume = 0.5f;
		const float wave_period = (sample_rate_hz / 440.0f);
		const float sin_per = 2.0f * mathlib::PI;

		int16_t* buffer = reinterpret_cast<int16_t*>(data);
		for (uint32_t frame = 0; frame < frames_available; ++frame)
		{
			float sin_value = sinf(agd->t_sin);
			short value = static_cast<short>(sin_value * 6000);
			uint32_t index = frame * 2;
			buffer[index + 0] = static_cast<short>(value * volume);
			buffer[index + 1] = static_cast<short>(value * volume);

			max_index = (index + 1);
			// increment the t_sin value
			agd->t_sin += static_cast<float>((1.0 * sin_per) / wave_period);

			// make sure it wraps to avoid glitches.
			if (agd->t_sin > sin_per)
			{
				agd->t_sin -= sin_per;
			}
		}
	}

	platform::audio_frame_position();
}

UNITTEST(audio)
{
	audio_startup();

	audio_generator_data agd;
	agd.time_period = 0;
	agd.t_sin = 0.0f;
	agd.wave_type = 0;

	Array<audio_device*> devices;
	audio_enumerate_devices(devices);

	LOGV("total audio devices: %i\n", devices.size());
	for (size_t index = 0; index < devices.size(); ++index)
	{
		audio_device* device = devices[index];
		LOGV("device: %s\n", device->name());
	}

	if (devices.size() > 0)
	{
		// try to open the device
		platform::Result res = audio_open_output_device(devices[0]);
		if (res.succeeded())
		{
			audio_set_callback(test_callback, &agd);

			// play a square wave
			uint64_t start = platform::microseconds();

			uint64_t elapsed = 0;

			LOGV("now playing square wave\n");
			while (elapsed < (1000 * MicrosecondsPerMillisecond))
			{
				elapsed = (platform::microseconds() - start);
			}

			LOGV("now playing sine wave: %2.2f\n", elapsed * SecondsPerMicrosecond);

			// play a sine wave
			agd.wave_type = 1;

			start = platform::microseconds();
			elapsed = 0;

			while (elapsed < (1000 * MicrosecondsPerMillisecond))
			{
				elapsed = (platform::microseconds() - start);
			}

			LOGV("finished playing sound %2.2f seconds elapsed.\n", elapsed * SecondsPerMicrosecond);

			audio_close_output_device();
		}
		else
		{
			LOGE("Audio Open Output Device failed: %s\n", res.message);
		}
	}
	else
	{
		LOGE("No audio devices found.\n");
	}

	audio_shutdown();
}

#endif // GEMINI_ENABLE_AUDIO

// ---------------------------------------------------------------------
// platform
// ---------------------------------------------------------------------
UNITTEST(platform)
{
	LOGV("PLATFORM_NAME: %s\n", PLATFORM_NAME);
	LOGV("PLATFORM_COMPILER: %s, version: %s\n", PLATFORM_COMPILER, PLATFORM_COMPILER_VERSION);
}

// ---------------------------------------------------------------------
// dynamic library
// ---------------------------------------------------------------------
UNITTEST(dynamic_library)
{
	const char* library_extension = platform::dylib_extension();
	TEST_ASSERT(library_extension, dylib_extension);
}

// ---------------------------------------------------------------------
// filesystem
// ---------------------------------------------------------------------
UNITTEST(filesystem)
{
	platform::PathString filename = platform::get_program_directory();

	// see if we can verify directories exist; otherwise, we cannot verify
	// other functions
	LOGV("checking program directory exists: '%s'\n", filename());
	TEST_ASSERT(platform::fs_directory_exists(filename()), fs_directory_exists);

	filename.append(PATH_SEPARATOR_STRING);
	filename.append("test.conf");

	platform::File file = platform::fs_open(filename(), platform::FileMode_Write);
	TEST_ASSERT(file.handle != 0, fs_open_for_write);
	TEST_ASSERT(file.is_open(), file_is_open);

	size_t bytes_written = 0;
	if (file.is_open())
	{
		const char buffer[] = "Hello, this is a test\n";
		bytes_written = platform::fs_write(file, buffer, 22, 1);
		platform::fs_close(file);
	}
	TEST_ASSERT(bytes_written == 22, fs_write);

	file = platform::fs_open(filename(), platform::FileMode_Read);
	TEST_ASSERT(file.handle != 0, fs_open_for_read);
	size_t bytes_read = 0;
	if (file.is_open())
	{
		core::StackString<32> buffer;
		bytes_read = platform::fs_read(file, &buffer[0], 22, 1);
		platform::fs_close(file);
	}
	TEST_ASSERT(bytes_read == 22, fs_read);


	platform::PathString content_directory = platform::fs_content_directory();
	TEST_ASSERT(!content_directory.is_empty(), fs_content_directory);

	// test directories
	platform::Result result;
	platform::PathString program_directory = platform::get_program_directory();
	TEST_ASSERT(!program_directory.is_empty(), get_program_directory);

	// this currently fails on subsequent runs because the directory
	// is never removed.
	result = platform::make_directory("test_directory");
	TEST_ASSERT(result.succeeded(), make_directory);

	result = platform::remove_directory("test_directory");
	TEST_ASSERT(result.succeeded(), remove_directory);

	const char* user_home = nullptr;
#if defined(PLATFORM_WINDOWS)
	user_home = platform::get_environment_variable("%USERPROFILE%");
#elif defined(PLATFORM_APPLE) || defined(PLATFORM_LINUX)
	user_home = platform::get_environment_variable("HOME");
#else
	#error I do not know how to test this platform.
#endif

	TEST_ASSERT(user_home != nullptr, get_environment_variable);
//	LOGV("get_environment_variable [%%USERPROFILE%% / $HOME]: '%s'\n", user_home);


	platform::PathString user_directory = platform::get_user_directory();
	TEST_ASSERT(!user_directory.is_empty(), get_user_directory);
//	LOGV("get_user_directory: '%s'\n", user_directory);


	platform::PathString temp_directory = platform::get_user_temp_directory();
	TEST_ASSERT(!temp_directory.is_empty(), get_user_temp_directory);

	platform::PathString abs_path = platform::make_absolute_path("./test");
	TEST_ASSERT(abs_path.size() > 5, make_absolute_path);

}

// ---------------------------------------------------------------------
// network
// ---------------------------------------------------------------------

static bool net_listen_thread = true;

const size_t TOTAL_SENSORS = 2;

struct bno055_packet_t
{
	uint8_t header;
	uint8_t data[8 * TOTAL_SENSORS];
	uint8_t footer;

	bno055_packet_t()
	{
		header = 0xba;
		footer = 0xff;
	}

	bool is_valid() const
	{
		return (header == 0xba) && (footer == 0xff);
	}

	glm::quat get_orientation(uint32_t sensor_index = 0) const
	{
		int16_t x = 0;
		int16_t y = 0;
		int16_t z = 0;
		int16_t w = 0;

		const uint8_t* buffer = (data + (sensor_index * 8));

		// they are 16-bit LSB
		x = (((uint16_t)buffer[3]) << 8) | ((uint16_t)buffer[2]);
		y = (((uint16_t)buffer[5]) << 8) | ((uint16_t)buffer[4]);
		z = (((uint16_t)buffer[7]) << 8) | ((uint16_t)buffer[6]);
		w = (((uint16_t)buffer[1]) << 8) | ((uint16_t)buffer[0]);

		const double QUANTIZE = (1.0 / 16384.0);

		return glm::quat(w * QUANTIZE, x * QUANTIZE, y * QUANTIZE, z * QUANTIZE);
	}
}; // bno055_packet_t

// Returns true if timeout_msec has passed since target_msec.
// If true, sets target_msec to millis().
bool msec_passed(uint64_t& target_msec, uint32_t timeout_msec)
{
	uint64_t current_milliseconds = platform::microseconds() * MillisecondsPerMicrosecond;
	assert(current_milliseconds >= target_msec);
	if ((current_milliseconds - target_msec) > timeout_msec)
	{
		//LOGV("msec_passed: %d ms\n", (current_milliseconds - target_msec));
		target_msec = current_milliseconds;
		return true;
	}

	return false;
} // msec_passed

void test_network_thread(platform::Thread* thread)
{
	net_socket* sock = static_cast<net_socket*>(thread->user_data);
	LOGV("launched network listen thread.\n");

	const size_t PACKET_SIZE = sizeof(bno055_packet_t);
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


	while (net_listen_thread)
	{
		net_address source;

		uint64_t current_milliseconds = platform::microseconds() * MillisecondsPerMicrosecond;

		if (current_state == STATE_STREAMING)
		{
			if (msec_passed(last_client_contact_msec, CLIENT_TIMEOUT_MSEC))
			{
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

		int select_result = select((*sock)+1, &receive, &transmit, nullptr, &zero_timeval);
		assert(select_result >= 0);

		if (FD_ISSET(*sock, &receive))
		{
			int32_t bytes_available = net_socket_recvfrom(*sock, &source, buffer, PACKET_SIZE);
			if (bytes_available > 0)
			{
				if (current_state == STATE_STREAMING)
				{
					last_client_contact_msec = current_milliseconds;
					bno055_packet_t* packet = reinterpret_cast<bno055_packet_t*>(buffer);
					if (packet->is_valid())
					{
						for (size_t index = 0; index < TOTAL_SENSORS; ++index)
						{
							glm::quat q = packet->get_orientation(index);
							LOGV("q[%i]: %2.2f, %2.2f, %2.2f, %2.2f\n", index, q.x, q.y, q.z, q.w);
						}
					}
					else
					{
						LOGV("Received invalid packet!\n");
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
						net_socket_sendto(*sock, &source, (const char*)&HANDSHAKE_VALUE, sizeof(uint32_t));
						current_state = STATE_HANDSHAKE;
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
						net_socket_sendto(*sock, &source, (const char*)response, sizeof(uint32_t));

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
			}
		}
	}
}

#if 0
void test_network_thread(platform::Thread* thread)
{
	net_socket* sock = static_cast<net_socket*>(thread->user_data);
	LOGV("launched network listen thread.\n");
	while (net_listen_thread)
	{
		net_address source;
		char buffer[256] = { 0 };
		const size_t BUFFER_SIZE = 255;
		int32_t bytes_available = net_socket_recvfrom(*sock, &source, buffer, BUFFER_SIZE);
		if (bytes_available > 0)
		{
			LOGV("read: %s\n", buffer);
		}
	}
}
#endif

UNITTEST(network)
{
	int32_t result = net_startup();
	TEST_ASSERT(result == 0, net_startup);


	// create a socket we'll use for UDP receive.

	net_socket sock1 = net_socket_open(net_socket_type::UDP);
	TEST_ASSERT(net_socket_is_valid(sock1), net_socket_open);

	net_address interface;
	net_address_init(&interface);
	net_address_set(&interface, "127.0.0.1", 27015);


	// net_socket_set_blocking(sock1, 0);
	int32_t bind_result = net_socket_bind(sock1, &interface);
	TEST_ASSERT(bind_result == 0, net_socket_bind);

	platform::Thread* handle = platform::thread_create(test_network_thread, &sock1);

	LOGV("Closing thread in 5 seconds...\n");
	platform::thread_sleep(30000);

	net_listen_thread = false;

	platform::thread_sleep(200);

	platform::thread_join(handle, 1000);
	platform::thread_destroy(handle);

	net_shutdown();
}

// ---------------------------------------------------------------------
// serial
// ---------------------------------------------------------------------
UNITTEST(serial)
{

}

// ---------------------------------------------------------------------
// system
// ---------------------------------------------------------------------
UNITTEST(system)
{
	size_t page_size = platform::system_pagesize_bytes();

	LOGV("page size: %i bytes\n", page_size);
	TEST_ASSERT(page_size > 0, page_size);

	size_t total_processors = platform::system_processor_count();
	LOGV("total processors: %i\n", total_processors);
	TEST_ASSERT(total_processors >= 1, system_processor_count);

	double uptime_seconds = platform::system_uptime_seconds();
	LOGV("system_uptime_seconds: %f\n", uptime_seconds);
	TEST_ASSERT(uptime_seconds > 0, system_uptime_seconds);

	core::StackString<64> version = platform::system_version_string();
	LOGV("system_version_string: %s\n", version());
	TEST_ASSERT(!version.is_empty(), system_version_string);
}

// ---------------------------------------------------------------------
// thread
// ---------------------------------------------------------------------
void test_thread(platform::Thread* /*data*/)
{
	platform::ThreadId thread_id = platform::thread_id();
	LOGV("test_thread enter: %i\n", thread_id);

	LOGV("test_thread exit\n");
}

UNITTEST(thread)
{
	platform::Thread* thread = platform::thread_create(test_thread, nullptr);
	TEST_ASSERT(thread, thread_create);

	platform::thread_join(thread);

	platform::thread_destroy(thread);
}

// ---------------------------------------------------------------------
// time
// ---------------------------------------------------------------------
UNITTEST(datetime)
{
	platform::DateTime dt;
	platform::datetime(dt);

	bool maybe_valid = \
		(dt.day != 0 ||
		dt.dayOfWeek != 0 ||
		dt.hour != 0 ||
		dt.milliseconds != 0 ||
		dt.minute != 0 ||
		dt.month != 0 ||
		dt.second != 0) &&
	dt.year != 0;

	// this isn't something I know how to reliably test. here goes nothing.
	TEST_ASSERT(maybe_valid, datetime_sanity);

	uint64_t us = platform::microseconds();
	TEST_ASSERT(us != 0, microseconds);

	LOGV("waiting three seconds...\n");

	uint64_t last = us;
	while((last - us) < (3 * MicrosecondsPerSecond))
	{
		last = platform::microseconds();
	}

	LOGV("three seconds have passed!\n");

	LOGV("ticks: %zu\n", time_ticks());
}

int main(int, char**)
{
	gemini::core_startup();
	//unittest::UnitTest::execute();
	TEST_EXECUTE(network);

	gemini::core_shutdown();
	return 0;
}
