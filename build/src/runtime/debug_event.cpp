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

#include <runtime/debug_event.h>

#include <core/array.h>
#include <core/logging.h>
#include <core/mem.h>

using namespace platform;

namespace gemini
{
	void viewer_thread(platform::Thread* thread)
	{
		telemetry_viewer* viewer = static_cast<telemetry_viewer*>(thread->user_data);

		LOGV("launched viewer listen thread.\n");

		while (viewer->is_listening)
		{
			timeval zero_timeval;
			zero_timeval.tv_sec = 0;
			zero_timeval.tv_usec = 1;

			fd_set receive;
			FD_ZERO(&receive);
			FD_SET(viewer->connection, &receive);

			fd_set transmit;
			FD_ZERO(&transmit);
			FD_SET(viewer->connection, &transmit);

			int highest_socket = viewer->connection + 1;

			int select_result = select(highest_socket, &receive, &transmit, nullptr, &zero_timeval);
			assert(select_result >= 0);

			if (FD_ISSET(viewer->connection, &receive))
			{
				net_address source;
				debug_frame_t frame;
				int32_t bytes_available = net_socket_recvfrom(viewer->connection, &source, (char*)&frame, sizeof(debug_frame_t));
				LOGV("read %i bytes\n", bytes_available);
			}
		}

		LOGV("leaving viewer thread\n");
	}

	void telemetry_viewer_create(telemetry_viewer* viewer, uint32_t frame_history_size, const char* ip_address, uint16_t port)
	{
		net_address address;
		net_address_set(&address, ip_address, port);

		viewer->connection = net_socket_open(net_socket_type::UDP);
		if (!net_socket_is_valid(viewer->connection))
		{
			LOGW("Unable to create viewer socket.\n");
			return;
		}

		int32_t bind_result = net_socket_bind(viewer->connection, &address);
		if (bind_result != 0)
		{
			LOGW("Unable to bind telemetry viewer socket.\n");
			net_socket_close(viewer->connection);
			viewer->connection = -1;
			return;
		}

		viewer->is_listening = 1;

		// now create a listener thread
		viewer->listener_thread  = platform::thread_create(viewer_thread, viewer);
	}

	void telemetry_viewer_destroy(telemetry_viewer* viewer)
	{
		viewer->is_listening = 0;

		if (net_socket_is_valid(viewer->connection))
		{
			net_socket_shutdown(viewer->connection, net_socket_how::READ);
			net_socket_close(viewer->connection);
			viewer->connection = -1;
		}

		platform::thread_join(viewer->listener_thread, 2);
		platform::thread_destroy(viewer->listener_thread);
	}


	//void debug_record(debug_record_t* record, const char* name, float input_value)
	//{
	//	core::str::copy(record->name, name, 0);
	//	float* value = reinterpret_cast<float*>(record->data);
	//	*value = input_value;
	//	record->type = DEBUG_RECORD_TYPE_FLOAT;
	//}

	void debug_server_create(debug_server_t* server, uint32_t max_records, const char* ip_address, uint16_t port)
	{
		memset(server, 0, sizeof(debug_server_t));
		server->allocator = memory_allocator_default(MEMORY_ZONE_DEFAULT);
		server->total_records = max_records;
		server->records = static_cast<debug_record_t*>(MEMORY2_ALLOC(server->allocator, sizeof(debug_record_t) * max_records));

		server->connection = net_socket_open(net_socket_type::UDP);
		if (!net_socket_is_valid(server->connection))
		{
			LOGW("Unable to create telemetry source socket.\n");
			return;
		}

		net_address_set(&server->destination, ip_address, port);
	}

	void debug_server_destroy(debug_server_t* server)
	{
		if (net_socket_is_valid(server->connection))
		{
			net_socket_shutdown(server->connection, net_socket_how::READ_WRITE);
			net_socket_close(server->connection);
			server->connection = -1;
		}

		MEMORY2_DEALLOC(server->allocator, server->records);

	}

	void debug_server_begin_frame(debug_server_t* server)
	{

	}

	void debug_server_end_frame(debug_server_t* server)
	{
		assert(net_socket_is_valid(server->connection));

		// try and send some data.
		char data[16] = { 0 };
		core::str::copy(data, "hello", 0);
		size_t data_length = 16;
		int32_t bytes_sent = net_socket_sendto(server->connection, &server->destination, data, data_length);
		LOGV("bytes_sent = %i\n", bytes_sent);
	}

	void debug_server_push_record(debug_server_t* server, debug_record_t* record)
	{
	}
} // namespace gemini
