// -------------------------------------------------------------
// Copyright (C) 2011- Adam Petrone

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM,OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
// -------------------------------------------------------------
#if 0
USAGE:
	xsocket_t s;

	xnet_startup();
	xnet_sock_open( &s, xsocket_type_udp );
	xnet_sock_bind( s, 0 );

	xnet_sock_close( &s );

	xnet_shutdown();
#endif

#pragma once

#ifdef WIN32
	#pragma warning(disable: 4996)
	#pragma warning(disable: 4018)
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN 1
	#endif
	#include <winsock2.h>
	#include <windows.h>
	#include <Ws2tcpip.h>
	#pragma comment( lib, "ws2_32.lib" )
#else
	#include <errno.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <fcntl.h>
	#include <sys/time.h>
	#include <arpa/inet.h>
	#include <sys/types.h>
	#include <netdb.h>
	#include <unistd.h> // warning: implicit declaration of function 'close'
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifdef WIN32
	typedef SOCKET xsocket_t;
#else
	typedef int xsocket_t;
#endif

typedef struct sockaddr_in xaddress_t;

typedef enum
{
	xsocket_type_udp,
	xsocket_type_tcp
} xsocket_type;

void xaddress_init( xaddress_t * address );
void xaddress_set( xaddress_t * address, const char * host, unsigned short port );
void xaddress_host( xaddress_t * address, char * buffer, unsigned int bufferLength );
unsigned short xaddress_port( xaddress_t * address );
int xaddress_size( void );

//
// socket functions
// returns -1 on failure
int xnet_sock_open( xsocket_t * sock, xsocket_type type );

void xnet_sock_close( xsocket_t * sock );
void xnet_sock_bind( xsocket_t sock, unsigned short port );

// returns bytes sent
int xnet_sock_send( xsocket_t sock, const char * packet, int packet_size );
int xnet_sock_sendto( xsocket_t sock, xaddress_t * to, const char * packet, int packet_size );

// returns bytes received
int xnet_sock_recv( xsocket_t sock, char * buffer, int packet_size );
int xnet_sock_recvfrom( xsocket_t sock, xaddress_t * from, char * buffer, int packet_size );

// returns 0 on success, -1 on error
int xnet_sock_connect( xsocket_t sock, xaddress_t * to );



// returns > 0 on success
int xnet_enableReuse( xsocket_t sock );
int xnet_enableNonBlocking( xsocket_t sock );

// split a hostname up into ip and port (if specified), otherwise returns hostname
void xnet_split_host( const char * ip, char * host, unsigned short * port );

// decompose a full url, "http://www.gesturebox.net/update/update.xml" into a hostname and file string.
// ex: "www.gesturebox.net", and "update.xml"
int xnet_decompose_url( const char * fullURL, char * file, char * hostname, char * service, short * port );

// retrieve the standard port for a service name; http -> 80, https -> 443, ftp -> 21, ssh -> 22, smtp -> 25
short xnet_standard_port_for_service( const char * service );

int xnet_startup( void );
void xnet_shutdown( void );

// convert errno to string
const char * xnet_errno_string( void );

int xnet_ipfromhost( const char * host, const char * service, int protocol, char * ip );
void xnet_gethostname( char * host, int host_size );
	
#ifdef __cplusplus
}; // extern "C"
#endif
