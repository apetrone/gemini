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

#include <platform/platform_internal.h>

#include <core/str.h>

#include <core/logging.h>

// Ignore these warnings in Microsoft's headers.
#pragma warning(push)
#pragma warning(disable: 4365) // conversion from 'bool' to 'BOOLEAN', signed/unsigned mismatch
#pragma warning(disable: 4574) // 'INCL_WINSOCK_API_TYPEDEFS' is defined to be '0': did you mean to use '#if INCL_WINSOCK_API_TYPEDEFS'?

#include <ws2tcpip.h> // for socklen_t

#pragma warning(pop)

namespace platform
{
	void net_address_init(net_address* address)
	{
		memset(address, 0, sizeof(net_address));
	} // net_address_init

	void net_address_set(net_address* address, const char* ip, uint16_t port)
	{
		address->sin_family = AF_INET;
		address->sin_port = htons(port);

		if (port > 0)
		{
			net_address_port(address, port);
		}

		if (ip)
		{
			int32_t result = inet_pton(AF_INET, ip, &address->sin_addr);
			// result == 0: src does not contain a character string representing a valid network address.
			// result == -1: errno set to EAFNOSUPPORT.
			assert(result > 0);
		}
		else
		{
			address->sin_addr.S_un.S_addr = htonl(INADDR_ANY);
		}
	}

	int32_t net_address_host(net_address* address, char* buffer, size_t buffer_size)
	{
		assert(buffer_size >= INET_ADDRSTRLEN);

		const char* result = inet_ntop(AF_INET, &address->sin_addr, buffer, buffer_size);
		return result != nullptr;
	} // net_address_host

	uint16_t net_address_port(net_address* address)
	{
		return ntohs(address->sin_port);
	} // net_address_port

	void net_address_port(net_address* address, uint16_t port)
	{
		address->sin_port = htons(port);
	} // net_address_port


	void net_shutdown()
	{
	} // net_shutdown

	// socket functions

	/// @returns 0 on success; -1 on failure
	net_socket net_socket_open(net_socket_type type)
	{
		int sock_type = (type == net_socket_type::UDP) ? SOCK_DGRAM : SOCK_STREAM;
		int protocol = (type == net_socket_type::UDP) ? IPPROTO_UDP : IPPROTO_TCP;
		return socket(AF_INET, sock_type, protocol);
	} // net_socket_open

	bool net_socket_is_valid(net_socket sock)
	{
		return (sock > 0);
	} // net_socket_is_valid

	void net_socket_close(net_socket sock)
	{
		closesocket(sock);
	} // net_socket_close

	int32_t net_socket_bind(net_socket sock, net_address* address)
	{
		return bind(sock, (const struct sockaddr*)address, sizeof(net_address));
	} // net_sock_bind

	/// @brief Send data (TCP-only)
	/// @returns bytes written.
	int32_t net_socket_send(net_socket sock, const char* data, size_t data_size)
	{
		return send(sock, data, static_cast<int>(data_size), 0);
	} // net_socket_send

	int32_t net_socket_sendto(net_socket sock, net_address* destination, const char* data, size_t data_size)
	{
		return sendto(sock, data, static_cast<int>(data_size), 0, (const struct sockaddr*)destination, sizeof(net_address));
	} // net_socket_sendto

	int32_t net_socket_recv(net_socket sock, char* buffer, size_t buffer_size)
	{
		return recv(sock, buffer, static_cast<int>(buffer_size), 0);
	} // net_socket_recv

	int32_t net_socket_recvfrom(net_socket sock, net_address* from, char* buffer, size_t buffer_size)
	{
		socklen_t from_length = sizeof(net_address);
		return recvfrom(sock, buffer, static_cast<int>(buffer_size), 0, (struct sockaddr*)from, &from_length);
	} // net_socket_recvfrom

	// socket options

	int32_t net_socket_set_reuseaddr(net_socket sock, int32_t value)
	{
		int result = value;
		result = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&result, sizeof(DWORD));
		return result;
	} // net_socket_set_reuseaddr

	int32_t net_socket_set_blocking(net_socket sock, int32_t value)
	{
		u_long non_blocking = static_cast<u_long>(value) ? 1UL : 0UL;
		return ioctlsocket(sock, FIONBIO, &non_blocking);
	} // net_socket_set_blocking

	int32_t net_startup()
	{
		struct WSAData wsdata;
		WORD version = MAKEWORD(2, 2);
		return WSAStartup(version, &wsdata);
	} // net_startup

	int32_t net_ipv4_by_hostname(const char* hostname, const char* service, char ip_address[16])
	{
		// https://msdn.microsoft.com/en-us/library/ms738520(v=vs.85).aspx

		struct addrinfo hints;
		struct addrinfo* addr_result = nullptr;
		struct addrinfo* address_info = nullptr;
		struct sockaddr_in* sock_address = nullptr;

		memset(&hints, 0, sizeof(struct addrinfo));

		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_CANONNAME;

		int result = getaddrinfo(hostname, service, &hints, &addr_result);
		if (result != 0)
		{
			LOGV("getaddrinfo failed: %s\n", gai_strerrorA(result));
			return -1;
		}

		// scan results
		for (address_info = addr_result; address_info; address_info = address_info->ai_next)
		{
			if (address_info->ai_family == PF_INET)
			{
				sock_address = (struct sockaddr_in*)address_info->ai_addr;
				core::str::copy(ip_address, (inet_ntoa(sock_address->sin_addr)), 16);
				break;
			}
		}

		freeaddrinfo(addr_result);

		return 0;
	} // net_ipv4_by_hostname

	int32_t net_socket_connect(net_socket sock, net_address& to)
	{
		return connect(sock, (sockaddr*)&to, sizeof(net_address));
	} // net_socket_connect
} // namespace platform
