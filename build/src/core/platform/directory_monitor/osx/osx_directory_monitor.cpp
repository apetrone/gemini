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
#include "platform_internal.h"
#include <platform/directory_monitor.h>

#include <core/logging.h>
#include <core/mem.h>
#include <core/hashset.h>

// https://developer.apple.com/library/content/documentation/Darwin/Conceptual/FSEvents_ProgGuide/UsingtheFSEventsFramework/UsingtheFSEventsFramework.html

namespace gemini
{
	struct DirectoryMonitorRecord
	{
		MonitorDelegate callback;
		platform::PathString path;
		int32_t notify_handle;
	};

	struct DirectoryMonitorState
	{
		Allocator& allocator;
		int32_t inotify_instance;
		Array<DirectoryMonitorRecord*> records;

		DirectoryMonitorState(Allocator& _allocator)
			: allocator(_allocator)
			, records(_allocator)
		{
		}
	}; // DirectoryMonitorState

	DirectoryMonitorState* _monitor_state = nullptr;

	int32_t directory_monitor_startup(Allocator& allocator)
	{
		// Not yet implemented.
		assert(0);
		return 0;
	} // directory_monitor_startup

	void directory_monitor_shutdown()
	{
	} // directory_monitor_shutdown

	DirectoryMonitorRecord* record_from_watch_descriptor(int watch_descriptor)
	{
		return nullptr;
	}

	void directory_monitor_update()
	{
	} // directory_monitor_update

	MonitorHandle directory_monitor_add(const char* path, MonitorDelegate delegate)
	{
		// Not yet implemented.
		assert(0);
		MonitorHandle handle = _monitor_state->records.size() + 1;
		return handle;
	} // directory_monitor_add

	void directory_monitor_remove(MonitorHandle handle)
	{
		// Not yet implemented.
		assert(0);
	} // directory_monitor_remove
} // namespace platform
