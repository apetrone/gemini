// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM,OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
// -------------------------------------------------------------
#pragma once

#include <core/str.h>

#define log_verbose(format, ...) \
	gemini::core::log::instance()->dispatch(\
	gemini::core::logging::ILog::Verbose,\
	xstr_format(format, ##__VA_ARGS__),\
	__FUNCTION__,\
	__FILE__,\
	__LINE__)

#define log_warning(format, ...) \
	gemini::core::log::instance()->dispatch(\
	gemini::core::logging::ILog::Warning,\
	gemini::core::str::format(format, ##__VA_ARGS__),\
	__FUNCTION__,\
	__FILE__,\
	__LINE__)

#define log_error(format, ...) \
	gemini::core::log::instance()->dispatch(\
	gemini::core::logging::ILog::Warning,\
	gemini::core::str::format(format, ##__VA_ARGS__),\
	__FUNCTION__,\
	__FILE__,\
	__LINE__)

#include <core/interface.h>

namespace gemini
{
	namespace core
	{
		namespace logging
		{
			struct Handler
			{
				// called when a log message is received
				void (*message)(struct Handler* handler, const char* message, const char* filename, const char* function, int line, int type );
				
				// called when the log should open
				// expects 0 on failure; 1 on success
				int (*open)(Handler* handler);
				
				// called when the log should close
				void (*close)(Handler* handler);
				
				// userdata
				void * userdata;
				
				Handler() : message(0), open(0), close(0), userdata(0) {}
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
			
			public:
				virtual ~ILog() {}
				
				/// @desc Dispatches a message to all handlers
				virtual void dispatch(ILog::MessageType type, const char* message, const char* function, const char* filename, int linenumber) = 0;
				
				/// @desc Add a log handler for dispatch
				/// @param handler Handler struct with pointers to functions
				virtual void add_handler(Handler* handler) = 0;
				
				/// @desc Open the log handlers
				/// @returns The total number of successfully opened handlers
				virtual uint32_t startup() = 0;
				
				/// @desc Shutdown the log handlers
				virtual void shutdown() = 0;
			}; // ILog
		} // namespace logging
		
		typedef Interface<logging::ILog> log;
	}
} // namespace gemini