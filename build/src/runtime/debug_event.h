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
#pragma once

#include <core/mem.h>
#include <core/str.h>

#include <core/array.h>
#include <core/mathlib.h>

#include <platform/network.h>
#include <platform/platform.h>

/*
Telemetry interface:
The telemetry system is composed of a Source and Viewer.
The Source is where the data originates from, usually your application under
test.
The Viewer is a separate tool used to collect and view the data.


Usage:

I want to collect data per frame and expose a user interface to drill down
and further examine details.

This can rapidly just ship off the data to a tool so it doesn't need to remain
in the memory of the engine.

This should be thread-safe and report which thread it was recorded on.


*/

namespace gemini
{
	enum
	{
		DEBUG_RECORD_TYPE_FLOAT,
		DEBUG_RECORD_TYPE_UINT32,
		DEBUG_RECORD_TYPE_INT32
	};

	const uint16_t TELEMETRY_VIEWER_PORT = 12807;

	struct debug_record_t
	{
		//char name[31];
		//uint8_t type;
		//char data[4];

		const char* function;
		const char* filename;
		uint64_t cycles;
		uint32_t line_number;
		uint32_t hitcount;
	};

	struct debug_frame_t
	{
		debug_record_t records[4];
		//uint32_t total_records;
	};

	struct telemetry_viewer
	{
		debug_frame_t history[240];
		//uint32_t total_frames;
		platform::net_socket connection;
		uint32_t is_listening;
		platform::Thread* listener_thread;
	};

	void telemetry_viewer_create(telemetry_viewer* client, uint32_t history_size_frames, const char* ip_address, uint16_t port);
	void telemetry_viewer_destroy(telemetry_viewer* client);

	//void debug_record(debug_record_t* record, const char* name, float input_value);


	struct debug_server_t
	{
		debug_record_t* records;
		uint32_t total_records;
		uint32_t current_record;
		gemini::Allocator allocator;
		platform::net_socket connection;
		platform::net_address destination;
		uint32_t is_running;
		platform::Thread* transmit_thread;
	};

	void debug_server_create(debug_server_t* server, uint32_t max_records, const char* ip_address, uint16_t port);
	void debug_server_destroy(debug_server_t* server);
	void debug_server_begin_frame(debug_server_t* server);
	void debug_server_end_frame(debug_server_t* server);
	void debug_server_push_record(debug_server_t* server, debug_record_t* record);
} // namespace gemini
