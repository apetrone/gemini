// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone
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
#include "typedefs.h"
#include "filesystem.h"

#include <core/logging.h>
#include <core/str.h>
#include <core/config.h>

#include <platform/platform.h>

#include <renderer/renderer.h>

#include "filesystem_interface.h"



#include <functional>

using platform::PathString;

namespace gemini
{
	namespace detail
	{
		// log handler function declarations
		void file_logger_message(core::logging::Handler* handler, const char* message, const char* filename, const char* function, int line, int type)
		{
			core::StackString<MAX_PATH_SIZE> path = filename;
			fprintf((FILE*)handler->userdata, "[%i %s %s %i] %s", type, path.basename()(), function, line, message);
			//fprintf( (FILE*)handler->userdata, "\t%s", message );
			fflush((FILE*)handler->userdata);
		}

		int file_logger_open(core::logging::Handler* handler)
		{
			const char* logname = (const char*)handler->userdata;
			handler->userdata = fopen(logname, "wb");
			return handler->userdata != 0;
		}

		void file_logger_close(core::logging::Handler* handler)
		{
			if (handler->userdata)
			{
				fclose((FILE*)handler->userdata);
			}
		}

		class RuntimeResourceProvider : public render2::ResourceProvider
		{
		public:
			virtual bool load_file(Array<unsigned char>& data, const char* filename) const
			{
				core::filesystem::instance()->virtual_load_file(data, filename);
				return true;
			}
		}; // RuntimeResourceProvider

		static RuntimeResourceProvider resource_provider;
	}

	// initialize filesystem

	// set paths

	// initialize handlers

	LIBRARY_EXPORT platform::Result runtime_startup(const char* application_data_path, std::function<void(const char*)> custom_path_setup)
	{
		platform::PathString root_path = platform::get_program_directory();
		platform::PathString content_path = platform::fs_content_directory();

		// create file system instance
		core::filesystem::IFileSystem* filesystem = MEMORY_NEW(core::filesystem::FileSystemInterface, core::memory::global_allocator());
		core::filesystem::set_instance(filesystem);
		if (!filesystem)
		{
			return platform::Result::failure("Unable to create filesystem instance");
		}

		if (custom_path_setup)
		{
			custom_path_setup(application_data_path);
		}
		else
		{
			// default path setu
			platform::PathString application_path = platform::get_user_application_directory(application_data_path);
			core::filesystem::instance()->root_directory(root_path);
			core::filesystem::instance()->content_directory(content_path);
			core::filesystem::instance()->user_application_directory(application_path);
		}

#if 0 && defined(PLATFORM_FILESYSTEM_SUPPORT)
		// install disk logging handler
		const char GEMINI_LOG_PATH[] = "logs";
		const unsigned int GEMINI_DATETIME_STRING_MAX = 128;

		platform::DateTime dt;
		platform::datetime(dt);


		core::filesystem::IFileSystem* fs = core::filesystem::instance();

		char datetime_string[ GEMINI_DATETIME_STRING_MAX ];
		core::str::sprintf(datetime_string, GEMINI_DATETIME_STRING_MAX, "%02d-%02d-%04d-%02d-%02d-%02d.log",
					 dt.month, dt.day, dt.year, dt.hour, dt.minute, dt.second);

		platform::PathString log_directory;
		log_directory = fs->user_application_directory();
		log_directory.append(PATH_SEPARATOR_STRING).append(GEMINI_LOG_PATH).append(PATH_SEPARATOR_STRING);
		log_directory.normalize(PATH_SEPARATOR);

		// make sure target folder is created
		platform::path::make_directories(log_directory());

		log_directory.append(datetime_string);

		core::logging::Handler filelogger;
		filelogger.open = detail::file_logger_open;
		filelogger.close = detail::file_logger_close;
		filelogger.message = detail::file_logger_message;
		filelogger.userdata = (void*)log_directory();
		core::logging::instance()->add_handler(&filelogger);
#endif

		// install the resource provider to the renderer?
		render2::set_resource_provider(&detail::resource_provider);

		return platform::Result::success();
	}

	LIBRARY_EXPORT void runtime_shutdown()
	{
		core::filesystem::instance()->shutdown();

		core::filesystem::IFileSystem* filesystem = core::filesystem::instance();
		MEMORY_DELETE(filesystem, core::memory::global_allocator());
	} // shutdown
} // namespace gemini
