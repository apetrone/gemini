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
#include "typedefs.h"

#include "http.h"

#include <core/logging.h>
#include <core/str.h>
#include <core/config.h>

#include <runtime/runtime.h>

using namespace platform;

namespace gemini
{
	const size_t HTTP_MAX_CONCURRENT_DOWNLOADS = 4;
	http_download_state _states[HTTP_MAX_CONCURRENT_DOWNLOADS];

	bool http_startup()
	{
		for (size_t index = 0; index < HTTP_MAX_CONCURRENT_DOWNLOADS; ++index)
		{
			memset(&_states[index], 0, sizeof(http_download_state));
		}

		return true;
	} // http_startup

	void http_shutdown()
	{
		for (size_t index = 0; index < HTTP_MAX_CONCURRENT_DOWNLOADS; ++index)
		{
			const bool is_active = (_states[index].flags & HTTP_FLAG_ACTIVE);
			if (is_active)
			{
				net_socket_close(_states[index].socket);
				_states[index].flags &= ~HTTP_FLAG_ACTIVE;
			}
		}
	} // http_shutdown

	void http_update()
	{
		fd_set read_set;
		fd_set except_set;

		FD_ZERO(&read_set);
		FD_ZERO(&except_set);

		timeval select_timeout;

		int last_socket = -1;

		for (size_t index = 0; index < HTTP_MAX_CONCURRENT_DOWNLOADS; ++index)
		{
			const bool is_active = (_states[index].flags & HTTP_FLAG_ACTIVE);
			if (is_active)
			{
				last_socket = 1 + _states[index].socket;
				FD_SET(_states[index].socket, &read_set);
			}
		}

		const size_t HTTP_BUFFER_SIZE = 32768;
		char buffer[HTTP_BUFFER_SIZE] = { 0 };

		int32_t socks_ready = select(last_socket, &read_set, 0, 0, &select_timeout);
		if (socks_ready > 0)
		{
			for (size_t index = 0; index < HTTP_MAX_CONCURRENT_DOWNLOADS; ++index)
			{
				http_download_state* state = &_states[index];
				const bool is_active = (_states[index].flags & HTTP_FLAG_ACTIVE);
				if (is_active && FD_ISSET(_states[index].socket, &read_set))
				{
					// This socket can be read from...
					memset(buffer, 0, HTTP_BUFFER_SIZE);

					if (state->flags & HTTP_FLAG_READ_CONTENT)
					{
						assert(state->content_length > 0);

						// reading file content...
						int32_t bytes_read = net_socket_recv(state->socket, buffer, HTTP_BUFFER_SIZE);

						fs_write(state->handle, buffer, 1, bytes_read);
						state->content_bytes_read += bytes_read;

						if (bytes_read == 0)
						{
							LOGV("file completed download of %i/%i bytes.\n", state->content_bytes_read, state->content_length);
							state->flags &= ~HTTP_FLAG_ACTIVE;
							net_socket_close(state->socket);
							state->socket = 0;
							state->content_length = 0;
							state->content_bytes_read = 0;
							fs_close(state->handle);
						}
					}
					else if (state->flags & HTTP_FLAG_READ_HEADERS)
					{
						// we are reading http headers from the server.
						int32_t bytes_read = net_socket_recv(state->socket, buffer, HTTP_BUFFER_SIZE);
						if (bytes_read < 0)
						{
							LOGV("error reading http headers\n");
							state->flags &= ~HTTP_FLAG_ACTIVE;
							net_socket_close(state->socket);
							state->socket = 0;
							state->content_length = 0;
							state->content_bytes_read = 0;
							fs_close(state->handle);
							continue;
						}

						state->bytes_read += bytes_read;

						size_t read_pointer = 0;
						size_t bytes_read_into_header = 0;

						for (size_t index = 0; index < bytes_read; ++index)
						{
							state->header_data[state->header_length++] = buffer[read_pointer++];
							bytes_read_into_header++;

							// If you hit this, we're reading really large headers.
							assert(state->header_length < HTTP_HEADER_MAX_SIZE);

							// Keep reading data until we reach the content start
							// marked by '\r\n\r\n'
							const char* content_start = core::str::strstr(state->header_data, "\r\n\r\n");
							if (content_start)
							{
								content_start += 4;

								// Left over data from the buffer that we need to read.
								const size_t content_to_copy = (bytes_read - bytes_read_into_header);
								fs_write(state->handle, &buffer[bytes_read_into_header], 1, content_to_copy);
								state->content_length += content_to_copy;
								state->content_bytes_read += content_to_copy;

								// Now start reading content...
								state->flags &= ~HTTP_FLAG_READ_HEADERS;
								state->flags |= HTTP_FLAG_READ_CONTENT;

								// process headers
								http_request request;
								http_process_headers(state, state->header_data, state->header_length, &request);
								break;
							}
						}
					}
				}
			}
		}
	} // http_update

