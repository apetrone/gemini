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

#if defined(PLATFORM_RASPBERRYPI)
	#include <bcm_host.h>

	// PLATFORM_SUPPORT_EGL and PLATFORM_SUPPORT_OPENGLES must be defined
	#if !(defined(PLATFORM_EGL_SUPPORT) || !defined(PLATFORM_OPENGLES_SUPPORT))
		#error RaspberryPi requires EGL and OpenGL ES support!
	#endif
#endif


#if defined(PLATFORM_EGL_SUPPORT)
	#include "../window/linux/egl/egl_backend.h"
#endif

namespace platform
{

	Result os_startup()
	{

		return Result(Result::Success);
	}
	
	void os_shutdown()
	{

	}
	
	int os_run_application(int argc, const char** argv)
	{
		return 0;
	}


	void linux_window_backend_startup()
	{
#if defined(PLATFORM_RASPBERRYPI)
		// this must be called before we can issue any GPU commands
		back_host_init();
#endif



#if defined(PLATFORM_EGL_SUPPORT)
		egl_backend_startup();
#endif
	}

	void linux_window_backend_shutdown()
	{
#if defined(PLATFORM_EGL_SUPPORT)
		egl_backend_shutdown();
#endif
	}
} // namespace platform