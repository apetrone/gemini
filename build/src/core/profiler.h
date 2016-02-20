// -------------------------------------------------------------
// Copyright (C) 2016- Adam Petrone
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

#include <core/typedefs.h>

// KNOWN ISSUES:
// The profiler can be confused if you use identical string names
// for different blocks. This is because (I'm guessing) the string pointers
// will be identical in the text segment of the binary -- even though they are
// referenced from different compilation units. I should verify this.

// enable this to allow profiling
#define GEMINI_ENABLE_PROFILER

namespace gemini
{
#if defined(GEMINI_ENABLE_PROFILER)
	#define PROFILE_BEGIN(x) ::gemini::profiler::begin_scope(x, PLATFORM_FANCY_FUNCTION)
	#define PROFILE_END(x) ::gemini::profiler::end_scope(x, PLATFORM_FANCY_FUNCTION)

	namespace profiler
	{
		struct profile_block
		{
			uint32_t parent_index;
			uint32_t index;
			uint32_t hitcount;
			uint32_t depth;
			uint64_t cycles;
			const char* name;
		};

		typedef void (*profile_callback)(const char* name, uint64_t cycles, uint32_t depth, uint32_t hitcount, float parent_weight);

		void begin_scope(const char* name, const char* fancy_name);
		void end_scope(const char* name, const char* caller_name);
		void report(profile_callback callback);
		void reset();
		void startup();
		void shutdown();

	} // namespace profiler
#else
	#define PROFILE_BEGIN(x)
	#define PROFILE_END(x)
#endif
} // namespace gemini