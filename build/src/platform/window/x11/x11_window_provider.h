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
#pragma once

#include <core/config.h>
#include "window_provider.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

namespace platform
{
	namespace window
	{
		struct X11Window : public NativeWindow
		{
			X11Window() :
				native_window(0),
				visual(nullptr)
			{
			}

			virtual void* get_native_handle() override
			{
				return &native_window;
			}

			virtual void update_visual(int visual_id) override
			{
				// visual = visual_id;
			}

			Window native_window;
			Visual* visual;
		}; // struct X11Window

		class X11WindowProvider : public WindowProvider
		{
		public:
			X11WindowProvider();
			virtual ~X11WindowProvider() override;

			virtual Result startup() override;
			virtual void shutdown() override;
			virtual NativeWindow* create(const Parameters& parameters, void* native_visual) override;
			virtual void destroy(NativeWindow* window) override;
			virtual Frame get_frame(NativeWindow* window) const override;
			virtual Frame get_render_frame(NativeWindow* window) const override;
			virtual size_t get_screen_count() const override;
			virtual Frame get_screen_frame(size_t screen_index) const override;
			virtual void dispatch_events() override;

			Display* get_display() const;

		private:
			void process_event(XEvent& event);

			Display* display;
			XEvent last_key_release;
			Atom atom_delete_window;
		}; // class X11WindowProvider
	} // namespace window
} // namespace platform
