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

#include "mem.h"
#include "platform_internal.h"

#include <unistd.h> // for close()
#include <termios.h>
#include <fcntl.h>
#include <assert.h>

namespace platform
{
	struct PosixSerial : Serial
	{
		int32_t socket;
	};

	Serial* serial_open(const char* device, uint32_t baud_rate)
	{
		int32_t socket = ::open(device, O_RDWR | O_NOCTTY | O_NDELAY);
		if (socket == -1)
		{
			return 0;
		}
		
		// clear file status
		fcntl(socket, F_SETFL, 0);
		
		{
			// setup the baud rate
			struct termios options;
			tcgetattr(socket, &options);
			
			cfsetispeed(&options, baud_rate);
			cfsetospeed(&options, baud_rate);
			
			// enable receiver and set local mode
			options.c_cflag |= (CLOCAL | CREAD);
			
			// set new options
			tcsetattr(socket, TCSANOW, &options);
		}
		
		PosixSerial* serial = CREATE(PosixSerial);
		serial->socket = socket;
		return serial;
	}
	
	void serial_close(Serial* serial)
	{
		PosixSerial* device = static_cast<PosixSerial*>(serial);
		if (device->socket != -1)
		{
			::close(device->socket);
		}
		DESTROY(PosixSerial, device);
	}
	
	
	int serial_read(Serial* serial, void* buffer, int total_bytes)
	{
		PosixSerial* device = static_cast<PosixSerial*>(serial);
		assert(device != 0);
		return ::read(device->socket, buffer, total_bytes);
	}
	
	int serial_write(Serial* serial, const void* buffer, int total_bytes)
	{
		PosixSerial* device = static_cast<PosixSerial*>(serial);
		assert(device != 0);
		return ::write(device->socket, buffer, total_bytes);
	}
	
}; // namespace platform