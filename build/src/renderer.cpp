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



// compile-time selection of these classes starts here.

#if RASPBERRYPI || (PLATFORM_IOS && PLATFORM_IS_MOBILE)
	// force use of OpenGL ES v2
//	#include <gldrivers/glesv2.hpp>
#else
	// use OpenGL
//	#include <gldrivers/opengl_legacy21.hpp>
	#include <gldrivers/opengl_core32.hpp>
#endif

namespace renderer
{
	namespace _internal
	{
		RenderDriverCreator _creator_list = 0;
		
		void register_driver( DriverType type, RenderDriverCreator creator )
		{
			// I'm going to ignore most of the parameters to this function for now.
			// however, once the need arises to use more than one driver, I'll code this up.
			_creator_list = creator;
		} // register_driver
			
		RenderDriverCreator find_driver( DriverType type )
		{
			return _creator_list;
		} // find_driver
	}; // _internal

	IRenderDriver * _render_driver = 0;
	
	int startup( DriverType driver_type )
	{
		// run-time selection of the renderer has to happen here based on hints by the kernel
		_internal::register_driver( OpenGL, GLCore32::creator );
		
		// choose the correct driver at run time; based on some hints?
		RenderDriverCreator creator = _internal::find_driver( driver_type );
		if ( creator )
		{
			_render_driver = creator();
			if ( _render_driver )
			{
				LOGV( "Initialized renderer: '%s'\n", _render_driver->description() );
				return 1;
			}
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