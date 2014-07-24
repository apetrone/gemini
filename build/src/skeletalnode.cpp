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
#include <gemini/typedefs.h>
#include <slim/xlog.h>

#include "debugdraw.h"

#include "scene_graph.h"
#include "skeletalnode.h"
#include "core/assets/asset_mesh.h"

namespace scenegraph
{
	SkeletalNode::SkeletalNode() :
		scale_channel(scale),
		rotation_channel(rotation),
		translation_channel(translation)
	{
	}

	void SkeletalNode::setup_skeleton()
	{
		// This creates a local array of bone data
		// used to store this instance's bone transforms.

		assert(mesh != 0);
		if (mesh->total_bones > 0)
		{
			LOGV("setup skeleton bones\n");
			transforms.allocate(mesh->total_bones);

			for (size_t bone_index = 0; bone_index < mesh->total_bones; ++bone_index)
			{
				// Iterate over each bone and calculate the global transform
				// for each bone.
				assets::Bone* bone = &mesh->bones[bone_index];
				
				if (bone->parent_index == -1)
				{
					bone->world_transform = bone->local_transform;
				}
				else
				{
					bone->world_transform = mesh->bones[bone->parent_index].world_transform * bone->local_transform;
				}

				// prepend the inverse bind pose which places this into bone-space
				glm::mat4& tr = transforms[bone_index];
				tr = bone->inverse_bind_matrix * bone->world_transform;
			}
		}
		else
		{
			if (mesh->animation.scale.keys.size() > 0)
			{
				// set data sources; this could be done better?
				scale_channel.set_data_source(&mesh->animation.scale);
				rotation_channel.set_data_source(&mesh->animation.rotation);
				translation_channel.set_data_source(&mesh->animation.translation);
			}
		}
	}

	void SkeletalNode::update(float delta_seconds)
	{
		assert(mesh != 0);

		for(size_t boneid = 0; boneid < mesh->total_bones; ++boneid)
		{
			assets::Bone* bone = &mesh->bones[boneid];
			debugdraw::axes(bone->bind_matrix, 1.0f);

			debugdraw::sphere(glm::vec3(bone->bind_matrix[3]), Color(255,0,0), 0.25f);
		}
		
//		scale_channel.update(delta_seconds, mesh->animation.frame_delay_seconds);
		rotation_channel.update(delta_seconds, mesh->animation.frame_delay_seconds);
//		translation_channel.update(delta_seconds, mesh->animation.frame_delay_seconds);
		

		glm::mat4 sc = glm::scale(glm::mat4(1.0), scale);
		glm::mat4 ro = glm::toMat4(rotation);
		glm::mat4 tr = glm::translate(glm::mat4(1.0), translation);
		
		local_to_world = /*sc * */ro/* * tr*/;
	}
}; // namespace scenegraph
