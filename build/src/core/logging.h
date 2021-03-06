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

#include <core/str.h>

#define LOGV(...) \
	core::logging::instance()->dispatch(\
	core::logging::ILog::Verbose,\
	core::str::format(__VA_ARGS__),\
	__FUNCTION__,\
	__FILE__,\
	__LINE__)

#define LOGW(...) \
	core::logging::instance()->dispatch(\
	core::logging::ILog::Warning,\
	core::str::format(__VA_ARGS__),\
	__FUNCTION__,\
	__FILE__,\
	__LINE__)

#define LOGE(...) \
	core::logging::instance()->dispatch(\
	core::logging::ILog::Error,\
	core::str::format(__VA_ARGS__),\
	__FUNCTION__,\
	__FILE__,\
	__LINE__)

#include <core/interface.h>

namespace core
{
	namespace logging
	{
		struct Handler
		{
			// called when the log should open
			// expects 0 on failure; 1 on success
			int (*open)(Handler* handler);

			// called when the log should close
			void (*close)(Handler* handler);

			// called when a log message is received
			void (*message)(struct Handler* handler, const char* message, const char* filename, const char* function, int line, int type);

			// userdata
			void* userdata;

			Handler() : open(0), close(0), message(0), userdata(0) {}
		};

		class ILog
		{
		public:
			enum MessageType
			{
				Invalid,
				Verbose,
				Warning,
				Error
			};

			enum class Flags
			{
				Uninitialized,
				Initialized
			};

		public:
			virtual ~ILog() {}

			/// @brief Dispatches a message to all handlers
			virtual void dispatch(ILog::MessageType type, const char* message, const char* function, const char* filename, int linenumber) = 0;

			/// @brief Add a log handler for dispatch
			/// @param handler Handler struct with pointers to functions
			virtual void add_handler(Handler* handler) = 0;

			/// @brief Remove a log handler
			/// @param handler Handler previously passed to add_handler
			virtual void remove_handler(Handler* handler) = 0;

			/// @brief Open the log handlers
			/// @returns The total number of successfully opened handlers
//			virtual uint32_t startup() = 0;

			/// @brief Shutdown the log handlers
//			virtual void shutdown() = 0;
		}; // ILog

		GEMINI_DECLARE_INTERFACE(ILog);
	} // namespace logging
} // namespace core
