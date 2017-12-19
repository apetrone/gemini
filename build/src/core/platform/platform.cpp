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
#include "platform_internal.h"
#include "kernel.h"

#include <core/logging.h>
#include <core/mathlib.h>
#include <core/mem.h>
#include <core/core.h>

#include <assert.h>

#if defined(PLATFORM_WINDOWS)
	//#include <windows.h>
	//#include <direct.h> // for _mkdir
#elif defined(PLATFORM_LINUX)
	#include <sys/sysinfo.h>
	//#include <errno.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <stdio.h> // for snprintf
	#include <stdlib.h> // for abort
	#include <unistd.h> // for readlink, getpid

#elif defined(PLATFORM_APPLE)
	#include <stdio.h>
	#include <sys/stat.h>
	#include <sys/types.h>
	#include <inttypes.h>
	#include <stdlib.h> // for realpath
	#include <unistd.h>	// for fork
#endif

#include <string.h> // for strrchr



namespace platform
{
	MainParameters _mainparameters;
	gemini::Allocator _platform_allocator;

	gemini::Allocator& get_platform_allocator2()
	{
		return _platform_allocator;
	}

	DynamicLibrary::~DynamicLibrary()
	{
	}

	Process::~Process()
	{
	}

	Serial::~Serial()
	{
	}

	Thread::~Thread()
	{
	}

	Mutex::~Mutex()
	{
	}

	Semaphore::~Semaphore()
	{
	}


	namespace detail
	{
		void stdout_message(core::logging::Handler*, const char* message, const char* filename, const char* function, int line, int type)
		{
			FILE* log_message_to_pipe[] = {
				stdout,
				stdout,
				stdout,
				stderr
			};

			const char *message_types[] = {"INVALID", "VERBOSE", "WARNING", "ERROR"};
			core::StackString<MAX_PATH_SIZE> path = filename;
			fprintf(log_message_to_pipe[static_cast<int>(type)], "[%s] %s, %s, %i | %s", message_types[type], path.basename()(), function, line, message);
		}

		int stdout_open(core::logging::Handler*)
		{
			return 1;
		}

		void stdout_close(core::logging::Handler*)
		{
		}

#if defined(PLATFORM_WINDOWS)
		void vs_message(core::logging::Handler*, const char* message, const char*, const char*, int, int)
		{
			// honey badger don't care about LogMessageType
			OutputDebugStringA(message);
		}

		int vs_open(core::logging::Handler*)
		{
			return 1;
		}

		void vs_close(core::logging::Handler*)
		{
		}
#endif

#if defined(PLATFORM_ANDROID)
		void log_android_message(core::logging::Handler* handler, const char* message, const char* filename, const char* function, int line, int type)
		{
			// this must match with the android_LogPriority enum in <android/log.h>
			android_LogPriority message_types[] = {
				ANDROID_LOG_UNKNOWN,
				ANDROID_LOG_VERBOSE,
				ANDROID_LOG_WARN,
				ANDROID_LOG_ERROR
			};
			__android_log_print(message_types[static_cast<int>(type)], "gemini", "%s", message);
		}

		int log_android_open(core::logging::Handler* handler)
		{
			return 1;
		}

		void log_android_close(core::logging::Handler* handler)
		{
		}
#endif
	}



	Result startup()
	{
		Result result;

		// add platform-level logging handlers
		{
			core::logging::ILog* log_system = core::logging::instance();
			using namespace detail;

			core::logging::Handler stdout_handler;
			stdout_handler.open = stdout_open;
			stdout_handler.close = stdout_close;
			stdout_handler.message = stdout_message;
			assert(log_system != nullptr);
			log_system->add_handler(&stdout_handler);

#if defined(PLATFORM_WINDOWS)
			if (IsDebuggerPresent())
			{
				core::logging::Handler msvc_logger;
				msvc_logger.open = vs_open;
				msvc_logger.close = vs_close;
				msvc_logger.message = vs_message;
				log_system->add_handler(&msvc_logger);
			}
#endif

#if defined(PLATFORM_ANDROID)
			core::logging::Handler android_log;
			android_log.open = log_android_open;
			android_log.close = log_android_close;
			android_log.message = log_android_message;
			log_system->add_handler(&android_log);
#endif
		}

		_platform_allocator = gemini::memory_allocator_default(gemini::MEMORY_ZONE_PLATFORM);

		result = backend_startup();
		if (result.failed())
		{
			LOGE("backend_startup failed: '%s'\n", result.message);
		}

		result = timer_startup();
		if (result.failed())
		{
			LOGE("timer_startup failed: '%s'\n", result.message);
		}

		return result;
	}

	void shutdown()
	{
		timer_shutdown();
		backend_shutdown();
	}

	void update(float delta_milliseconds)
	{
		backend_update(delta_milliseconds);
	}

