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
#include "typedefs.h"
#include "log.h"
#include "renderer.hpp"
#include "factory.hpp"


// compile-time selection of these classes starts here.

#if RASPBERRYPI || (PLATFORM_IOS && PLATFORM_IS_MOBILE)
	// force use of OpenGL ES v2
	#include <gldrivers/opengl_glesv2.hpp>
#else
	// use OpenGL
//	#include <gldrivers/opengl_legacy21.hpp>
	#include <gldrivers/opengl_core32.hpp>
#endif

namespace renderer
{
	IRenderDriver * _render_driver = 0;
	
	IRenderDriver * driver() { return _render_driver; }
	
	int startup( DriverType driver_type )
	{
		typedef Factory<IRenderDriver, 4> RendererFactory;
		RendererFactory factory;

		// run-time selection of the renderer has to happen here based on hints by the kernel
		// for now, we just hard-code these
#if PLATFORM_IS_MOBILE
		factory.register_class( GLESv2::creator, "OpenGL ES 2.0", GLESv2 );
		driver_type = GLESv2;
#else
		factory.register_class( &GLCore32::creator, "OpenGL3.2", OpenGL );
		driver_type = OpenGL;
#endif
		
		// choose the correct driver at run time; based on some hints?
		RendererFactory::Record * record = factory.find_class( 0, driver_type );
		if ( record )
		{
			_render_driver = record->creator();
			if ( _render_driver )
			{
				LOGV( "Initialized renderer: '%s'\n", _render_driver->description() );
				return 1;
			}
		}
		else
		{
			LOGE( "Unable to find a renderer matching driver type: %i\n", driver_type );
		}

		return 0;
	} // startup
	
	void shutdown()
	{
		if ( _render_driver )
		{
			DEALLOC(IRenderDriver, _render_driver);
		}
	} // shutdown
	
}; // namespace renderer