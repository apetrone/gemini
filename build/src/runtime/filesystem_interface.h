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
#pragma once

#include <core/typedefs.h>
#include "filesystem.h"

#include <core/fixedarray.h>

namespace core
{
	namespace filesystem
	{
		class FileSystemInterface : public IFileSystem
		{
			::platform::PathString root_path;
			::platform::PathString user_application_path;
			::platform::PathString content_path;
		
		public:
			virtual ~FileSystemInterface();
			
			virtual void startup();
			virtual void shutdown();
			
			virtual bool file_exists(const char* path, bool path_is_relative) const;
			virtual bool directory_exists(const char* path, bool path_is_relative) const;
			
			virtual void root_directory(const ::platform::PathString& root_path);
			virtual const ::platform::PathString& root_directory() const;
			
			virtual void content_directory(const ::platform::PathString& content);
			virtual const ::platform::PathString& content_directory() const;

			virtual const ::platform::PathString& user_application_directory() const;
			virtual void user_application_directory(const ::platform::PathString& application_directory);
			
			virtual bool virtual_file_exists(const char* relative_path) const;
			virtual bool virtual_directory_exists(const char* relative_path) const;
			virtual char* virtual_load_file(const char* relative_path, char* buffer, size_t* buffer_length) const;
		};
	} // namespace filesystem
} // namespace core
