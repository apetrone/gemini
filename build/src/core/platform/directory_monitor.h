// -------------------------------------------------------------
// Copyright (C) 2017- Adam Petrone
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
#include <core/util.h>

namespace gemini
{
	typedef uint32_t MonitorHandle;

	enum class MonitorAction
	{
		Invalid,

		// the file was added to the watched directory
		Added,

		// the file was removed from the watched directory
		Removed,

		// the file was modified
		Modified
	};

	typedef Delegate<void(MonitorHandle, MonitorAction, const platform::PathString&)> MonitorDelegate;

	// returns 0 on success; non-zero on failure
	int32_t directory_monitor_startup(gemini::Allocator& allocator);

	// shutdown the directory monitoring system
	void directory_monitor_shutdown();

	// call this periodically to scan for and dispatch changes.
	void directory_monitor_update();

	// add a path to the watch list
	MonitorHandle directory_monitor_add(const char* path, MonitorDelegate delegate);

	// remove a monitor handle from being watched
	void directory_monitor_remove(MonitorHandle handle);
} // namespace gemini