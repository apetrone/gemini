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
#include <commdlg.h> // for OPENFILENAME, GetOpenFileName, GetSaveFileName

namespace platform
{
	static unsigned int _previous_error_mode;


	void joystick_startup();
	void joystick_shutdown();
	void joystick_update(float delta_milliseconds);

	Result backend_startup()
	{
		// prevent error dialogs from hanging the process.
		// these errors are forwarded to calling process.
		_previous_error_mode = SetErrorMode(SEM_FAILCRITICALERRORS);

		joystick_startup();

		return Result::success();
	} // backend_startup

	void backend_shutdown()
	{
		joystick_shutdown();

		// restore the error mode
		SetErrorMode(_previous_error_mode);
	} // backend_shutdown


	void backend_update(float delta_milliseconds)
	{
		// dispatch available window events
		window::dispatch_events();

		// update joysticks
		joystick_update(delta_milliseconds);
	} // backend_update

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

	Process* process_create(
		const char*,
		const Array<PathString>&,
		const char*
		)
	{
		return nullptr;
	} // process_create

	void process_destroy(Process*)
	{
	} // process_destroy

	bool process_is_running(Process*)
	{
		return false;
	} // process_is_running

	// Creates a filter string for use with OPENFILENAME structure.
	// The resulting char* MUST BE DEALLOCATED by the caller.
	char* create_filter_string(const Array<PlatformExtensionDescription>& extensions)
	{
		// determine the size of the buffer we need for the filter string.
		size_t filter_buffer_size = 0;
		for (size_t index = 0; index < extensions.size(); ++index)
		{
			// each entry is '<description> (*.<extension>)\0*.<extension>\0'
			const PlatformExtensionDescription& extensiondesc = extensions[index];

			filter_buffer_size += \
				core::str::len(extensiondesc.description) + \
				2 * core::str::len(extensiondesc.extension) + \
				8; // spaces, parens, terminators, etc.
		}

		// We need a buffer size of fiter_buffer_size bytes to pass to the
		// dialog function. This buffer requires TWO NULL TERMINATORS.
		char* filter_buffer = static_cast<char*>(MEMORY_ALLOC(filter_buffer_size + 2, core::memory::global_allocator()));
		memset(filter_buffer, 0, filter_buffer_size + 2);

		char* working_buffer = filter_buffer;

		// Now populate the filters string.
		for (size_t index = 0; index < extensions.size(); ++index)
		{
			// each entry is '<description> (*.<extension>)\0*.<extension>\0'
			const PlatformExtensionDescription& extensiondesc = extensions[index];

			char* description_line = core::str::format("%s (*.%s)",
				extensiondesc.description,
				extensiondesc.extension);
			core::str::cat(working_buffer, description_line);

			size_t buffer_offset = 0;

			buffer_offset = core::str::len(description_line);
			working_buffer += buffer_offset + 1;

			buffer_offset = 0;
			char* filter_line = core::str::format("*.%s", extensiondesc.extension);
			core::str::cat(working_buffer, filter_line);
			buffer_offset = core::str::len(filter_line);

			working_buffer += buffer_offset + 1;
		}

		// terminate the filter_buffer
		working_buffer[0] = '\0';

		return filter_buffer;
	}

	// These should be converted to use the OFNHookProc.
	// https://msdn.microsoft.com/en-us/library/ms646931(v=vs.85).aspx

	Result show_open_dialog(const char* title, uint32_t open_flags, Array<PathString>& paths)
	{
		char filename_buffer[MAX_PATH_SIZE] = { 0 };

		OPENFILENAMEA info;
		ZeroMemory(&info, sizeof(OPENFILENAMEA));

		info.lStructSize = sizeof(OPENFILENAMEA);
		info.hwndOwner = GetFocus();

		info.lpstrTitle = title;
		info.lpstrFile = filename_buffer;
		info.nMaxFile = MAX_PATH_SIZE;

		if (open_flags & OpenDialogFlags::ShowHiddenFiles)
		{
			info.Flags |= OFN_FORCESHOWHIDDEN;
		}

		if (open_flags & OpenDialogFlags::AllowMultiselect)
		{
			info.Flags |= OFN_ALLOWMULTISELECT;
		}

		if (GetOpenFileNameA(&info))
		{
			paths.push_back(info.lpstrFile);
			return Result::success();
		}

		return Result::failure("Not implemented");
	} // show_open_dialog

	Result show_save_dialog(const char* title,
							uint32_t save_flags,
							const Array<PlatformExtensionDescription>& extensions,
							const PathString& default_extension,
							PathString& filename)
	{
		char filename_buffer[MAX_PATH_SIZE] = { 0 };

		char* filter_buffer = create_filter_string(extensions);

		OPENFILENAMEA info;
		ZeroMemory(&info, sizeof(OPENFILENAMEA));

		info.lStructSize = sizeof(OPENFILENAMEA);
		info.hwndOwner = GetFocus();

		info.lpstrTitle = title;
		info.lpstrFilter = filter_buffer;
		info.lpstrFile = filename_buffer;
		info.nMaxFile = MAX_PATH_SIZE;

		info.Flags = OFN_OVERWRITEPROMPT;

		if (save_flags & OpenDialogFlags::ShowHiddenFiles)
		{
			info.Flags |= OFN_FORCESHOWHIDDEN;
		}

		if (save_flags & OpenDialogFlags::AllowMultiselect)
		{
			info.Flags |= OFN_ALLOWMULTISELECT;
		}

		info.lpstrDefExt = default_extension();

		if (GetSaveFileNameA(&info))
		{
			MEMORY_DEALLOC(filter_buffer, core::memory::global_allocator());
			filename = filename_buffer;
			return Result::success();
		}

		MEMORY_DEALLOC(filter_buffer, core::memory::global_allocator());

		return Result::failure("User canceled dialog.");
	} // show_save_dialog


} // namespace platform
