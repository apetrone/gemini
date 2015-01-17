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
#include <stdio.h> // for FILE, file functions

#if __ANDROID__
	#include <android/log.h>
#endif

#if _WIN32
#define WIN32_LEAN_AND_MEAN 1
	#include <windows.h>
#endif

#include <core/stackstring.h>
#include "logging.h"

using namespace gemini::core::logging;

namespace gemini
{
	namespace core
	{
		// log handler function definitions
		namespace _internal
		{
			void file_logger_message(Handler* handler, const char* message, const char* filename, const char* function, int line, int type)
			{
				StackString<MAX_PATH_SIZE> path = filename;
				fprintf((FILE*)handler->userdata, "[%i %s %s %i] %s", type, path.basename()(), function, line, message);
				//fprintf( (FILE*)handler->userdata, "\t%s", message );
				fflush((FILE*)handler->userdata);
			}
			
			int file_logger_open(Handler* handler)
			{
				const char* logname = (const char*)handler->userdata;
				handler->userdata = fopen(logname, "wb");
				return handler->userdata != 0;
			}
			
			void file_logger_close(Handler* handler)
			{
				if (handler->userdata)
				{
					fclose((FILE*)handler->userdata);
				}
			}
			
			
			void stdout_message(Handler* handler, const char* message, const char* filename, const char* function, int line, int type)
			{
				const char *message_types[] = {0, "VERBOSE", "WARNING", " ERROR "};
				StackString<MAX_PATH_SIZE> path = filename;
				fprintf(stdout, "[%s] %s, %s, %i | %s", message_types[type], path.basename()(), function, line, message);
				//fflush( stdout );
			}
			
			int stdout_open(Handler* handler)
			{
				return 1;
			}
			
			void stdout_close(Handler* handler)
			{
			}

	#if _WIN32
			void vs_message(Handler* handler, const char* message, const char* filename, const char* function, int line, int type)
			{
		//		const char *message_types[] = { 0, "VERBOSE", "WARNING", " ERROR " };
	//			fprintf( stdout, "[%s] %s, %s, %i | %s", message_types[ type ], xstr_filefrompath(filename), function, line, message );


				OutputDebugStringA(message);
			}
			
			int vs_open(Handler* handler)
			{
				return 1;
			}
			
			void vs_close(Handler* handler)
			{
			}	
	#endif
			
	#if __ANDROID__
			void log_android_message(Handler* handler, const char* message, const char* filename, const char* function, int line, int type)
			{
				// this must match with the android_LogPriority enum in <android/log.h>
				android_LogPriority message_types[] = {ANDROID_LOG_UNKNOWN, ANDROID_LOG_VERBOSE, ANDROID_LOG_WARN, ANDROID_LOG_ERROR};
				__android_log_print(message_types[ type ], "gemini", message);
			}
			
			int log_android_open(Handle* handler)
			{
				return 1;
			}
			
			void log_android_close(Handler* handler)
			{
			}
	#endif
		} // namespace _internal
	} // namespace core
} // namespace gemini