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
#include <core/core.h>
#include <core/logging.h>
#include <core/logging_interface.h>
#include <core/mem.h>
#include <core/profiler.h>

#include <platform/platform.h>

namespace gemini
{
	// The logging interface is quite unique in that it needs to exist
	// as systems startup and survive until the last bit of statics go away.
	// For this, we'll allocate this in static memory.
	StaticMemory<core::logging::LogInterface> log_system_data;

#if defined(GEMINI_ENABLE_PROFILER)
	gemini::Allocator _profile_allocator;
#endif

	platform::Result core_startup()
	{
		// create an instance of the log system
		core::logging::ILog* log_system = memory_static_allocate(log_system_data);
		core::logging::set_instance(log_system);

		memory_startup();

		platform::Result result = platform::startup();
		assert(result.succeeded());

#if defined(GEMINI_ENABLE_PROFILER)
		_profile_allocator = memory_allocator_default(MEMORY_ZONE_DEFAULT);
		profiler::startup(_profile_allocator);
#endif

		return platform::Result::success();
	}

	void core_shutdown()
	{
		platform::shutdown();

#if defined(GEMINI_ENABLE_PROFILER)
		profiler::shutdown();
#endif

		memory_shutdown();

		core::logging::ILog* log_system = core::logging::instance();
		log_system->~ILog();
		core::logging::set_instance(nullptr);
	}
} // namespace gemini
