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
//
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

#include <core/platform/platform_internal.h>
#include <core/typedefs.h>
#include <core/mem.h>

#if defined(PLATFORM_WINDOWS)
#	define WIN32_LEAN_AND_MEAN 1
#	define NOMINMAX
#	include <windows.h>
#	include <winsock2.h>
#elif defined(PLATFORM_LINUX) || defined(PLATFORM_APPLE) || defined(PLATFORM_ANDROID)
#	include <netinet/in.h> // for sockaddr_in
#	include <sys/select.h>
#else
#	error Unknown platform!
#endif

namespace platform
{
	// ---------------------------------------------------------------------
	// network
	// ---------------------------------------------------------------------
#if defined(PLATFORM_WINDOWS)
	typedef SOCKET net_socket;
	const net_socket NET_SOCKET_INVALID = 0;
#elif defined(PLATFORM_POSIX)
	typedef int net_socket;
	const net_socket NET_SOCKET_INVALID = 0;
#else
#error net_socket not defined for this platform!
#endif

	enum class net_socket_type
	{
		UDP,
		TCP
	};

	enum class net_socket_how
	{
		READ,
		WRITE,
		READ_WRITE
	};


	typedef struct sockaddr_in net_address;

	void net_address_init(net_address* address);
	void net_address_set(net_address* address, const char* ip, uint16_t port);
	int32_t net_address_host(net_address* address, char* buffer, size_t buffer_size);
	uint16_t net_address_port(net_address* address);
	void net_address_port(net_address* address, uint16_t port);

	void net_shutdown();

	// socket functions

	// returns the last error code if a function fails
	int32_t net_last_error();

	/// @returns 0 on success; -1 on failure
	net_socket net_socket_open(net_socket_type type);

	// @returns true if socket is valid, false otherwise.
	bool net_socket_is_valid(net_socket sock);

	void net_socket_close(net_socket sock);
	void net_socket_shutdown(net_socket sock, net_socket_how how);

	// multi-cast group membership
	int32_t net_socket_add_multicast_group(net_socket sock, const char* group_address);
	int32_t net_socket_remove_multicast_group(net_socket sock, const char* group_address);

	/// @brief Accept a connection (TCP-only)
	/// @returns non-zero on success: file descriptor for the accepted socket.
	net_socket net_socket_accept(net_socket sock, net_address* source);

	/// @returns 0 on success
	int32_t net_socket_bind(net_socket sock, net_address* interface);

	// Connect to a remote server (TCP only)
	int32_t net_socket_connect(net_socket sock, net_address* to);

	/// @brief causes the socket to enter listening state (TCP-only)
	/// @returns 0 on success
	int32_t net_socket_listen(net_socket sock, int32_t queued_connections);

	/// @brief Send data (TCP-only)
	/// @returns bytes written.
	int32_t net_socket_send(net_socket sock, const char* data, size_t data_size);

	/// @brief Send data (UDP-only)
	/// @returns bytes read if > 0; otherwise an error code.
	int32_t net_socket_sendto(net_socket sock, net_address* destination, const char* data, size_t data_size);

	/// @brief Receive data (TCP-only)
	/// @returns bytes read if > 0; otherwise an error code.
	int32_t net_socket_recv(net_socket sock, char* buffer, size_t buffer_size);

	/// @brief Receive data (UDP-only)
	/// @returns bytes read if > 0; otherwise an error code.
	int32_t net_socket_recvfrom(net_socket sock, net_address* from, char* buffer, size_t buffer_size);

	// socket options

	// enable/disable address re-use
	int32_t net_socket_set_reuseaddr(net_socket sock, int32_t value);

	// enable/disable blocking i/o
	int32_t net_socket_set_blocking(net_socket sock, int32_t value);

	// enable/disable broadcast permission on this socket
	int32_t net_socket_set_broadcast(net_socket sock, int32_t enable_broadcast);

	// returns 0 on success
	int32_t net_startup();

	int32_t net_ipv4_by_hostname(const char* hostname, const char* service, char ip_address[16]);
} // namespace platform
