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
#include <platform/network.h>
#include <rapidjson/rapidjson.h>

namespace gemini
{
	const size_t HTTP_STATUS_OK = 200;
	const size_t HTTP_STATUS_FORBIDDEN = 403;
	const size_t HTTP_STATUS_NOT_FOUND = 404;
	const size_t HTTP_STATUS_INTERNAL_SERVER_ERROR = 500;

	const size_t HTTP_HEADER_MAX_SIZE = 512;


	enum HttpFlag
	{
		HTTP_FLAG_ACTIVE = 1,
		HTTP_FLAG_READ_HEADERS = 2,
		HTTP_FLAG_READ_CONTENT = 4,
		HTTP_FLAG_ERROR = 8
	};

	struct http_request
	{
		int32_t status;
		char message[32];
		int32_t protocol_major;
		int32_t protocol_minor;
	};

	struct http_download_state
	{
		size_t content_length;
		size_t content_bytes_read;

		// total bytes read from remote
		uint32_t bytes_read;

		// total bytes sent to remote
		uint32_t bytes_sent;

		uint32_t header_length;

		uint16_t flags;

		platform::net_socket socket;
		platform::File handle;

		char header_data[HTTP_HEADER_MAX_SIZE];

		void* userdata;
	};

	bool http_startup();
	void http_shutdown();

	// update active requests
	void http_update();

	http_download_state* http_request_file(const char* url, const char* temp_path, const char* user_agent);
	int32_t http_process_headers(http_download_state* state, const char* lines, size_t header_length, http_request* request);
	uint32_t http_active_download_count();

	uint32_t http_get_request(const char* url);

} // namespace gemini
