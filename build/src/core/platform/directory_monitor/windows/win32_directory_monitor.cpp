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

#define WIN32_LEAN_AND_MEAN 1
#define NOMINMAX
#include <windows.h>

#include <direct.h> // for _mkdir
#include <string.h> // for strrchr
#include <Shlwapi.h> // for PathFileExists

#include <core/logging.h>
#include <core/mem.h>
#include <core/hashset.h>

// https://qualapps.blogspot.com/2010/05/understanding-readdirectorychangesw_19.html

namespace gemini
{
	const size_t DIRECTORY_MONITOR_MAX_BUFFERS = 2;
	const size_t DIRECTORY_MONITOR_BUFFER_SIZE = 4096;


	struct DirectoryMonitorRecord
	{
		MonitorDelegate callback;
		MonitorHandle monitor_handle;
		platform::PathString path;
		uint32_t buffer_index;
		HANDLE handle;
		BYTE* buffer;
		OVERLAPPED async_data;
	};

	struct DirectoryMonitorState
	{
		Allocator& allocator;
		Array<DirectoryMonitorRecord*> records;
		HashSet<platform::PathString, DirectoryMonitorRecord*> records_by_path;

		DirectoryMonitorState(Allocator& _allocator)
			: allocator(_allocator)
			, records(_allocator)
			, records_by_path(_allocator)
		{
		}

		DirectoryMonitorState(const DirectoryMonitorState& state) = delete;
		DirectoryMonitorState& operator=(const DirectoryMonitorState& other) = delete;
		DirectoryMonitorState& operator=(const DirectoryMonitorState&& other) = delete;
	}; // DirectoryMonitorState

	DirectoryMonitorState* _monitor_state = nullptr;

	bool directory_monitor_read_changes(DirectoryMonitorRecord* record);

	void WINAPI directory_monitor_completion_routine(DWORD error_code,
												DWORD bytes_transferred,
												OVERLAPPED* async_data)
	{
		if (error_code == ERROR_OPERATION_ABORTED)
		{
			// CancelIo was called -- this is the final notification.
			return;
		}

		if (bytes_transferred == 0)
		{
			return;
		}

		// cast the record from hEvent
		DirectoryMonitorRecord* record = reinterpret_cast<DirectoryMonitorRecord*>(async_data->hEvent);
		assert(record);

		// increment the buffer
		BYTE* buffer = record->buffer + (DIRECTORY_MONITOR_BUFFER_SIZE * record->buffer_index);
		++record->buffer_index;
		if (record->buffer_index >= DIRECTORY_MONITOR_MAX_BUFFERS)
		{
			// wrap back around
			record->buffer_index = 0;
		}

		// iterate over all file notification events...
		size_t index = 0;
		while (true)
		{
			FILE_NOTIFY_INFORMATION* notify_info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer + index);

			platform::PathString path;
			if (notify_info->FileNameLength > 0)
			{
				int count = WideCharToMultiByte(CP_ACP,
					0,
					notify_info->FileName,
					notify_info->FileNameLength / sizeof(WCHAR),
					&path[0],
					(MAX_PATH_SIZE - 1),
					NULL,
					NULL);
				path[count] = '\0';
				path.recompute_size();

				// The path returned above is a relative path to the directory
				// referenced in the record. Concatenate that path to make the
				// absolute path that we can pass to the callback.
				platform::PathString absolute_path = record->path;
				absolute_path.append(PATH_SEPARATOR_STRING);
				absolute_path.append(path);

				// TODO: dispatch the action type?
				MonitorAction action;
				switch (notify_info->Action)
				{
				case FILE_ACTION_ADDED:
					action = MonitorAction::Added;
					break;

				case FILE_ACTION_REMOVED:
					action = MonitorAction::Removed;
					break;

				case FILE_ACTION_MODIFIED:
					action = MonitorAction::Modified;
					break;

				case FILE_ACTION_RENAMED_OLD_NAME:
					// file was renamed and this is the old name
				case FILE_ACTION_RENAMED_NEW_NAME:
					// file was renamed and this is the new name
					break;
				}

				record->callback(record->monitor_handle, action, absolute_path);
			}

			++index;
			if (notify_info->NextEntryOffset == 0)
			{
				break;
			}
		}

