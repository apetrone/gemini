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

//#include <gemini/core/filesystem.h>
//#include <gemini/core/log.h>
#include <gemini/core/xfile.h>
#include <gemini/util/stackstring.h>

//#include <gemini/util/arg.h>
#include <gemini/util/datastream.h>

#include <slim/xtime.h>
#include <slim/xlog.h>

#include "common.h"
#include "common/extension.h"
#include "datamodel/model.h"


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
	core::Result convert_model(StackString<MAX_PATH_SIZE>& input_path, StackString<MAX_PATH_SIZE>& output_path)
	{
		datamodel::Model model;

		// verify we can read the format
		std::string ext = input_path.extension();
		const Extension<datamodel::Model> archiver_extension = find_entry_for_extension<datamodel::Model>(ext);
		tools::Reader<datamodel::Model>* reader = archiver_extension.reader;
		if (!reader)
		{
			LOGE("no reader found for extension: %s\n", ext.c_str());
			return core::Result(core::Result::Failure, "Unable to read format");
		}
		
		// verify we can write the format
		ext = output_path.extension();
		const Extension<datamodel::Model> writer_extension = find_entry_for_extension<datamodel::Model>(ext);
		tools::Writer<datamodel::Model>* writer = writer_extension.writer;
		if (!writer)
		{
			LOGE("no writer found for extension: %s\n", ext.c_str());
			return core::Result(core::Result::Failure, "Unable to write format");
		}
		
		util::MemoryStream mds;
		mds.data = (uint8_t*)input_path();
		reader->read(&model, mds);
		
		util::ResizableMemoryStream rs;
		writer->write(&model, rs);

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


int main(int argc, char** argv)
{
	tools::startup();
	
	// Commented out until I fix some issues with it.
//	args::Argument* asset_root = args::add("asset_root","-d", "--asset-root", 0, 0);
//	args::Argument* input_file = args::add("input_file", "-f", "--input", 0, 0);
//	args::Argument* output_root = args::add("output_root", "-o", "--output-root", 0, 0);
	
	if (argc < 4)
	{
		LOGE("Argument parsing failed. Usage: \"/path/to/assets\" \"models/plane.fbx\" \"/path/to/build/latest\"\n");
		return -1;
	}
	
	const char* asset_root = argv[1];
	const char* input_file = argv[2];
	const char* output_root = argv[3];

	xtime_t timer;
	xtime_startup(&timer);
	double start = xtime_msec(&timer);

	// determine our input and output filenames
	StackString<MAX_PATH_SIZE> input_filename = asset_root;
	input_filename.normalize();
	input_filename.strip_trailing('/').append("/");
	input_filename.append(input_file);
	
	StackString<MAX_PATH_SIZE> output_filename = output_root;
	output_filename.normalize();
	output_filename.strip_trailing('/').append("/");
	output_filename = output_filename.append(input_file).remove_extension().append(".model");

	// perform the conversion -- right now, we always assume it's a scene/model
	core::Result result = tools::convert_model(input_filename, output_filename);
	if (result.failed())
	{
		LOGV("conversion failed: %s\n", result.message);
	}

	double duration = (xtime_msec(&timer) - start);
	LOGV("processed in %2.2f seconds\n", duration*.001f);
	
	tools::shutdown();
	return 0;
}