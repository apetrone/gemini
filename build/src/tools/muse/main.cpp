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

#include <string>

#include <stdio.h>
#include <stdlib.h>

#include <gemini/mem.h>
#include <gemini/core.h>
#include <gemini/core/filesystem.h>
#include <gemini/core/log.h>
#include <gemini/core/xfile.h>
#include <gemini/util/stackstring.h>

#include <gemini/util/arg.h>
#include <gemini/util/datastream.h>

#include <gemini/mathlib.h>

#include <slim/xtime.h>

#include <slim/xlog.h>

#include "common.h"
#include "datamodel.h"
#include "common/extension.h"

// include reader/writers
#include "io_fbx.h"
#include "io_json.h"

// need to support:
// static (non-animated) meshes
// hierarchical/skeletal meshes
// morph targets

// skeleton (bones in a hierarchical tree)
// mesh/geometry (vertices, normals, uvs[2], vertex-colors)


//https://docs.unrealengine.com/latest/INT/Engine/Content/FBX/index.html

using namespace tools;


namespace tools
{
	core::Result convert_scene(StackString<MAX_PATH_SIZE>& input_path, StackString<MAX_PATH_SIZE>& output_path)
	{
		datamodel::SceneNode root;

		// verify we can read the format
		std::string ext = input_path.extension();
		const Extension<datamodel::SceneNode> archiver_extension = find_entry_for_extension<datamodel::SceneNode>(ext);
		tools::Reader<datamodel::SceneNode>* reader = archiver_extension.reader;
		if (!reader)
		{
			LOGE("no reader found for extension: %s\n", ext.c_str());
			return core::Result(core::Result::Failure, "Unable to read format");
		}
		
		// verify we can write the format
		ext = output_path.extension();
		const Extension<datamodel::SceneNode> writer_extension = find_entry_for_extension<datamodel::SceneNode>(ext);
		tools::Writer<datamodel::SceneNode>* writer = writer_extension.writer;
		if (!writer)
		{
			LOGE("no writer found for extension: %s\n", ext.c_str());
			return core::Result(core::Result::Failure, "Unable to write format");
		}
		
		util::MemoryStream mds;
		mds.data = (uint8_t*)input_path();
		reader->read(&root, mds);
		
		util::ResizableMemoryStream rs;
		writer->write(&root, rs);

		rs.rewind();
		
		xfile_t out = xfile_open(output_path(), XF_WRITE);
		if (xfile_isopen(out))
		{
			xfile_write(out, rs.get_data(), rs.get_data_size(), 1);
			xfile_close(out);
		}
		return core::Result(core::Result::Success);
	}
}

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

int main(int argc, char** argv)
{
	memory::startup();
	core::startup();
	
	args::Argument* asset_root = args::add("asset_root","-d", "--asset-root", 0, 0);
	args::Argument* input_file = args::add("input_file", "-f", "--input", 0, 0);
	args::Argument* output_root = args::add("output_root", "-o", "--output-root", 0, 0);
		
	if (!args::parse_args(argc, argv))
	{
		LOGE("Argument parsing failed. Usage: -d \"/path/to/assets\" -f \"models/plane.fbx\" -o \"/path/to/build/latest\"\n");
		return -1;
	}
	
	register_types();

	xtime_t timer;
	xtime_startup(&timer);
	double start = xtime_msec(&timer);

	// determine our input and output filenames
	StackString<MAX_PATH_SIZE> input_filename = asset_root->string;
	input_filename.normalize();
	input_filename.strip_trailing('/').append("/");
	input_filename.append(input_file->string);
	
	StackString<MAX_PATH_SIZE> output_filename = output_root->string;
	output_filename.normalize();
	output_filename.strip_trailing('/').append("/");
	output_filename = output_filename.append(input_file->string).remove_extension().append(".model");

	// perform the conversion -- right now, we always assume it's a scene/model
	core::Result result = tools::convert_scene(input_filename, output_filename);
	if (result.failed())
	{
		LOGV("conversion failed: %s\n", result.message);
	}

	double duration = (xtime_msec(&timer) - start);
	LOGV("processed in %2.2f seconds\n", duration*.001f);
	
	purge_registry<datamodel::SceneNode>();

	core::shutdown();
	memory::shutdown();
	return 0;
}