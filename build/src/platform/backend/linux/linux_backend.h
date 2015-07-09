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
#include "platform_internal.h"

namespace platform
{
	namespace window
	{
		class WindowProvider
		{
		public:
			virtual ~WindowProvider();

			virtual Result startup() = 0;
			virtual void shutdown() = 0;
			virtual platform::window::NativeWindow* create(const platform::window::Parameters& parameters) = 0;
			virtual void destroy(platform::window::NativeWindow* window) = 0;
			virtual platform::window::Frame get_window_rect(platform::window::NativeWindow* window) const = 0;
			virtual platform::window::Frame get_window_render_rect(platform::window::NativeWindow* window) const = 0;
			virtual size_t get_screen_count() const = 0;
			virtual platform::window::Frame get_screen_rect(size_t screen_index) const = 0;
		};

		class GraphicsProvider
		{
		public:
			virtual ~GraphicsProvider();

			virtual Result startup() = 0;
			virtual void shutdown() = 0;
			virtual void create_context(platform::window::NativeWindow* window) = 0;
			virtual void destroy_context(platform::window::NativeWindow* window) = 0;
			virtual void activate_context(platform::window::NativeWindow* window) = 0;
			virtual void swap_buffers(platform::window::NativeWindow* window) = 0;
			virtual void* get_symbol(const char* symbol_name) = 0;

			/// @brief If the graphics provider needs to store custom
			/// data on a per-window basis; return the size in bytes
			/// which needs to be allocated.
			virtual size_t get_graphics_data_size() const = 0;
		};
	} // namespace window

} // namespace platform