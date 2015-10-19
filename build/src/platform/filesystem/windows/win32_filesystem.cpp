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
#define NOMINMAX
#include <windows.h>

#include <direct.h> // for _mkdir
#include <string.h> // for strrchr
#include <Shlwapi.h> // for PathFileExists

namespace platform
{
	const unsigned int STATIC_BUFFER_SIZE = 8192;
	static char _static_buffer[STATIC_BUFFER_SIZE] = {0};

	PathString get_program_directory()
	{
		PathString directory;

		Result error;

		int result = 0;
		char* sep;
		result = GetModuleFileNameA(GetModuleHandleA(0), &directory[0], directory.max_size());

		// GetModuleFilenameA failed!
		assert(result != 0);

		char* path = directory();
		if (result != 0)
		{
			sep = strrchr(path, PATH_SEPARATOR);

			if (sep)
			{
				size_t index = (sep - path);
				directory[index] = '\0';
				directory.recompute_size();
			}
		}
		
		return directory;
	}

	Result make_directory(const char* path)
	{
		Result result;
		int status_code = 0;

		status_code = _mkdir(path);
		if (status_code == -1)
		{
			// TODO: print out the errno
			result = Result::failure("_mkdir failed!");
		}

		return result;
	}

	const char* get_environment_variable(const char* name)
	{
		memset(_static_buffer, 0, STATIC_BUFFER_SIZE);
		GetEnvironmentVariableA(name, _static_buffer, STATIC_BUFFER_SIZE);
		return _static_buffer;
	}

	PathString get_user_directory()
	{
		return get_environment_variable("HOMEPATH");
	}

	PathString get_user_application_directory(const char* application_data_path)
	{
		PathString result = get_environment_variable("LOCALAPPDATA");
		result.append(PATH_SEPARATOR_STRING);
		result.append(application_data_path);
		result.normalize(PATH_SEPARATOR);
		return result;
	}

	PathString get_user_temp_directory()
	{
		PathString result = get_environment_variable("TEMP");
		return result;
	}

	platform::File fs_open(const char* path, FileMode mode)
	{
		platform::File file;
		DWORD desired_access = 0;
		// allow subsequent open operations on a file to request READ access
		DWORD share_mode = FILE_SHARE_READ;
		DWORD creation_disposition = 0;
		DWORD flags = FILE_ATTRIBUTE_NORMAL;

		if (mode == FileMode_Read)
		{
			creation_disposition |= OPEN_EXISTING;
			desired_access = GENERIC_READ;
		}
		else if (mode == FileMode_Write)
		{
			creation_disposition |= CREATE_ALWAYS;
			desired_access = GENERIC_WRITE;
		}

		file.handle = CreateFileA(path, desired_access, share_mode, NULL, creation_disposition, flags, NULL);
		assert(file.handle != INVALID_HANDLE_VALUE);

		return file;
	}

	void fs_close(platform::File file)
	{
		assert(CloseHandle(file.handle));
	}

	size_t fs_read(platform::File file, void* destination, size_t size, size_t count)
	{
		OVERLAPPED* overlapped = NULL;
		DWORD bytes_read = 0;
		assert(ReadFile(file.handle, destination, size*count, &bytes_read, overlapped));

		return bytes_read;
	}

	size_t fs_write(platform::File file, const void* source, size_t size, size_t count)
	{
		OVERLAPPED* overlapped = NULL;
		DWORD bytes_written = 0;
		assert(WriteFile(file.handle, source, size*count, &bytes_written, overlapped));
		return bytes_written;
	}

	int32_t fs_seek(platform::File file, long int offset, FileSeek origin)
	{
		DWORD move_method;
		switch (origin)
		{
			case FileSeek_Begin: move_method = FILE_BEGIN; break;
			case FileSeek_Relative: move_method = FILE_CURRENT; break;
			case FileSeek_End: move_method = FILE_END; break;
			default:
				// Unknown FileSeek enum
				assert(0);
		}

		return SetFilePointer(file.handle, offset, NULL, move_method);
	}

	long int fs_tell(platform::File file)
	{
		// offset is 0, move from the current file position
		// this essentially returns the current file pointer
		// see: http://stackoverflow.com/questions/17707020/is-there-no-getfilepointerex-windows-api-function
		return SetFilePointer(file.handle, 0, NULL, FILE_CURRENT);
	}

	bool fs_file_exists(const char* path)
	{
		return PathFileExistsA(path);
	}

	bool fs_directory_exists(const char* path)
	{
		return PathFileExistsA(path);
	}

	PathString fs_content_directory()
	{
		return get_program_directory();
	}
} // namespace platform