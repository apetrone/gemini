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

#include <core/logging.h>

#include <assert.h>

// TODO@APP: Implement me: https://msdn.microsoft.com/en-us/library/ms810467.aspx
namespace platform
{
	class Win32Serial : public Serial
	{
	public:
		HANDLE handle;
	};

	// If serial port reads are delayed on Windows; you may need to lower
	// the Serial Port Transfoer/Receive sizes to their minimum.
	// In addition, lower the Latency Timer to the lowest.
	// Device Manager -> Ports -> Advanced
	Serial* serial_open(const char* port, uint32_t baud_rate)
	{
		HANDLE handle = CreateFileA(port,
			GENERIC_READ | GENERIC_WRITE,
			0, // must be zero
			NULL,
			OPEN_EXISTING, // must be OPEN_EXISTING
			FILE_ATTRIBUTE_NORMAL,
			NULL // must be NULL
		);

		if (handle == INVALID_HANDLE_VALUE)
		{
			DWORD last_error = GetLastError();
			if (last_error == ERROR_FILE_NOT_FOUND)
			{
				LOGW("Serial port '%s' does not exist.\n", port);
			}
			else
			{
				LOGW("Unable to open '%s'; GetLastError() = %i\n", port, last_error);
			}

			return nullptr;
		}

		DCB dcb;
		FillMemory(&dcb, sizeof(dcb), 0);
		dcb.DCBlength = sizeof(DCB);
		if (!GetCommState(handle, &dcb))
		{
			LOGE("Error in GetCommState!\n");
			CloseHandle(handle);
			return nullptr;
		}

		dcb.BaudRate = baud_rate;
		dcb.ByteSize = 8;
		dcb.StopBits = 0;
		dcb.Parity = 0;
		if (!SetCommState(handle, &dcb))
		{
			LOGE("Error in SetCommState.\n");
			CloseHandle(handle);
			return nullptr;
		}

		Win32Serial* serial = MEMORY_NEW(Win32Serial, platform::get_platform_allocator());
		serial->handle = handle;
		return serial;
	}

	void serial_close(Serial* serial)
	{
		Win32Serial* native_serial = static_cast<Win32Serial*>(serial);
		CloseHandle(native_serial->handle);
		MEMORY_DELETE(native_serial, platform::get_platform_allocator());
	}

	int serial_read(Serial* serial, void* buffer, int bytes_requested)
	{
		Win32Serial* native_serial = static_cast<Win32Serial*>(serial);
		DWORD bytes_read = 0;

		// blocking read; could also use OVERLAPPED I/O for async reads.
		if (!SetCommMask(native_serial->handle, EV_RXCHAR))
		{
			LOGE("Error in SetCommMask\n");
			return 0;
		}

		DWORD comm_event;
		if (WaitCommEvent(native_serial->handle, &comm_event, 0))
		{
			bool result = ReadFile(
				native_serial->handle,
				buffer,
				bytes_requested,
				&bytes_read, 0);

			if (!result)
			{
				DWORD last_error = GetLastError();
				if (last_error == ERROR_NOACCESS)
				{
					LOGE("ReadFile returned ERROR_NOACCESS (998)\n");
				}
				else
				{
					LOGE("Unhandled error: %i\n", last_error);
				}
			}
		}
		return static_cast<int>(bytes_read);
	}

	int serial_write(Serial* serial, const void* buffer, int bytes_to_write)
	{
		Win32Serial* native_serial = static_cast<Win32Serial*>(serial);
		DWORD bytes_written;
		bool result = WriteFile(native_serial->handle,
			buffer,
			bytes_to_write,
			&bytes_written,
			NULL
		);

		assert(result);
		return static_cast<int>(bytes_written);
	}

}; // namespace platform
