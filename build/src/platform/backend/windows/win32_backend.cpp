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

#include "platform_internal.h"

#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>

namespace platform
{
	static unsigned int _previous_error_mode;

	Result backend_startup()
	{
		// prevent error dialogs from hanging the process.
		// these errors are forwarded to calling process.
		_previous_error_mode = SetErrorMode(SEM_FAILCRITICALERRORS);

		return Result::success();
	} // backend_startup

	void backend_shutdown()
	{
		// restore the error mode
		SetErrorMode(_previous_error_mode);
	} // backend_shutdown

	void backend_log(platform::LogMessageType type, const char* message)
	{
		// honey badger don't care about LogMessageType
		OutputDebugStringA(message);
	} // backend_log





	// system
	size_t system_pagesize_bytes()
	{
		SYSTEM_INFO system_info;
		GetSystemInfo(&system_info);
		return system_info.dwPageSize;
	} // system_pagesize_bytes

	size_t system_processor_count()
	{
		// docs mention use of 'GetLogicalProcessorInformation',
		// but I don't understand the difference.
		SYSTEM_INFO system_info;
		GetSystemInfo(&system_info);
		return system_info.dwNumberOfProcessors;
	} // system_processor_count

	double system_uptime_seconds()
	{
		return GetTickCount64() * SecondsPerMillisecond;
	} // system_uptime_seconds

	core::StackString<64> system_version_string()
	{
		core::StackString<64> version = "Windows";
		return version;
	} // system_version_string

} // namespace platform
