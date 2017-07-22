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

#include <sys/inotify.h>
#include <unistd.h>

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
		assert(_monitor_state == nullptr);
		_monitor_state = MEMORY2_NEW(allocator, DirectoryMonitorState)(allocator);
		_monitor_state->inotify_instance = inotify_init1(IN_NONBLOCK);
		if (_monitor_state->inotify_instance == -1)
		{
			LOGE("inotify_init failed, errno = %i\n", errno);
			return errno;
		}
		return 0;
	} // directory_monitor_startup

	void directory_monitor_shutdown()
	{
		assert(_monitor_state != nullptr);

		for (size_t index = 0; index < _monitor_state->records.size(); ++index)
		{
			DirectoryMonitorRecord* record = _monitor_state->records[index];
			inotify_rm_watch(_monitor_state->inotify_instance, record->notify_handle);
			MEMORY2_DELETE(_monitor_state->allocator, record);
		}

		close(_monitor_state->inotify_instance);

		MEMORY2_DELETE(_monitor_state->allocator, _monitor_state);
		_monitor_state = nullptr;
	} // directory_monitor_shutdown

	DirectoryMonitorRecord* record_from_watch_descriptor(int watch_descriptor)
	{
		for (size_t index = 0; index < _monitor_state->records.size(); ++index)
		{
			if (_monitor_state->records[index]->notify_handle == watch_descriptor)
			{
				return _monitor_state->records[index];
			}
		}
		return nullptr;
	}

	void directory_monitor_update()
	{
		struct inotify_event* event;
		uint8_t buffer[1024] = {0};
		int32_t bytes_received = 0;

		bytes_received = read(_monitor_state->inotify_instance, buffer, 1024);
		if (bytes_received > 0)
		{
			event = reinterpret_cast<struct inotify_event*>(buffer);
			if (event)
			{
				DirectoryMonitorRecord* record = record_from_watch_descriptor(event->wd);
				if (!record)
				{
					return;
				}

				MonitorAction action = MonitorAction::Invalid;
				if (event->mask & IN_CREATE)
				{
					action = MonitorAction::Added;
				}
				else if (event->mask & IN_DELETE)
				{
					action = MonitorAction::Removed;
				}
				else if (event->mask & IN_MODIFY)
				{
					action = MonitorAction::Modified;
				}

				if (action != MonitorAction::Invalid)
				{
					platform::PathString absolute_path = record->path;
					absolute_path.append(PATH_SEPARATOR_STRING);
					absolute_path.append(event->name);
					record->callback(event->wd, action, absolute_path);
				}
			}
		}
	} // directory_monitor_update

	MonitorHandle directory_monitor_add(const char* path, MonitorDelegate delegate)
	{
		// user must have read permissions for this
		uint32_t mask = IN_CREATE | IN_DELETE | IN_MODIFY;
		int32_t inotify_handle = inotify_add_watch(
										_monitor_state->inotify_instance,
										path,
										mask);

		if (inotify_handle == -1)
		{
			if (errno == ENOSPC)
			{
				// No more watches left!
				LOGW("Upper limit of watches reached!\n");
			}

			LOGW("inotify_add_watch failed with errno = %i\n", errno);
			return MonitorHandle(0);
		}

		MonitorHandle handle = _monitor_state->records.size() + 1;
		DirectoryMonitorRecord* record = MEMORY2_NEW(_monitor_state->allocator, DirectoryMonitorRecord);
		_monitor_state->records.push_back(record);
		record->callback = delegate;
		record->path = path;
		record->notify_handle = inotify_handle;

		return handle;
	} // directory_monitor_add

	void directory_monitor_remove(MonitorHandle handle)
	{
		DirectoryMonitorRecord* record = _monitor_state->records[handle - 1];
		int32_t result = inotify_rm_watch(_monitor_state->inotify_instance, record->notify_handle);
		if (result != 0)
		{
			LOGW("inotify_rm_watch returned error: %i\n", errno);
			return;
		}
		_monitor_state->records.erase(record);
		MEMORY2_DELETE(_monitor_state->allocator, record);
	} // directory_monitor_remove
} // namespace platform