	static http_download_state* find_unused_state()
	{
		for (size_t index = 0; index < HTTP_MAX_CONCURRENT_DOWNLOADS; ++index)
		{
			const bool is_active = (_states[index].flags & HTTP_FLAG_ACTIVE);
			if (!is_active)
			{
				memset(&_states[index], 0, sizeof(http_download_state));
				return &_states[index];
			}
		}

		return nullptr;
	} // find_unused_state

	http_download_state* http_request_file(const char* url, const char* temp_path, const char* user_agent)
	{
		char service[32] 		= { 0 };
		char filename[256] 		= { 0 };
		char hostname[1024] 	= { 0 };
		char host[1024] 		= { 0 };
		char get_request[1024] 	= { 0 };

		// ipv4
		char ip_address[16] 	= { 0 };
		uint16_t port 			= 0;

		// try to get a state for a new request...
		http_download_state* state = find_unused_state();
		if (!state)
		{
			return nullptr;
		}

		state->bytes_read = 0;
		state->bytes_sent = 0;
		state->header_length = 0;
		state->content_bytes_read = 0;

		// decompose the entered URL into hostname, port, service type, and filename.
		runtime_decompose_url(url, filename, hostname, service, &port);

		// convert hostname to an ipv4 address.
		net_ipv4_by_hostname(hostname, "http", ip_address);

		// create a socket
		state->socket = net_socket_open(net_socket_type::TCP);
		if (!state->socket)
		{
			LOGW("Unable to create a socket.\n");
			return nullptr;
		}

		net_address target_address;
		net_address_set(&target_address, ip_address, port);

		if (net_socket_connect(state->socket, &target_address) < 0)
		{
			LOGW("Failed to connect to server '%s:%i'\n",
				ip_address, port);
			return nullptr;
		}

		state->flags |= HTTP_FLAG_ACTIVE;

		// send a GET request to the server...
		core::str::sprintf(get_request,
			1024,
			"GET %s HTTP/1.1\r\nUser-Agent: %s\r\nAccept: */*\r\nHost: %s\r\nConnection: close\r\n\r\n",
			filename,
			user_agent,
			hostname
		);

		int32_t bytes_sent = net_socket_send(state->socket, get_request, core::str::len(get_request));
		if (bytes_sent < 0)
		{
			LOGW("Error sending data to server.\n");
			state->flags &= ~HTTP_FLAG_ACTIVE;
			net_socket_close(state->socket);
			state->socket = 0;
			return nullptr;
		}

		state->bytes_sent = bytes_sent;

		// now reading headers...
		memset(state->header_data, 0, HTTP_HEADER_MAX_SIZE);
		state->flags |= HTTP_FLAG_READ_HEADERS;

		// make directories
		PathString file_path = temp_path;
		path::normalize(&file_path[0]);
		path::make_directories(file_path());

		// make temp file
		state->handle = fs_open(file_path(), FileMode::FileMode_Write);

		return state;
	} // http_request_file


	int32_t http_process_headers(http_download_state* state, const char* buffer, size_t header_length, http_request* request)
	{
		const char* current = buffer;
		const char* line_start = buffer;
		memset(request, 0, sizeof(http_request));

		while(current < (buffer + header_length))
		{
			if (current[0] == '\r' && current[1] == '\n')
			{
				if (request->status > 0)
				{
					// Process Individual headers here
					const size_t HTTP_MAX_HEADER_STRING_SIZE = 256;

					size_t header_length = (current - line_start);
					if (header_length > HTTP_MAX_HEADER_STRING_SIZE)
					{
						LOGV("Ignoring header (size too large)\n");
					}
					else
					{
						if (core::str::case_insensitive_compare(line_start, "content-length", 14) == 0)
						{
							state->content_length = atoi(line_start + 15);
							assert(state->content_length > 0);
						}
					}
				}
				else
				{
					// parse the http status line
					int items_read = sscanf(line_start,
						"HTTP/%d.%d %d %s",
						&request->protocol_major,
						&request->protocol_minor,
						&request->status,
						&request->message);
					assert(items_read == 4);
					if (request->status != HTTP_STATUS_OK)
					{
						// The data we read was not file contents.
						LOGV("http status: %i\n", request->status);
						if (request->status == HTTP_STATUS_NOT_FOUND)
						{
							LOGV("File not found\n");
						}
						else if (request->status == HTTP_STATUS_INTERNAL_SERVER_ERROR)
						{
							LOGV("Internal server error\n");
						}
						else
						{
							LOGV("Unknown status: %i\n", request->status);
						}

						state->flags &= ~HTTP_FLAG_ACTIVE;
						net_socket_close(state->socket);
						state->socket = 0;
						state->content_length = 0;
						fs_close(state->handle);
					}
				}

				line_start = current + 2;
				current += 2;
				continue;
			}

			current++;
		}

		return 0;
	} // http_process_headers

