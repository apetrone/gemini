// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//      * Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.

//      * Redistributions in binary form must reproduce the above copyright notice,
//      this list of conditions and the following disclaimer in the documentation
//      and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//       SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------
#pragma once

#include "input.h"
#include "kernel.h"
#include <map>

namespace gemini
{
	struct DesktopKernelState;

	class DesktopKernel : public virtual kernel::IKernel
	{
		struct DesktopParams : public kernel::Params
		{
			int argc;
			char ** argv;
			
			bool has_window;
		};
		
		bool active;
		DesktopParams params;
		
		int target_renderer;

		DesktopKernelState* state;

	public:
		DesktopKernel( int argc, char ** argv );

		virtual bool is_active() const { return active; }
		virtual void set_active( bool isactive ) { active = isactive; }
		virtual kernel::Params & parameters() { return params; }
		
		virtual void startup();
		virtual void register_services();
		virtual void pre_tick();
		virtual void post_tick();
		virtual void post_application_config( kernel::ApplicationResult result );
		virtual void post_application_startup( kernel::ApplicationResult result );
		virtual void shutdown();
		
		virtual void capture_mouse(bool capture);
		virtual void warp_mouse(int x, int y);
		virtual void get_mouse_position(int& x, int& y);
		virtual void show_mouse(bool show);
	private:
		struct xwl_window_s *create_window( struct xwl_windowparams_s * windowparams, const char * title, unsigned int * attribs );
	};

	namespace kernel
	{
		// main loop for a desktop app; this manages the main loop itself.
		// it's enough in a desktop application to simply hand off control to this function.
		Error main( IKernel * kernel_instance, const char * application_name );
	} // namespace kernel
} // namespace gemini