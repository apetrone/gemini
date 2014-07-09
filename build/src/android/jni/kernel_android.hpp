// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM,OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
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
