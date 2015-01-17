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
#include "kernel_android.h"

void AndroidKernel::startup()
{
	is_kernel_initialized = true;
}

void AndroidKernel::register_services()
{

}

void AndroidKernel::pre_tick()
{

}

void AndroidKernel::post_tick()
{

}

void AndroidKernel::post_application_config( kernel::ApplicationResult result )
{
	this->parameters().device_flags |= kernel::DeviceAndroid;
}

void AndroidKernel::post_application_startup( kernel::ApplicationResult result )
{

}

void AndroidKernel::shutdown()
{

}

void AndroidKernel::on_surface_changed(int width, int height)
{
	this->parameters().window_width = this->parameters().render_width = (width + status_bar_height);
	this->parameters().window_height = this->parameters().render_height = height;
}

void AndroidKernel::set_status_bar_height( int height )
{
	this->status_bar_height = height;
}

void AndroidKernel::set_display_density( float density )
{
	this->display_density = density;
}
