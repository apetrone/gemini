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
#include "xnet.h"
#include <stdlib.h>
#include <string.h>

#if LINUX
#include <errno.h>
#endif

#if LINUX || __APPLE__
#define strnicmp strncasecmp
#define stricmp strcasecmp
#endif

#ifdef __cplusplus
extern "C" {
#endif

void xaddress_init( xaddress_t * address )
{
	if ( !address )
		return;

	memset( address, 0, xaddress_size() );
}

void xaddress_set( xaddress_t * address, const char * host, unsigned short port )
{
	if ( !address )
		return;

	address->sin_family = AF_INET;
	address->sin_port = htons( port );

	if ( !host )
	{
#ifdef WIN32
		address->sin_addr.S_un.S_addr = INADDR_ANY;
#else
		address->sin_addr.s_addr = INADDR_ANY;
#endif
	}
	else
	{
#ifdef WIN32
		address->sin_addr.S_un.S_addr = inet_addr(host);
#else
		address->sin_addr.s_addr = inet_addr(host);
#endif
	}
}

void xaddress_host( xaddress_t * address, char * buffer, unsigned int bufferLength )
{
	char * pt = 0;
	size_t maxPt = 0;

	if ( !address )
		return;

	pt = inet_ntoa( address->sin_addr );

	maxPt = strlen(pt);

	if ( maxPt > bufferLength )
		maxPt = bufferLength;

	strncpy( buffer, pt, maxPt );
} // xaddress_host

unsigned short xaddress_port( xaddress_t * address )
{
	if ( !address )
		return 0;

	return ntohs( address->sin_port );
} // xaddress_port

int xaddress_size( void )
{
	return sizeof(struct sockaddr_in);
} // xaddress_size


// socket functions
int xnet_sock_open( xsocket_t * sock, xsocket_type type )
{
	*sock = socket( AF_INET, (type == xsocket_type_udp) ? SOCK_DGRAM : SOCK_STREAM, (type == xsocket_type_udp) ? IPPROTO_UDP : IPPROTO_TCP );

	if ( *sock <= 0 )
	{
		printf( "Failed creating socket!\n" );
		return 0;
	}

	return *sock > 0;
}

void xnet_sock_close( xsocket_t * sock )
{
#ifdef WIN32
	closesocket( *sock );
#else
	close( *sock );
#endif

	*sock = 0;
} // xnet_sock_close

void xnet_sock_bind( xsocket_t sock, unsigned short port )
{
	struct sockaddr_in a;
	int err = 0;
	a.sin_family = AF_INET;
	if ( port > 0 )
	{
		a.sin_port = htons( port );
	}

#ifdef WIN32
	a.sin_addr.S_un.S_addr = INADDR_ANY;
#else
	a.sin_addr.s_addr = INADDR_ANY;
#endif

	err = bind( sock, (const struct sockaddr*)&a, xaddress_size() );
	if ( err < 0 )
	{
		printf( "Error binding socket! %i\n", err );
	}
} // xnet_sock_bind

// returns bytes sent
int xnet_sock_send( xsocket_t sock, const char * packet, int packetSize )
{
	int bytesSent = send( sock, packet, packetSize, 0 );

	if ( bytesSent == -1 )
	{
#ifdef WIN32
		printf( "[Socket] Error sending data. %i\n", GetLastError() );
#else
		printf( "[Socket] Error sending data.\n" );
#endif
	}
	else
	{

		if ( bytesSent < packetSize )
		{
			printf( "[Socket] Failed to send packet!\n" );
		}
	}

	return bytesSent;
} // xnet_sock_send

	
// returns bytes sent
int xnet_sock_sendto( xsocket_t sock, xaddress_t * to, const char * packet, int packetSize )
{
	int bytesSent = sendto( sock, packet, packetSize, 0, (const struct sockaddr*)to, xaddress_size() );

	if ( bytesSent == -1 )
	{
#ifdef WIN32
		printf( "[Socket] Error sending data. %i\n", GetLastError() );
#else
		//printf( "[Socket] Error sending data. %i\n", errno );
#endif
	}
	else
	{
		if ( bytesSent < packetSize )
		{
			printf( "[Socket] Failed to send packet!\n" );
		}
	}

	return bytesSent;
} // xnet_sock_sendto

// returns bytes received
int xnet_sock_recv( xsocket_t sock, char * buffer, int packetSize )
{
	return recv( sock, buffer, packetSize, 0 );
} // xnet_sock_recv

// returns bytes received
int xnet_sock_recvfrom( xsocket_t sock, xaddress_t * from, char * buffer, int packet_size )
{
	socklen_t fromLen = xaddress_size();
	return recvfrom( sock, buffer, packet_size, 0, (struct sockaddr*)from, &fromLen );
} // xnet_sock_recvfrom

int xnet_sock_connect( xsocket_t sock, xaddress_t * to )
{
	return connect( sock, (struct sockaddr*)to, xaddress_size() );
} // xnet_sock_connect


// generic network functions

int xnet_enableReuse( xsocket_t sock )
{
	int r = 0;

#ifdef WIN32
	r = (setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&r, sizeof(r) ) != 0);
#else
	sock = sock;
#endif

	return r;
}

