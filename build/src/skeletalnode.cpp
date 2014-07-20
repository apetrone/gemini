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

#include "scene_graph.h"
#include "skeletalnode.h"
#include "core/assets/asset_mesh.h"

namespace scenegraph
{

	void SkeletalNode::setup_skeleton()
	{
		// This creates a local array of bone data
		// used to store this instance's bone transforms.

		assert(mesh != 0);
		transforms.allocate(mesh->total_bones);

		for (size_t bone_index = 0; bone_index < mesh->bones.size(); ++bone_index)
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

			// apply the inverse bind pose
			

			glm::mat4& tr = transforms[bone_index];
			tr = bone->inverse_bind_pose * bone->world_transform;	
		}
		
	}

	void SkeletalNode::update(float delta_seconds)
	{
		assert(mesh != 0);

		// iterate over the source animation skeleton
		// and apply transforms?
	}
}; // namespace scenegraph
