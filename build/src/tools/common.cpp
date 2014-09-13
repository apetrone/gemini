// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone

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
#include <gemini/typedefs.h>
#include <gemini/mem.h>
#include <gemini/core.h>

#include "common.h"

// include reader/writers
#include "io_fbx.h"
#include "io_json.h"

namespace tools
{
	// register all datamodel io types
	void register_types()
	{
		Extension<datamodel::SceneNode> ext;
		ext.reader = AutodeskFbxReader::plugin_create();
		register_extension<datamodel::SceneNode>("fbx", ext);
		
		ext.reader = 0;
		ext.writer = JsonSceneWriter::plugin_create();
		register_extension<datamodel::SceneNode>("model", ext);
	}

	void startup()
	{
		memory::startup();
		core::startup();
		
		register_types();
	}
	
	void shutdown()
	{
		purge_registry<datamodel::SceneNode>();
		
		core::shutdown();
		memory::shutdown();
	}
	
	
	ApplicationData& data()
	{
		static ApplicationData _data;
		return _data;
	}
}