int xnet_enableNonBlocking( xsocket_t sock )
{
	int r = 1;

#ifdef WIN32
	DWORD nonBlocking = 1;
	if ( ioctlsocket( sock, FIONBIO, &nonBlocking ) != 0 )
	{
		return 0;
	}
#else
	int nonBlocking = 1;
	if ( fcntl( sock, F_SETFL, O_NONBLOCK, nonBlocking ) == -1 )
	{
		return 0;
	}
#endif

	return r;
}

void xnet_split_host( const char * ip, char * host, unsigned short * port )
{
	const char * pos = 0;
	pos = strchr( ip, ':' );
	if ( pos != 0 )
	{
		strncpy( host, ip, (pos-ip) );
		//*port = atoi( pos+1 );
		*port = strtoul( pos+1, 0, 0 );
	}
	else
	{
		strcpy( host, ip );
	}
} // xnet_split_host


int xnet_decompose_url( const char * fullURL, char * file, char * hostname, char * service, short * port )
{
    //printf( "fullURL: %s\n", fullURL );
    int urlLength = strlen(fullURL);

    int i;
	int j;
    const char * cur = fullURL;
	const char * svc;
    int slashPos = -1;
    int hostStart = -1;

	svc = strstr( fullURL, "://" );
	if ( svc )
	{
		// found a service.
		strncpy( service, fullURL, svc-fullURL );
	}


    // if the last character is a '/' then our job is easy
    if ( fullURL[ urlLength-1 ] == '/' )
    {
        //printf( "URL ends with a /\n" );
        slashPos = urlLength-1;
    }


    for( i = 0; i < urlLength; ++i )
    {
        if ( hostStart == -1 && cur[i] == '/' && cur[i+1] == '/' )
        {
            //printf( "Found Host Start: %i\n", i+2 );
            hostStart = i+2;
        }
        else if ( hostStart == -1 && cur[i] == '.' )
        {
            hostStart = 0;
        }

        if ( slashPos == -1 && cur[i] == '/' && cur[i-1] != '/' && cur[i+1] != '/' )
        {
            //printf( "Found Slash Pos: %i\n", i );
            slashPos = i;
            break;
        }
    }

    // special case when there is no ending slash
    if ( slashPos == -1 )
    {
        file[0] = '/';
        slashPos = urlLength;
    }
    else
    {

        for( j = 0, i = slashPos; i < urlLength; ++i, ++j)
        {
            file[ j ] = fullURL[ i ];

        }
    }

    for( j = 0, i = hostStart; i< slashPos; ++i, ++j )
    {
        hostname[ j ] = fullURL[ i ];
    }


	// see if a special port was specified
	cur = strchr( hostname, ':' );
	if ( cur != 0 )
	{
		// truncate host name
		hostname[ cur-hostname ] = 0;
		*port = atoi( cur+1 );
	}
	else
	{
		*port = xnet_standard_port_for_service( service );
	}


    return 0;
} // xnet_decompose_url

