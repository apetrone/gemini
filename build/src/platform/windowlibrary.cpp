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

#include "mem.h"
#include "config.h"
//#include "windowlibrary.h"
#include "platform.h"

#if defined(PLATFORM_IS_MOBILE)
	#error Not supported!
#else
	#include "sdl_windowlibrary.h"
	typedef platform::SDLWindowLibrary WindowLibrary;
#endif

using namespace platform;

namespace platform
{
	namespace detail
	{
		IWindowLibrary* windowlibrary = 0;
	}
	

		
		uint32_t get_renderinterface_caps()
		{
			uint32_t caps = 0;
			
#if defined(PLATFORM_SUPPORT_OPENGL)
			caps |= RenderBackend_OpenGL;
#elif defined(PLATFORM_SUPPORT_OPENGLES)
			caps |= RenderBackend_OpenGLES;
#else
	#error Unknown or unsupported render system
#endif
			
			return caps;
		}
		
		

	
	
	void startup_window_interface(
		const platform::RenderBackend& render_interface,
		const InputBackend& input_backend)
	{
		
	}
	
	void shutdown_window_interface()
	{
		
	}
	
	
	
	
	// don't bloat my code up
	WindowParameters::~WindowParameters()
	{
	}
	
	NativeWindow::~NativeWindow()
	{
	}
	
	IWindowLibrary::~IWindowLibrary()
	{
	}

	IWindowLibrary* create_window_library()
	{
		if (!detail::windowlibrary)
		{
			detail::windowlibrary = MEMORY_NEW(WindowLibrary, core::memory::global_allocator());
		}
		
		return detail::windowlibrary;
	}
	
	void destroy_window_library()
	{
		MEMORY_DELETE(detail::windowlibrary, core::memory::global_allocator());
		
		shutdown_window_interface();
	}
} // namespace platform
