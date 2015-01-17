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
#pragma once

#include "kernel.h"

class AndroidKernel : public kernel::IKernel
{
public:
	bool active;
	kernel::Params params;
	bool is_kernel_initialized;
	int status_bar_height;
	float display_density;

public:
	virtual ~AndroidKernel() {}
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

	void on_surface_changed( int width, int height );
	bool is_initialized() const { return is_kernel_initialized; }
	void set_status_bar_height( int height );
	void set_display_density( float density );
}; // AndroidKernel