	// This nastiness MUST be here, because on different platforms
	// platform::startup is called at different times. This is done to allow
	// startup and shutdown on the correct threads via certain platforms.
#if defined(PLATFORM_LINUX) || defined(PLATFORM_WINDOWS)
	kernel::Error backend_run_application()
	{
		gemini::core_startup();

#if defined(PLATFORM_WINDOWS)
		// set title bar height, debugdraw uses this as an offset for text.
//		kernel::parameters().titlebar_height = GetSystemMetrics(SM_CYCAPTION);
#endif

		uint64_t last_frame_microseconds = platform::microseconds();

		// attempt kernel startup, mostly initializing core systems
		kernel::Error error = kernel::startup();

		if (error != kernel::NoError)
		{
			LOGE("Kernel startup failed with kernel code: %i\n", error);
			platform::shutdown();
			gemini::core_shutdown();
			return kernel::StartupFailed;
		}
		else
		{
			uint64_t current_time = platform::microseconds();
			LOGV("Kernel startup in %2.2fms\n", (current_time - last_frame_microseconds) * MillisecondsPerMicrosecond);
			last_frame_microseconds = current_time;

			double accumulator = 0.0;

			// startup succeeded; enter main loop
			while(kernel::instance() && kernel::instance()->is_active())
			{
				uint64_t current_time = platform::microseconds();
				kernel::Parameters& params = kernel::parameters();

				// calculate delta ticks in milliseconds
				params.framedelta_milliseconds = (current_time - last_frame_microseconds) * MillisecondsPerMicrosecond;

				// cache the value in seconds
				params.framedelta_seconds = params.framedelta_milliseconds * SecondsPerMillisecond;

				params.simulation_delta_seconds = (params.framedelta_milliseconds * params.simulation_time_scale) * SecondsPerMillisecond;

				last_frame_microseconds = current_time;

				//PROFILE_BEGIN("platform_update");
				platform::update(kernel::parameters().framedelta_milliseconds);
				//PROFILE_END("platform_update");

				accumulator += params.framedelta_seconds;

				uint8_t performed_fixed_update = 0;

				// If the frame time is so fast we don't have enough accuracy
				// to accumulate time, break after the iteration limit.
				const uint32_t MAX_ITERATIONS_BEFORE_TICK = 8;

				uint32_t iteration_count = 0;
				while (accumulator > params.step_interval_seconds && (iteration_count < MAX_ITERATIONS_BEFORE_TICK))
				{
					Array<gemini::InputMessage> &events = gemini::kernel_events();
					for (size_t index = 0; index < events.size(); ++index)
					{
						kernel::instance()->handle_input(events[index]);
					}
					gemini::kernel_event_reset();

					kernel::instance()->fixed_update(params.step_interval_seconds);

					// subtract the interval from the accumulator
					accumulator -= params.step_interval_seconds;

					// increment tick counter
					params.current_physics_tick++;

					performed_fixed_update = 1;

					++iteration_count;
				}

				// calculate the interpolation alpha that can be used with this frame.
				params.step_alpha = glm::clamp(static_cast<float>(accumulator / params.step_interval_seconds), 0.0f, 1.0f);

				kernel::instance()->tick(performed_fixed_update > 0);
			}
		}

		// cleanup kernel memory
		kernel::shutdown();
		gemini::core_shutdown();

		return error;
	} // main
#elif defined(PLATFORM_APPLE)
	// Handled through Cocoa's backend_run_application
#elif defined(PLATFORM_ANDROID)
	// Handled through Android's backend_run_application
#else
	#error Unknown platform!
#endif




	int run_application(kernel::IKernel* instance)
	{
		int return_code = -1;
		kernel::set_instance(instance);
		instance->set_active(true);

		// register event handlers
		kernel::assign_listener_for_eventtype(kernel::Keyboard, instance);
		kernel::assign_listener_for_eventtype(kernel::Mouse, instance);
		kernel::assign_listener_for_eventtype(kernel::System, instance);
		kernel::assign_listener_for_eventtype(kernel::GameController, instance);

#if defined(PLATFORM_APPLE)
		return_code = backend_run_application(_mainparameters.argc, (const char**)_mainparameters.argv);
#elif defined(PLATFORM_LINUX) || defined(PLATFORM_WINDOWS)
		return_code = backend_run_application();
#elif defined(PLATFORM_ANDROID)
		// Android will use the Java activity; not launch from NativeActivity.
		backend_run_application(_mainparameters.app);
#else
	#error Unknown platform!
#endif
		// instance is owned by the platform
		delete instance;

		return return_code;
	}


	void set_mainparameters(const MainParameters& params)
	{
		_mainparameters = params;
	}

	const MainParameters& get_mainparameters()
	{
		return _mainparameters;
	}

	namespace path
	{
		void normalize(char* path)
		{
			while(*path)
			{
				if (*path == '/' || *path == '\\')
				{
					// conform to this platform's path separator
					*path = PATH_SEPARATOR;
				}

				++path;
			}
		} // normalize


		platform::Result make_directories(const char* normalized_path)
		{
			const char* path = normalized_path;
			char directory[MAX_PATH_SIZE];

			// don't accept paths that are too short
			if (strlen(normalized_path) < 2)
			{
				return Result::failure("Path is too short");
			}

			memset(directory, 0, MAX_PATH_SIZE);

			// loop through and call mkdir on each separate directory
			// leading up to the final directory.
			while(*path)
			{
				if (*path == PATH_SEPARATOR)
				{
					core::str::copy(directory, normalized_path, static_cast<size_t>((path + 1) - normalized_path));
					if (!platform::fs_directory_exists(directory))
					{
						platform::Result result = platform::make_directory(directory);
						if (result.failed())
						{
							return result;
						}
					}
				}

				++path;
			}

			return platform::make_directory(normalized_path);
		} // make_directories
	} // namespace path
} // namespace platform
