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
#include "common.h"
#include "common/extension.h"
#include "datamodel/model.h"
#include "datamodel/mesh.h"

// include reader/writers
#include "io_json.h"

#include <core/logging.h>

#include <platform/platform.h>

#include <core/stackstring.h>
#include <core/datastream.h>
#include <core/argumentparser.h>

#include <string>
#include <stdio.h>
#include <stdlib.h>

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

		// export the animation only
		bool animation_only;

		ToolOptions()
		{
			// some sane defaults?
			compress_animation = false;
			flip_vertically = false;

			animation_only = false;
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
//			LOGV("node: %s\n", node->name.c_str());

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
//					LOGV("applying transform to mesh '%s'\n", node->name.c_str());
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

		void calculate_geometry_boundingbox(datamodel::Node* node)
		{
			if (node->type == "mesh")
			{
				assert(node->mesh);
				if (node->mesh)
				{
					glm::vec3 mins(FLT_MAX, FLT_MAX, FLT_MAX);
					glm::vec3 maxs(-FLT_MAX, -FLT_MAX, -FLT_MAX);
					for (size_t v = 0; v < node->mesh->vertices.size(); ++v)
					{
						const glm::vec3& vertex = node->mesh->vertices[v];
						mins.x = glm::min(mins.x, vertex.x);
						mins.y = glm::min(mins.y, vertex.y);
						mins.z = glm::min(mins.z, vertex.z);

						maxs.x = glm::max(maxs.x, vertex.x);
						maxs.y = glm::max(maxs.y, vertex.y);
						maxs.z = glm::max(maxs.z, vertex.z);
					}

					node->mesh->mins = mins;
					node->mesh->maxs = maxs;
				}
			}
		}

		void calculate_geometry_bounds(datamodel::Model& model)
		{
			for (auto& child : model.root.children)
			{
				calculate_geometry_boundingbox(child);
			}
		}


		void verify_geometry_recursive(datamodel::Node* node, datamodel::Model& model)
		{
			if (node->type == "mesh")
			{
				// must have vertices
				assert(!node->mesh->vertices.empty());

				// must have indices
				assert(!node->mesh->indices.empty());

				if (model.skeleton)
				{
					// animated models require:
					// normals and uv0
					assert(node->mesh);

					// animated models require normals
					if (node->mesh->normals.empty())
					{
						LOGW("animated mesh '%s' has no normals! populating with dummy values...\n",
							 node->mesh->name.c_str());

						node->mesh->normals.allocate(node->mesh->vertices.size());
						for (size_t index = 0; index < node->mesh->normals.size(); ++index)
						{
							glm::vec3& normal = node->mesh->normals[index];
							normal = glm::vec3(0.0f, 0.0f, 1.0f);
						}
					}

					// animated models require one set of UVs
					if (node->mesh->uvs.empty())
					{
						LOGW("animated mesh '%s' has no uvs! populating with dummy values...\n",
							 node->mesh->name.c_str());

						// just need one set for now
						node->mesh->uvs.allocate(1);
						node->mesh->uvs[0].allocate(node->mesh->vertices.size());
						FixedArray<glm::vec2>& uvs = node->mesh->uvs[0];
						for (size_t index = 0; index < uvs.size(); ++index)
						{
							glm::vec2& uv = uvs[index];
							uv = glm::vec2(0.0f, 0.0f);
						}
					}
				}
			}
		}

		// This ensures that the data model has the correct collection of
		// attributes required by the runtime.
		void verify_geometry(datamodel::Model& model)
		{
			for (datamodel::Node* child : model.root.children)
			{
				verify_geometry_recursive(child, model);
			}
		}
	} // namespace extensions

	namespace tools
	{
		// register all datamodel io types
		void register_types()
		{
			Extension<datamodel::Model> ext;

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
				return platform::Result::failure("Unable to read format");
			}

			// verify we can write the format
			ext = output_path.extension();
			const Extension<datamodel::Model> writer_extension = find_entry_for_extension<datamodel::Model>("model");
			tools::Writer<datamodel::Model>* writer = writer_extension.writer;
			if (!writer)
			{
				LOGE("no writer found for extension: %s\n", ext.c_str());
				return platform::Result::failure("Unable to write format");
			}

			util::MemoryStream mds;
			mds.data = (uint8_t*)input_path();
			if (!reader->read(&model, mds))
			{
				LOGV("Error reading file '%s'\n", input_path());
				return platform::Result::failure("Error reading input file");
			}

			extensions::verify_geometry(model);

			// TODO: compress animation keys
			// This should eliminate duplicate values by merging them into a single key.

			// TODO: add modifier to flip UVs vertically

			// bake transforms into geometry nodes (only for static meshes)
			if (!model.skeleton)
			{
				extensions::bake_node_transforms(model);
				extensions::calculate_geometry_bounds(model);
			}

			if (options.animation_only)
			{
				model.remove_export_flag(datamodel::Model::EXPORT_MATERIALS);
				model.remove_export_flag(datamodel::Model::EXPORT_MESHES);
				model.remove_export_flag(datamodel::Model::EXPORT_SKELETON);
			}

			// calculate mass_center_offsets for all meshes
			for (auto& child : model.root.children)
			{
				if (child->type == "mesh" && child->mesh)
				{
					child->mesh->mass_center_offset = child->translation - geometry::compute_center_of_mass(child->mesh);
				}
			}

			writer->write(output_path(), &model);

			return platform::Result::success();
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
	--animation-only  Export only the animation from <input_path>
	)";


	core::argparse::ArgumentParser parser;
	core::argparse::VariableMap vm;

	std::vector<std::string> arguments = parser.split_tokens(argc, argv);

	if (!parser.parse(docstring, arguments, vm, "1.0.0-alpha"))
	{
		return -1;
	}

	const std::string asset_root = vm["source_asset_root"];
	const std::string input_path = vm["input_path"];
	const std::string output_root = vm["output_asset_root"];

	ToolOptions options;
	if (vm.find("--animation-only") != vm.end())
	{
		const std::string animation_only = vm["--animation-only"];

		options.animation_only = animation_only == "true";
	}


	tools::startup("arcfusion.net/lynx");

	tools::register_types();


	uint64_t start_ticks = platform::microseconds();

	// determine our input and output filenames
	StackString<MAX_PATH_SIZE> input_filename = asset_root.c_str();
	input_filename.normalize(PATH_SEPARATOR);
	input_filename.strip_trailing(PATH_SEPARATOR).append(PATH_SEPARATOR_STRING);
	input_filename.append(input_path.c_str());

	StackString<MAX_PATH_SIZE> output_filename = output_root.c_str();
	output_filename.normalize(PATH_SEPARATOR);
	output_filename.strip_trailing(PATH_SEPARATOR).append(PATH_SEPARATOR_STRING);
	output_filename = output_filename.append(input_path.c_str()).remove_extension();

	// perform the conversion -- right now, we always assume it's a scene/model
	platform::Result result = tools::convert_model(options, input_filename, output_filename);
	if (result.failed())
	{
		LOGV("conversion failed: %s\n", result.message);
	}

	float duration = (platform::microseconds() - start_ticks);
	LOGV("processed in %2.2f milliseconds\n", duration*.001f);

	tools::shutdown();
	return 0;
}