	uint32_t http_active_download_count()
	{
		uint32_t active_downloads = 0;

		for (size_t index = 0; index < HTTP_MAX_CONCURRENT_DOWNLOADS; ++index)
		{
			const bool is_active = (_states[index].flags & HTTP_FLAG_ACTIVE);
			if (is_active)
			{
				++active_downloads;
			}
		}

		return active_downloads;
	} // http_active_download_count


	int32_t rtsp_describe(http_connection* connection, http_request* request, const char* ip_address, const char* user_agent)
	{
		//char service[32] 		= { 0 };
		//char filename[256] 		= { 0 };
		//char hostname[1024] 	= { 0 };
		//char host[1024] 		= { 0 };
		char get_request[1024] 	= { 0 };

		// ipv4
		//char ip_address[16] 	= { 0 };
		uint16_t port 			= 554;

		//// try to get a state for a new request...
		//http_download_state* state = find_unused_state();
		//if (!state)
		//{
		//	return nullptr;
		//}

		//state->bytes_read = 0;
		//state->bytes_sent = 0;
		//state->header_length = 0;
		//state->content_bytes_read = 0;

		// decompose the entered URL into hostname, port, service type, and filename.
		//runtime_decompose_url(url, filename, hostname, service, &port);

		// convert hostname to an ipv4 address.
		//net_ipv4_by_hostname(hostname, "http", ip_address);

		// create a socket
		connection->socket = net_socket_open(net_socket_type::TCP);
		if (!connection->socket)
		{
			LOGW("Unable to create a socket.\n");
			return -1;
		}

		net_address target_address;
		net_address_set(&target_address, ip_address, port);

		if (net_socket_connect(connection->socket, &target_address) < 0)
		{
			LOGW("Failed to connect to server '%s:%i'\n",
				ip_address, port);
			return -1;
		}

		//state->flags |= HTTP_FLAG_ACTIVE;

		// "DESCRIBE rtsp://192.168.0.157/media.mp4 RTSP/1.0"

		// send a GET request to the server...
		//core::str::sprintf(get_request,
		//	1024,
		//	"GET %s HTTP/1.1\r\nUser-Agent: %s\r\nAccept: */*\r\nHost: %s\r\nConnection: close\r\n\r\n",
		//	filename,
		//	user_agent,
		//	hostname
		//);

		//core::str::sprintf(get_request,
		//	1024,
		//	"OPTIONS * RTSP/1.0\n"
		//	"CSeq: 1\n",
		//	ip_address
		//);

		core::str::sprintf(get_request,
			1024,
			"DESCRIBE rtsp://%s/11 RTSP/1.0\n"
			"CSeq: 1\n",
			"Authorization: Digest username=\"admin\",\n"
			"	realm=\"Hipcam RealServer/V1.0\",\n"
			"	nonce=\"2707c2645ddc4319fd538249f3dbcd1e\",\n"
			"	uri=\"/11\",\n"
			"	response=\"525252\"\n\n",
			ip_address
		);

/*
   In order to access media presentations from RTSP servers that require
   authentication, the client MUST additionally be able to do the
   following:
	 * recognize the 401 status code;
	 * parse and include the WWW-Authenticate header;
	 * implement Basic Authentication and Digest Authentication.
*/
	// https://tools.ietf.org/html/rfc2617

		int32_t bytes_sent = net_socket_send(connection->socket, get_request, core::str::len(get_request));
		if (bytes_sent < 0)
		{
			LOGW("Error sending data to server.\n");
			//state->flags &= ~HTTP_FLAG_ACTIVE;
			net_socket_close(connection->socket);
			connection->socket = 0;
			return -1;
		}

		connection->bytes_sent = bytes_sent;

		// now reading headers...
		//memset(state->header_data, 0, HTTP_HEADER_MAX_SIZE);
		//state->flags |= HTTP_FLAG_READ_HEADERS;

		// make directories
		//PathString file_path = temp_path;
		//path::normalize(&file_path[0]);
		//path::make_directories(file_path());

		// make temp file
		//state->handle = fs_open(file_path(), FileMode::FileMode_Write);

		gemini::Allocator allocator = gemini::memory_allocator_default(MEMORY_ZONE_DEFAULT);
		{
			fd_set read_set;
			fd_set except_set;

			FD_ZERO(&read_set);
			FD_ZERO(&except_set);

			timeval select_timeout;
			select_timeout.tv_sec = 1;

			int last_socket = -1;


			last_socket = connection->socket + 1;
			FD_SET(connection->socket, &read_set);


			const size_t HTTP_BUFFER_SIZE = 32768;
			char buffer[HTTP_BUFFER_SIZE] = { 0 };

			int32_t socks_ready = select(last_socket, &read_set, 0, 0, &select_timeout);
			if (socks_ready > 0)
			{
				if (FD_ISSET(connection->socket, &read_set))
				{
					// This socket can be read from...
					memset(buffer, 0, HTTP_BUFFER_SIZE);

					// reading file content...
					int32_t bytes_read = net_socket_recv(connection->socket, buffer, HTTP_BUFFER_SIZE);

					//fs_write(state->handle, buffer, 1, bytes_read);
					connection->bytes_read += bytes_read;
					LOGV("read %i bytes.\n", bytes_read);
					LOGV("data:\n%s\n", buffer);
					//gemini::string response = gemini::string_create(buffer);
					//Array<gemini::string> headers(allocator);
					//rtsp_parse_response(headers, response);
				}
			}
		}


		net_socket_close(connection->socket);


		return -1;
	} // rtsp_describe