		// re-issue read call with a different buffer to avoid race conditions
		directory_monitor_read_changes(record);

	} // directory_monitor_completion_routine

	bool directory_monitor_read_changes(DirectoryMonitorRecord* record)
	{
		DWORD notify_filter = FILE_NOTIFY_CHANGE_FILE_NAME | \
			FILE_NOTIFY_CHANGE_SIZE | \
			FILE_NOTIFY_CHANGE_CREATION;
		BOOL result = ReadDirectoryChangesW(record->handle,
			record->buffer + (DIRECTORY_MONITOR_BUFFER_SIZE * record->buffer_index),
			DIRECTORY_MONITOR_BUFFER_SIZE,
			FALSE,
			notify_filter,
			NULL, /* only needed for synchronous call */
			&record->async_data,
			directory_monitor_completion_routine);
		if (!result)
		{
			LOGV("ReadyDirectoryChangesW failed with error = %i\n", GetLastError());
		}

		return result;
	} // directory_monitor_read_changes

	int32_t directory_monitor_startup(Allocator& allocator)
	{
		assert(_monitor_state == nullptr);
		_monitor_state = MEMORY2_NEW(allocator, DirectoryMonitorState)(allocator);
		return 0;
	} // directory_monitor_startup

	void directory_monitor_shutdown()
	{
		assert(_monitor_state != nullptr);

		for (size_t index = 0; index < _monitor_state->records.size(); ++index)
		{
			DirectoryMonitorRecord* record = _monitor_state->records[index];

			// This is invoked because it forces all completion notifications
			// to happen one last time.
			CancelIo(record->handle);

			// Wait for i/o operations to finish up.
			if (!HasOverlappedIoCompleted(&record->async_data))
			{
				SleepEx(5, TRUE);
			}

			MEMORY2_DEALLOC(_monitor_state->allocator, record->buffer);
			CloseHandle(record->handle);
			MEMORY2_DELETE(_monitor_state->allocator, record);
		}

		MEMORY2_DELETE(_monitor_state->allocator, _monitor_state);
		_monitor_state = nullptr;
	} // directory_monitor_shutdown

	void directory_monitor_update()
	{
		for (size_t index = 0; index < _monitor_state->records.size(); ++index)
		{
			DirectoryMonitorRecord* record = _monitor_state->records[index];
			directory_monitor_read_changes(record);

			// Instead of using SleepEx, we use MsgWaitForMultiple here to
			// allow the completion routine to be called.
			MsgWaitForMultipleObjectsEx(0, NULL, 0, QS_ALLINPUT, MWMO_ALERTABLE);
		}
	} // directory_monitor_update

	MonitorHandle directory_monitor_add(const char* path, MonitorDelegate delegate)
	{
		MonitorHandle handle = _monitor_state->records.size() + 1;

		DirectoryMonitorRecord* record = MEMORY2_NEW(_monitor_state->allocator, DirectoryMonitorRecord);
		_monitor_state->records.push_back(record);
		record->callback = delegate;
		record->monitor_handle = handle;
		record->path = path;
		record->buffer_index = 0;

		// "ReadDirectoryChangesW fails with ERROR_NOACCESS when the buffer is not aligned on a DWORD boundary."
		record->buffer = static_cast<BYTE*>(MEMORY2_ALLOC_ALIGNED(_monitor_state->allocator, sizeof(DWORD), DIRECTORY_MONITOR_BUFFER_SIZE * DIRECTORY_MONITOR_MAX_BUFFERS));
		memset(record->buffer, 0, DIRECTORY_MONITOR_BUFFER_SIZE * DIRECTORY_MONITOR_MAX_BUFFERS);

		record->handle = CreateFileA(path,
			FILE_LIST_DIRECTORY, // access mode
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, // share mode
			NULL, // security descriptor
			OPEN_EXISTING, // how to create the handle
			FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, // file attributes
			NULL); // file with attributes to copy
		assert(record->handle != INVALID_HANDLE_VALUE);

		memset(&record->async_data, 0, sizeof(OVERLAPPED));
		record->async_data.hEvent = record;

		if (!directory_monitor_read_changes(record))
		{
			CloseHandle(record->handle);
			MEMORY2_DELETE(_monitor_state->allocator, record);
			return MonitorHandle(0);
		}

		return handle;
	} // directory_monitor_add

	void directory_monitor_remove(MonitorHandle handle)
	{
		DirectoryMonitorRecord* record = _monitor_state->records[handle - 1];
		_monitor_state->records_by_path.remove(record->path);
		_monitor_state->records.erase(record);
	} // directory_monitor_remove
} // namespace platform
