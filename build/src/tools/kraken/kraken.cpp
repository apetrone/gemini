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

#include <platform/platform.h>
#include <platform/kernel.h>
#include <platform/windowlibrary.h>
#include <platform/input.h>

#include <core/core.h>
#include <runtime/logging.h>



#include <nom/nom.hpp>
#include <nom/compositor.hpp>
#include <nom/graph.hpp>
#include <nom/button.hpp>


class EditorKernel : public kernel::IKernel,
public kernel::IEventListener<kernel::KeyboardEvent>
{
private:
	bool active;
	platform::IWindowLibrary* window_interface;
	
public:
	EditorKernel() :
		active(true),
		window_interface(0)
	{
	}
	
	virtual ~EditorKernel() {}
	
	virtual bool is_active() const { return active; }
	virtual void set_active(bool isactive) { active = isactive; }
	
	virtual void resolution_changed(int width, int height) {}

	virtual kernel::Error startup()
	{
		core::startup();
		window_interface = platform::create_window_library();
		window_interface->startup(kernel::parameters());
		
		kernel::Parameters& params = kernel::parameters();
		params.window_width = 1280;
		params.window_height = 720;
		params.window_title = "kraken";
		
		window_interface->create_window(kernel::parameters());
		
		return kernel::NoError;
	}
	
	virtual void tick()
	{
		window_interface->process_events();
		
		window_interface->swap_buffers();
	}
	
	virtual void shutdown()
	{
		window_interface->shutdown();
		platform::destroy_window_library();
		core::shutdown();
	}
	
	
	virtual void event(kernel::KeyboardEvent& event)
	{
		if (event.key == input::KEY_ESCAPE && event.is_down)
		{
			set_active(false);
		}
	}
	
};



PLATFORM_MAIN
{
	int return_code;
	return_code = platform::run_application(new EditorKernel());
	return return_code;
}