// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone
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

#include <string>
#include <stdio.h>
#include <stdlib.h>

#include <platform/platform.h>

#include <core/xfile.h>
#include <core/stackstring.h>
#include <core/datastream.h>
#include <core/logging.h>
#include <core/argumentparser.h>

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


// https://docs.unrealengine.com/latest/INT/Engine/Content/FBX/index.html


/*
Static Meshes:
- The pivot point is always at the origin (0, 0, 0) of your exported mesh.

[references]
	* https://docs.unrealengine.com/latest/INT/Engine/Content/FBX/StaticMeshes/index.html


Skeletal Meshes:
- The pivot point is always at the root joint. It doesn't matter where the root joint is located.
  It will be treated as the root joint is the origin.

[references]
	* https://docs.unrealengine.com/latest/INT/Engine/Content/FBX/SkeletalMeshes/index.html

*/


using namespace core;

namespace gemini
{
	struct ToolOptions
	{
		// compress animation keys to reduce output size
		bool compress_animation;

		// flip UVs vertically
		bool flip_vertically;

		// bake transforms from hierarchy into geometry
		bool bake_transforms;
		
		// export the animation only
		bool animation_only;
		
		ToolOptions()
		{
			// some sane defaults?
			compress_animation = false;
			flip_vertically = false;
			
			// this should be true for static meshes
			// and false for skeletal meshes.
			bake_transforms = false;
			
			animation_only = true;
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
//			node->rotation = glm::vec3(0.0f, 0.0f, 0.0f);
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

		int bake_node_transforms(datamodel::Model& model)
		{
			int result = 0;
			
			glm::mat4 transform;

			for (auto& child : model.root.children)
			{
				bake_geometry_transform(child, transform);
			}
			
			
			
			return result;
		}
	} // namespace extensions

	namespace tools
	{
		// register all datamodel io types
		void register_types()
		{
			Extension<datamodel::Model> ext;
			ext.reader = AutodeskFbxReader::plugin_create();
			register_extension<datamodel::Model>("fbx", ext);
			
//			ext.reader = OpenGEXReader::plugin_create();
//			register_extension<datamodel::Model>("ogex", ext);
			
			ext.reader = 0;
			ext.writer = JsonModelWriter::plugin_create();
			register_extension<datamodel::Model>("model", ext);
		}

		platform::Result convert_model(const ToolOptions& options, StackString<MAX_PATH_SIZE>& input_path, StackString<MAX_PATH_SIZE>& output_path)
		{
			datamodel::Model model;

			// verify we can read the format
			std::string ext = input_path.extension();
			const Extension<datamodel::Model> archiver_extension = find_entry_for_extension<datamodel::Model>(ext);
			tools::Reader<datamodel::Model>* reader = archiver_extension.reader;
			if (!reader)
			{
				LOGE("no reader found for extension: %s\n", ext.c_str());
				return platform::Result(platform::Result::Failure, "Unable to read format");
			}
					
			// verify we can write the format
			ext = output_path.extension();
			const Extension<datamodel::Model> writer_extension = find_entry_for_extension<datamodel::Model>(ext);
			tools::Writer<datamodel::Model>* writer = writer_extension.writer;
			if (!writer)
			{
				LOGE("no writer found for extension: %s\n", ext.c_str());
				return platform::Result(platform::Result::Failure, "Unable to write format");
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

			// calculate mass_center_offsets for all meshes
			for (auto& child : model.root.children)
			{
				if (child->type == "mesh" && child->mesh)
				{
					child->mesh->mass_center_offset = child->translation - geometry::compute_center_of_mass(child->mesh);
				}
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

			return platform::Result(platform::Result::Success);
		}
	} // namespace tools
} // namespace gemini




int main(int argc, char** argv)
{
	using namespace gemini;
	using namespace core::argparse;

	
	const char* docstring = R"(
Usage:
	[--animation-only] <source_asset_root> <input_path> <output_asset_root>
	
Options:
	-h, --help  Show this help screen
	--version  Display the version number
	)";
	
	
	core::argparse::ArgumentParser parser;
	core::argparse::VariableMap vm;
	
	if (!parser.parse(docstring, argc, argv, vm, "alpha 1.0.0"))
	{
		return -1;
	}
	
	const std::string asset_root = vm["source_asset_root"];
	const std::string input_path = vm["input_path"];
	const std::string output_root = vm["output_asset_root"];
	
	
	
	tools::startup();
	
	tools::register_types();
	
	ToolOptions options;
	uint64_t start_ticks = platform::instance()->get_time_microseconds();

	// determine our input and output filenames
	StackString<MAX_PATH_SIZE> input_filename = asset_root.c_str();
	input_filename.normalize();
	input_filename.strip_trailing(PATH_SEPARATOR).append(PATH_SEPARATOR_STRING);
	input_filename.append(input_path.c_str());
	
	StackString<MAX_PATH_SIZE> output_filename = output_root.c_str();
	output_filename.normalize();
	output_filename.strip_trailing(PATH_SEPARATOR).append(PATH_SEPARATOR_STRING);
	output_filename = output_filename.append(input_path.c_str()).remove_extension().append(".model");

	// perform the conversion -- right now, we always assume it's a scene/model
	platform::Result result = tools::convert_model(options, input_filename, output_filename);
	if (result.failed())
	{
		LOGV("conversion failed: %s\n", result.message);
	}

	float duration = (platform::instance()->get_time_microseconds() - start_ticks);
	LOGV("processed in %2.2f milliseconds\n", duration*.001f);
	
	tools::shutdown();
	return 0;
}