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

#include "logging_interface.h"

namespace core
{
	namespace logging
	{
		LogInterface::LogInterface()
		{
			handlers.allocate(LOG_MAX_HANDLERS);
		}

		LogInterface::~LogInterface()
		{
			handlers.clear();
		}

		void LogInterface::dispatch(ILog::MessageType type, const char *message, const char *function, const char* filename, int linenumber)
		{
			// dispatch message to all handlers
			for (size_t i = 0; i < handlers.size(); ++i)
			{
				Handler* handler = &handlers[i];
				if (handler->open && handler->message && handler->close)
				{
					handler->message(handler, message, filename, function, linenumber, type);
				}
			}
		}

		void LogInterface::add_handler(Handler *handler)
		{
			Handler* slot = 0;
			for (size_t i = 0; i < handlers.size(); ++i)
			{
				slot = &handlers[i];
				if (slot->open == 0 && slot->close == 0 && slot->message == 0)
				{
					break;
				}
				slot = 0;
			}

			// if we found an open slot...
			if (slot)
			{
				memcpy(slot, handler, sizeof(Handler));
			}
			else
			{
				// TODO: warn the user we've exceeded LOG_MAX_HANDLERS
			}
		}

		uint32_t LogInterface::startup()
		{
			uint32_t total_handlers_opened = 0;

			// initialize all log handlers
			for (size_t i = 0; i < handlers.size(); ++i)
			{
				Handler* handler = &handlers[i];
				if (!handler->open || (handler->open && !handler->open(handler)))
				{
					handler->open = 0;
				}
				else
				{
					++total_handlers_opened;
				}
			}

			// TODO: create mutex

			return total_handlers_opened;
		}

		void LogInterface::shutdown()
		{
			// close all log handlers
			for (size_t i = 0; i < handlers.size(); ++i)
			{
				Handler* handler = &handlers[i];
				if (handler->close)
				{
					handler->close(handler);
				}
			}

			// TODO: destroy mutex
		}
	} // namespace logging
} // namespace core
