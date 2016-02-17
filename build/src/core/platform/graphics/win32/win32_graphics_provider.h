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
#include "graphics_provider.h"

namespace platform
{
	namespace window
	{
		class Win32WindowProvider;

		class Win32GraphicsProvider : public GraphicsProvider
		{
		public:
			Win32GraphicsProvider();

			virtual Result startup(WindowProvider* window_provider) override;
			virtual void shutdown(WindowProvider* window_provider) override;

			virtual void create_context(NativeWindow* window) override;
			virtual void destroy_context(NativeWindow* window) override;

			virtual void attach_context(NativeWindow* window) override;
			virtual void detach_context(NativeWindow* window) override;

			virtual void create_surface(NativeWindow* window) override;
			virtual void destroy_surface(NativeWindow* window) override;

			virtual void swap_buffers(NativeWindow* window) override;
			virtual void* get_symbol(const char* symbol_name) override;
			virtual size_t get_graphics_data_size() const override;

			virtual void pre_window_creation(const Parameters& window_parameters, void* graphics_data) override;
			virtual void* get_native_visual(void* graphics_data) override;

		private:
			Win32WindowProvider* window_provider;
			HGLRC share_context;
		};
	} // namespace window
} // namespace platform
