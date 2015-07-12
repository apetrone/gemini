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
#include "core.h"
#include "filesystem.h"
#include "logging.h"
#include "logging_interface.h"

#include "filesystem_interface.h"

#include <core/config.h>
#include <platform/platform.h>

using platform::PathString;

namespace core
{
	namespace _internal
	{
		#define GEMINI_LOG_PATH "logs"
		const unsigned int GEMINI_DATETIME_STRING_MAX = 128;

		// log handler function declarations
		void file_logger_message(logging::Handler* handler, const char* message, const char* filename, const char* function, int line, int type);
		int file_logger_open(logging::Handler* handler);
		void file_logger_close(logging::Handler* handler);
		
		void stdout_message(logging::Handler* handler, const char* message, const char* filename, const char* function, int line, int type);
		int stdout_open(logging::Handler* handler);
		void stdout_close(logging::Handler* handler);
		
#if defined(PLATFORM_WINDOWS)
		void vs_message(logging::Handler* handler, const char* message, const char* filename, const char* function, int line, int type);
		int vs_open(logging::Handler* handler);
		void vs_close(logging::Handler* handler);
#endif
		
#if defined(PLATFORM_ANDROID)
		void log_android_message(logging::Handler* handler, const char* message, const char* filename, const char* function, int line, int type);
		int log_android_open(logging::Handler* handler);
		void log_android_close(logging::Handler* handler);
#endif
				
		platform::Result initialize_log_handlers(core::logging::ILog* log_system)
		{
			platform::Result result(platform::Result::Success);
			uint32_t total_log_handlers = 0;
			

			
			{
				logging::Handler stdout_handler;
				stdout_handler.open = stdout_open;
				stdout_handler.close = stdout_close;
				stdout_handler.message = stdout_message;
				log_system->add_handler(&stdout_handler);
				
				++total_log_handlers;
			}

			
#if defined(PLATFORM_WINDOWS)
			logging::Handler msvc_logger;
			msvc_logger.open = vs_open;
			msvc_logger.close = vs_close;
			msvc_logger.message = vs_message;
			log_system->add_handler(&msvc_logger);
			
			++total_log_handlers;
#endif

#if defined(PLATFORM_FILESYSTEM_SUPPORT)
			platform::DateTime dt;
			platform::datetime(dt);
			
			
			core::filesystem::IFileSystem* fs = core::filesystem::instance();
			
			char datetime_string[ GEMINI_DATETIME_STRING_MAX ];
			str::sprintf(datetime_string, GEMINI_DATETIME_STRING_MAX, "%02d-%02d-%04d-%02d-%02d-%02d.log",
						 dt.month, dt.day, dt.year, dt.hour, dt.minute, dt.second);
			
			platform::PathString log_directory;
			log_directory = fs->user_application_directory();
			log_directory.append(PATH_SEPARATOR_STRING).append(GEMINI_LOG_PATH).append(PATH_SEPARATOR_STRING);
			log_directory.normalize(PATH_SEPARATOR);
			
			// make sure target folder is created
			platform::path::make_directories(log_directory());
			
			log_directory.append(datetime_string);
			
			logging::Handler filelogger;
			filelogger.open = file_logger_open;
			filelogger.close = file_logger_close;
			filelogger.message = file_logger_message;
			filelogger.userdata = (void*)log_directory();
			log_system->add_handler(&filelogger);
			
			++total_log_handlers;
#endif
			
#if defined(PLATFORM_ANDROID)
			logging::Handler android_log;
			android_log.open = log_android_open;
			android_log.close = log_android_close;
			android_log.message = log_android_message;
			log_system->add_handler(&android_log);
			
			++total_log_handlers;
#endif

			// startup the log system; open handlers
			if (log_system->startup() < total_log_handlers)
			{
				fprintf(stderr, "Could not open one or more log handlers\n");
				result = platform::Result(platform::Result::Warning, "Could not open one or more log handlers");
			}

			return result;
		} // initialize_log_handlers
	} // namespace _internal
	
	
	platform::Result startup_filesystem()
	{
		platform::Result result(platform::Result::Success);
		
		// create file system instance
		core::filesystem::IFileSystem* filesystem = MEMORY_NEW(core::filesystem::FileSystemInterface, core::memory::global_allocator());
		core::filesystem::set_instance(filesystem);
		

		if (!filesystem)
		{
			result.message = "Unable to create filesystem instance";
			result.status = platform::Result::Failure;
		}
		
		return result;
	}
	
	platform::Result startup_logging()
	{
		platform::Result result(platform::Result::Success);
		
		if (!core::filesystem::instance())
		{
			result.message = "Filesystem instance is required for logging";
			result.status = platform::Result::Failure;
			return result;
		}
		
		
		
		// create an instance of the log system
		core::logging::ILog* log_system = MEMORY_NEW(core::logging::LogInterface, core::memory::global_allocator());
		core::logging::set_instance(log_system);
		
		// add logs
		result = _internal::initialize_log_handlers(log_system);
		

		return result;
	}
	
	void shutdown()
	{
		core::logging::instance()->shutdown();
		
		core::filesystem::instance()->shutdown();
		
		
		
		core::logging::ILog* log_system = core::logging::instance();
		MEMORY_DELETE(log_system, core::memory::global_allocator());
		
		core::filesystem::IFileSystem* filesystem = core::filesystem::instance();
		MEMORY_DELETE(filesystem, core::memory::global_allocator());
	} // shutdown

} // namespace core