	void rtsp_parse_response(gemini::Allocator& allocator, http_request* request, HeaderHashSet& headers, Array<gemini::string>& lines)
	{
		// Parse the response from an RTSP request.

		// 'RTSP/1.0 401 Unauthorized'

		// the first line of the response is always a response code
		int items_read = sscanf(lines[0].c_str(),
			"RTSP/%d.%d %d %s",
			&request->protocol_major,
			&request->protocol_minor,
			&request->status,
			&request->message);

		assert(items_read == 4);

		for (size_t index = 1; index < lines.size(); ++index)
		{
			gemini::string& line = lines[index];
			LOGV("process line: '%s' ----\n", line.c_str());
			Array<gemini::string> pieces(allocator);
			string_split(allocator, pieces, line, ":");

			assert(pieces.size() == 2);

			gemini::string& key = pieces[0];
			LOGV("key is '%s'\n", key.c_str());
			gemini::string& value = pieces[1];



			Array<gemini::string> csv(allocator);
			string_split(allocator, csv, value, ",");
			if (csv.size() > 0)
			{
				uint32_t advance = 0;
				if (key == "WWW - Authenticate")
				{
					// read and extract the auth type from the parameter list.
					char auth_type[16] = { 0 };
					int result = sscanf(csv[0].c_str(), "%s", &auth_type);
					LOGV("res: %i\n", result);
					advance = core::str::len(auth_type) + 1; // also skip past the space

					gemini::string prev = csv[0];
					csv[0] = string_substr(allocator, &csv[0][0], advance, csv[0].length() - advance);

					string_destroy(allocator, prev);

					// Only 'Digest' is supported.
					assert(core::str::case_insensitive_compare("Digest", auth_type, 0) == 0);

					// extract the once
					for (size_t csi = 0; csi < csv.size(); ++csi)
					{
						//LOGV("csi -> '%s'\n", csv[csi].c_str());
						Array<gemini::string> kv(allocator);
						string_split(allocator, kv, csv[csi], "=");

						if (kv[0] == "nonce")
						{
							// strip double-quotes and store as the nonce value.
							request->nonce_value = string_slice(allocator, &kv[1][0], 1, kv[1].length() - 1);
						}

						for (size_t ki = 0; ki < kv.size(); ++ki)
						{
							//LOGV("-> '%s'\n", kv[ki].c_str());
							string_destroy(allocator, kv[ki]);
						}
					}
				}
				else if (key == "CSeq")
				{
					request->client_sequence_id = string_create(allocator, value);
				}

				for (size_t csi = 0; csi < csv.size(); ++csi)
				{
					//LOGV("csi -> '%s'\n", csv[csi].c_str());
					string_destroy(allocator, csv[csi]);
				}
			}

			//for (size_t line_index = 0; line_index < pieces.size(); ++line_index)
			//{
			//	//LOGV("p: '%s'\n", pieces[line_index].c_str());

			//	Array<gemini::string> csv(allocator);
			//	string_split(allocator, csv, value, ",");

			//	for (size_t csi = 0; csi < csv.size(); ++csi)
			//	{
			//		LOGV("csi -> '%s'\n", csv[csi].c_str());
			//		string_destroy(allocator, csv[csi]);
			//	}

			//	string_destroy(allocator, pieces[line_index]);
			//}

			string_destroy(allocator, key);
			string_destroy(allocator, value);
		}
	} // rtsp_parse_response

} // namespace gemini
