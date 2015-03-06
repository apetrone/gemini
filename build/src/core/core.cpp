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
#include <core/typedefs.h>

#include "core.h"
#include "filesystem.h"

#include <platform/platform.h>

#include "logging.h"
#include "logging_interface.h"

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
		
#if _WIN32
		void vs_message(logging::Handler* handler, const char* message, const char* filename, const char* function, int line, int type);
		int vs_open(logging::Handler* handler);
		void vs_close(logging::Handler* handler);
#endif
		
#if __ANDROID__
		void log_android_message(logging::Handler* handler, const char* message, const char* filename, const char* function, int line, int type);
		int log_android_open(logging::Handler* handler);
		void log_android_close(logging::Handler* handler);
#endif
				
		uint32_t add_log_handlers(core::logging::ILog* log_system)
		{
			uint32_t total_log_handlers = 0;
			

			
			{
				logging::Handler stdout_handler;
				stdout_handler.open = stdout_open;
				stdout_handler.close = stdout_close;
				stdout_handler.message = stdout_message;
				log_system->add_handler(&stdout_handler);
				
				++total_log_handlers;
			}

			
#if _WIN32
			logging::Handler msvc_logger;
			msvc_logger.open = vs_open;
			msvc_logger.close = vs_close;
			msvc_logger.message = vs_message;
			log_system->add_handler(&msvc_logger);
			
			++total_log_handlers;
#endif

#if !PLATFORM_IS_MOBILE
			platform::DateTime dt;
			platform::instance()->get_current_datetime(dt);
			
			char datetime_string[ GEMINI_DATETIME_STRING_MAX ];
			str::sprintf(datetime_string, GEMINI_DATETIME_STRING_MAX, "%02d-%02d-%04d-%02d-%02d-%02d.log",
						 dt.month, dt.day, dt.year, dt.hour, dt.minute, dt.second);
			
			char logdir[MAX_PATH_SIZE];
			memset(logdir, 0, MAX_PATH_SIZE);
			str::copy(logdir, filesystem::content_directory(), 0);
			str::cat(logdir, "/" GEMINI_LOG_PATH "/");
			platform::path::normalize(logdir);
			
			// make sure target folder is created
			platform::path::make_directories( logdir );
			
			core::str::cat(logdir, datetime_string);
			
			logging::Handler filelogger;
			filelogger.open = file_logger_open;
			filelogger.close = file_logger_close;
			filelogger.message = file_logger_message;
			filelogger.userdata = logdir;
			log_system->add_handler(&filelogger);
			
			++total_log_handlers;
#endif
			
#if __ANDROID__
			logging::Handler android_log;
			android_log.open = log_android_open;
			android_log.close = log_android_close;
			android_log.message = log_android_message;
			log_system->add_handler(&android_log);
			
			++total_log_handlers;
#endif

			return total_log_handlers;
		} // add_log_handlers
	} // namespace _internal

	
	platform::Result startup()
	{
		platform::Result result(platform::Result::Success);
		
		// create an instance of the log system
		core::logging::ILog* log_system = CREATE(core::logging::LogInterface);
		core::log::set_instance(log_system);

		// add logs
		uint32_t total_log_handlers = _internal::add_log_handlers(log_system);
		
		// startup the log system; open handlers
		if (log_system->startup() < total_log_handlers)
		{
			fprintf(stderr, "Could not open one or more log handlers\n");
			result = platform::Result(platform::Result::Warning, "Could not open one or more log handlers");
		}
		
		LOGV("Logging system initialized.\n");
		
		LOGV("setting root to '%s', content: '%s'\n", filesystem::root_directory(), filesystem::content_directory());
		
		return result;
	} // startup
	
	void shutdown()
	{
		core::log::instance()->shutdown();
		
		core::logging::ILog* log_system = core::log::instance();
		DESTROY(ILog, log_system);
	} // shutdown

} // namespace core