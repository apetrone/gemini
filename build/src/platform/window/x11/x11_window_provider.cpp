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
#include "platform_internal.h"
#include "linux_backend.h"
#include "x11_window_provider.h"


namespace platform
{
	namespace window
	{
		struct X11Window : public NativeWindow
		{
			virtual void* get_native_handle() const override
			{
				return nullptr;
			}
		}; // struct X11Window


		X11WindowProvider::X11WindowProvider()
		{
		}

		X11WindowProvider::~X11WindowProvider()
		{
		}

		Result X11WindowProvider::startup()
		{
			return Result(Result::Success);
		}

		void X11WindowProvider::shutdown()
		{
		}

		NativeWindow* X11WindowProvider::create(const Parameters& parameters)
		{
			return nullptr;
		}

		void X11WindowProvider::destroy(NativeWindow* window)
		{
		}

		Frame X11WindowProvider::get_frame(NativeWindow* window) const
		{
			Frame frame;
			return frame;
		}

		Frame X11WindowProvider::get_render_frame(NativeWindow* window) const
		{
			Frame frame;
			return frame;
		}

		size_t X11WindowProvider::get_screen_count() const
		{
			return 0;
		}

		Frame X11WindowProvider::get_screen_frame(size_t screen_index) const
		{
			Frame frame;
			return frame;
		}		
	} // namespace window
} // namespace platform
