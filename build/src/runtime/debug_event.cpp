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
#include <core/atomic.h>
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

				debug_frame_t* frame = &viewer->frames[viewer->current_index];

				//debug_frame_t frame;
				int32_t bytes_available = net_socket_recvfrom(viewer->connection, &source, (char*)frame, sizeof(debug_frame_t));
				viewer->bytes_received += bytes_available;
				//LOGV("read %i bytes\n", bytes_available);
				//LOGV("read frame: %i\n", frame->frame_index);
				//LOGV("sequence: %i\n", record->hitcount);

				{
					debug_frame_t* leframe = frame;
					uint64_t max_cycles = 0;
					uint64_t total_cycles;
					// sort the frame
					for (size_t index = 0; index < TELEMETRY_MAX_RECORDS_PER_FRAME; ++index)
					{
						total_cycles += leframe->records[index].cycles;

						if (leframe->records[index].cycles > max_cycles)
						{
							max_cycles = leframe->records[index].cycles;
						}
					}
					leframe->total_cycles = total_cycles;
					leframe->max_cycles = max_cycles;
				}

				// wrap
				viewer->current_index++;
				if (viewer->current_index >= 240)
				{
					viewer->current_index = 0;
				}
				//
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

		viewer->current_index = 0;
		viewer->is_listening = 1;
		viewer->last_tick = 1.0f;

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

	void telemetry_viewer_tick(telemetry_viewer* viewer, float delta_seconds)
	{
		viewer->last_tick -= delta_seconds;
		if (viewer->last_tick <= 0.0f)
		{
			viewer->bytes_per_second = viewer->bytes_received;
			viewer->bytes_received = 0;
			viewer->last_tick = 1.0f;
		}
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
		//server->total_records = max_records;
		//server->records = static_cast<debug_record_t*>(MEMORY2_ALLOC(server->allocator, sizeof(debug_record_t) * max_records));

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

		//MEMORY2_DEALLOC(server->allocator, server->records);

	}

	void debug_server_begin_frame(debug_server_t* server)
	{
		server->current_record = 0;
		server->current_variable = 0;
	}

	void debug_server_end_frame(debug_server_t* server)
	{
		assert(net_socket_is_valid(server->connection));

		// try and send some data.
		//char data[16] = { 0 };
		//core::str::copy(data, "hello", 0);
		//size_t data_length = 16;
		//int32_t bytes_sent = net_socket_sendto(server->connection, &server->destination, data, data_length);
		//LOGV("bytes_sent = %i\n", bytes_sent);
		static uint32_t sequence = 0;


		//debug_frame_t frame;
		//for (size_t index = 0; index < TELEMETRY_MAX_RECORDS_PER_FRAME; ++index)
		//{
		//	debug_record_t* record = &frame.records[index];
		//	record->filename = __FILE__;
		//	record->function = PLATFORM_FANCY_FUNCTION;
		//	record->line_number = __LINE__;
		//	record->cycles = rand() % 100;
		//	record->hitcount = sequence++;
		//}
		int32_t bytes_sent = net_socket_sendto(server->connection, &server->destination, (const char*)&server->frame, sizeof(debug_frame_t));
		//LOGV("bytes sent = %i\n", bytes_sent);

		for (size_t index = 0; index < TELEMETRY_MAX_RECORDS_PER_FRAME; ++index)
		{
			debug_record_t* record = &server->frame.records[index];
			record->filename = nullptr;
			record->function = nullptr;
			record->line_number = 0;
			record->cycles = 0;
			record->hitcount = 0;
		}

		server->frame.frame_index++;

		for (size_t index = 0; index < TELEMETRY_MAX_VARIABLES; ++index)
		{
			debug_var_t* var = &server->frame.variables[index];
			memset(var, 0, sizeof(debug_var_t));
		}

	} // debug_server_end_frame

	void debug_server_push_record(debug_server_t* server, const char* function, const char* filename, uint64_t cycles, uint32_t line_number)
	{
		uint32_t current_record = server->current_record;
		debug_record_t* record = &server->frame.records[current_record];
		record->cycles = cycles;
		record->filename = filename;
		record->function = function;
		record->line_number = line_number;
		record->hitcount = 0;

		current_record = atom_increment32(&server->current_record);
		if (current_record >= TELEMETRY_MAX_RECORDS_PER_FRAME)
		{
			atom_compare_and_swap32(&server->current_record, 0, current_record);
		}
	} // debug_server_push_record

	void debug_server_push_variable(debug_server_t* server, const char* name, void* data, size_t data_size, uint8_t var_type)
	{
		uint32_t current_variable = server->current_variable;
		debug_var_t* var = &server->frame.variables[current_variable];

		// truncate name to 31 bytes (including terminator)
		core::str::copy(var->name, name, 30);

		assert(data_size <= 32);
		memcpy(var->data, data, data_size);

		var->type = var_type;

		current_variable = atom_increment32(&server->current_variable);
		if (current_variable >= TELEMETRY_MAX_VARIABLES)
		{
			atom_compare_and_swap32(&server->current_variable, 0, current_variable);
		}
	} // debug_server_push_variable

	template <>
	void telemetry_record_variable(debug_server_t* server, const char* name, float* type)
	{
		debug_server_push_variable(server, name, type, sizeof(float), DEBUG_RECORD_TYPE_FLOAT);
	} // telemetry_record_variable

	template <>
	void telemetry_record_variable(debug_server_t* server, const char* name, glm::vec3* type)
	{
		debug_server_push_variable(server, name, type, sizeof(glm::vec3), DEBUG_RECORD_TYPE_FLOAT3);
	} // telemetry_record_variable

	template <>
	void telemetry_record_variable(debug_server_t* server, const char* name, glm::vec4* type)
	{
		debug_server_push_variable(server, name, type, sizeof(glm::vec4), DEBUG_RECORD_TYPE_FLOAT4);
	} // telemetry_record_variable

	template <>
	void telemetry_record_variable(debug_server_t* server, const char* name, uint32_t* type)
	{
		debug_server_push_variable(server, name, type, sizeof(uint32_t), DEBUG_RECORD_TYPE_UINT32);
	} // telemetry_record_variable


} // namespace gemini
