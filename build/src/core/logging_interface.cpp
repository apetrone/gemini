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

#include "logging_interface.h"

namespace gemini
{
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
				int total_handlers_opened = 0;
			
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
		
		typedef Interface<logging::ILog> log;
	}
} // namespace gemini