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
#include "datamodel/mesh.h"

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

struct ToolOptions
{
	// compress animation keys to reduce output size
	bool compress_animation;

	// flip UVs vertically
	bool flip_vertically;

	// bake transforms from hierarchy into geometry
	bool bake_transforms;
	
	ToolOptions()
	{
		// some sane defaults?
		compress_animation = false;
		flip_vertically = false;
		bake_transforms = true;
	}
};


namespace extensions
{
	void compose_matrix(glm::mat4& out, const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale)
	{
		out = glm::translate(glm::mat4(1.0f), translation) *
		glm::toMat4(rotation) *
		glm::scale(glm::mat4(1.0f), scale);
	}

	void bake_geometry_transform(datamodel::Node* node, glm::mat4& parent_transform)
	{
		LOGV("node: %s\n", node->name.c_str());
		
		// local transform is from this node
		glm::mat4 local_transform;
		compose_matrix(local_transform, node->translation, node->rotation, node->scale);
		
		// reset transforms
		node->translation = glm::vec3(0.0f, 0.0f, 0.0f);
		node->rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		node->scale = glm::vec3(1.0f, 1.0f, 1.0f);
		
		// world transform is the accumulated parent transform of this node
		// multiplied by this node's local transform
		glm::mat4 world_transform = parent_transform * local_transform;
		

		if (node->type == "mesh")
		{
			
			assert(node->mesh != 0);
			if (node->mesh)
			{
				LOGV("applying transform to mesh '%s'\n", node->name.c_str());
				for(size_t v = 0; v < node->mesh->vertices.size(); ++v)
				{
					glm::vec3& vertex = node->mesh->vertices[v];
					vertex = glm::vec3(world_transform * glm::vec4(vertex, 1.0f));
				}
			}
		}
		
		for (auto& child : node->children)
		{
			bake_geometry_transform(child, world_transform);
		}
	}

	core::Result bake_node_transforms(datamodel::Model& model)
	{
		core::Result result(core::Result::Success);
		
		glm::mat4 transform;

		for (auto& child : model.root.children)
		{
			bake_geometry_transform(child, transform);
		}
		
		
		
		return result;
	}
}

namespace tools
{
	// register all datamodel io types
	void register_types()
	{
		Extension<datamodel::Model> ext;
		ext.reader = AutodeskFbxReader::plugin_create();
		register_extension<datamodel::Model>("fbx", ext);
		
		ext.reader = 0;
		ext.writer = JsonModelWriter::plugin_create();
		register_extension<datamodel::Model>("model", ext);
	}

	core::Result convert_model(const ToolOptions& options, StackString<MAX_PATH_SIZE>& input_path, StackString<MAX_PATH_SIZE>& output_path)
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
		
		// TODO: compress animation keys
		// This should eliminate duplicate values by merging them into a single key.
		
		// TODO: add modifier to flip UVs vertically
		
		// bake transforms into geometry nodes
		if (options.bake_transforms)
		{
			extensions::bake_node_transforms(model);
		}
		
		
		
		util::ResizableMemoryStream rs;
		writer->write(&model, rs);

		rs.rewind();
		
		// TODO: ensure the destination path exists?

		xfile_t out = xfile_open(output_path(), XF_WRITE);
		LOGV("writing destination file: %s\n", output_path());
		if (xfile_isopen(out))
		{
			xfile_write(out, rs.get_data(), rs.get_data_size(), 1);
			xfile_close(out);
		}
		else
		{
			LOGE("error writing to file %s\n", output_path());
			LOGE("does the path exist?\n");
		}

		return core::Result(core::Result::Success);
	}
}


int main(int argc, char** argv)
{
	ToolOptions options;

	tools::startup();
	
	register_types();
	
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
	input_filename.strip_trailing(PATH_SEPARATOR).append(PATH_SEPARATOR_STRING);
	input_filename.append(input_file);
	
	StackString<MAX_PATH_SIZE> output_filename = output_root;
	output_filename.normalize();
	output_filename.strip_trailing(PATH_SEPARATOR).append(PATH_SEPARATOR_STRING);
	output_filename = output_filename.append(input_file).remove_extension().append(".model");

	// perform the conversion -- right now, we always assume it's a scene/model
	core::Result result = tools::convert_model(options, input_filename, output_filename);
	if (result.failed())
	{
		LOGV("conversion failed: %s\n", result.message);
	}

	double duration = (xtime_msec(&timer) - start);
	LOGV("processed in %2.2f seconds\n", duration*.001f);
	
	tools::shutdown();
	return 0;
}