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
#include "kernel_android.hpp"

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