int xnet_startup( void )
{
	#ifdef WIN32
		struct WSAData data;
		WORD ver = MAKEWORD( 2, 2 );
		int err = WSAStartup( ver, &data );

		if ( err != 0 )
		{
			printf( "WSAStartup failed: %i\n", err );
			return 0;
		}
	#endif

	return 1;
} // xnet_Startup

void xnet_shutdown( void )
{
	#ifdef WIN32
		int err = WSACleanup();
		if ( err != 0 )
		{
			printf( "WSACleanup failed: %i\n", err );
		}
	#endif
} // xnet_Shutdown

const char * xnet_errno_string( void )
{
#if LINUX
    switch( errno )
    {
        case EACCES: return "EACCES";
        case EADDRINUSE: return "EADDRINUSE";
        case EAFNOSUPPORT: return "EAFNOSUPPORT";
        case EAGAIN: return "EAGAIN";
        case EALREADY: return "EALREADY";
        case EBADF: return "EBADF";
        case ECONNREFUSED: return "ECONNREFUSED";
        case EFAULT: return "EFAULT";
        case EINPROGRESS: return "EINPROGRESS";
        case EINTR: return "EINTR";
        case EISCONN: return "EISCONN";
        case ENETUNREACH: return "ENETUNREACH";
        case ENOTSOCK: return "ENOTSOCK";
        case ETIMEDOUT: return "ETIMEDOUT";
        default:
            printf( "errno: %i\n", errno );
            return "Unknown";
    }
#endif
    return "";
} // xnet_errno_string


int xnet_ipfromhost( const char * host, const char * service, int protocol, char * ip )
{
	// http://www.logix.cz/michal/devel/various/getaddrinfo.c.xp
	// http://msdn.microsoft.com/en-us/library/ms738520(v=vs.85).aspx
	// http://www.nongnu.org/ruli/tutorial/getaddrinfo.html
	//char * protoname = "tcp";
	struct addrinfo hints;
	//struct protoent *pe;
	struct addrinfo *ai_res;
	struct addrinfo * ai;
	struct sockaddr_in *sa;
	int result;
	int count = 0;

	//pe = getprotobyname( protoname );

	memset( &hints, 0, sizeof(struct addrinfo) );
	/*
	if (stricmp( protoname, "tcp" ) == 0 )
		hints.ai_socktype = SOCK_STREAM;
	else if ( stricmp( protoname, "udp" ) == 0 )
		hints.ai_socktype = SOCK_DGRAM;

	if ( !pe )
		return 0;
	hints.ai_protocol = pe->p_proto;
	*/

	if ( protocol == 0 )
		hints.ai_socktype = SOCK_DGRAM;
	else if ( protocol == 1 )
		hints.ai_socktype = SOCK_STREAM;

	hints.ai_flags = AI_CANONNAME;
	hints.ai_family = AF_UNSPEC;
	hints.ai_addrlen = 0;
	hints.ai_addr = 0;
	hints.ai_canonname = 0;

	result = getaddrinfo( host, service, &hints, &ai_res );
	if ( result != 0 )
	{
		//printf( "getaddrinfo failed: %s\n", gai_strerrorA(result) );
		return 0;
	}

	// scan result
	for( ai = ai_res; ai; ai = ai->ai_next )
	{
		switch( ai->ai_family )
		{
		case PF_INET:
			sa = (struct sockaddr_in*)ai->ai_addr;
			//printf( " canon=%s port=%d IPv4/%s\n", ai->ai_canonname, ntohs(sa->sin_port), inet_ntoa(sa->sin_addr));
			strncpy( ip, (inet_ntoa(sa->sin_addr)), 16 );
			++count;
		case PF_INET6:
			//printf( "Support inet6\n" );
			break;
		}
	}

	freeaddrinfo( ai_res );
	return count;
}

	
void xnet_gethostname( char * host, int host_size )
{
	gethostname( host, host_size );
}
	
short xnet_standard_port_for_service( const char * service )
{
	if ( stricmp( service, "http" ) == 0 )
		return 80;
	else if ( stricmp( service, "https" ) == 0 )
		return 443;

	return 0;
} // xnet_standard_port_for_service






#ifdef __cplusplus
}; // extern "C"
#endif
