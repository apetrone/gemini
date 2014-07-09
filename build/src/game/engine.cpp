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
#include <slim/xlog.h>
#include "engine.h"

namespace engine
{
	class Engine : public IEngineInterface
	{
		//ScreenController * s_control;
	public:
		
		Engine()
		{
			LOGV( "Engine starting up\n" );
			//s_control = CREATE(ScreenController);
		}
		
		~Engine()
		{
			LOGV( "Engine shutting down\n" );
			//DESTROY(ScreenController, s_control);
		}


#if 0
		virtual ScreenController * screen_controller()
		{
			return s_control;
		}
#endif
	}; // Engine
	
	static Engine * _engine = 0;
	
	void startup()
	{
		_engine = CREATE(Engine);
	}
	
	void shutdown()
	{
		DESTROY(Engine, _engine);
	}
	
	IEngineInterface * engine()
	{
		return _engine;
	}
}; // namespace engine