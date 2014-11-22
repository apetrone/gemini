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
	AnimatedNode::AnimatedNode() :
		scale_channel(scale),
		rotation_channel(rotation),
		translation_channel(translation)
	{
		type = ANIMATED;
		attributes = ANIMATED;
	}
	
	AnimatedNode::AnimatedNode(AnimatedNode& other) :
		scale_channel(scale),
		rotation_channel(rotation),
		translation_channel(translation),
		Node(other)
	{
		if (other.scale_channel.get_data_source())
		{
			scale_channel.set_data_source(other.scale_channel.get_data_source(), other.scale_channel.get_frame_delay());
		}
		
		if (other.rotation_channel.get_data_source())
		{
			rotation_channel.set_data_source(other.rotation_channel.get_data_source(), other.rotation_channel.get_frame_delay());
		}
		
		if (other.translation_channel.get_data_source())
		{
			translation_channel.set_data_source(other.translation_channel.get_data_source(), other.translation_channel.get_frame_delay());
		}
	}

	void AnimatedNode::post_processing(assets::Mesh* mesh, int32_t node_index)
	{
		if (!mesh || (mesh->animation.scale.empty() && mesh->animation.rotation.empty() && mesh->animation.translation.empty()))
		{
			return;
		}

		// set data sources; this could be done better?
		scale_channel.set_data_source(&mesh->animation.scale[node_index], mesh->animation.frame_delay_seconds);
		rotation_channel.set_data_source(&mesh->animation.rotation[node_index], mesh->animation.frame_delay_seconds);
		translation_channel.set_data_source(&mesh->animation.translation[node_index], mesh->animation.frame_delay_seconds);
	}
	
	void AnimatedNode::update(float delta_seconds)
	{
		if (scale_channel.get_data_source() && !scale_channel.get_data_source()->keys.empty())
		{
			scale_channel.update(delta_seconds);
			rotation_channel.update(delta_seconds);
			translation_channel.update(delta_seconds);
		}
		else
		{
			scale = local_scale;
			translation = local_position;
			rotation = local_rotation;
		}
	
		Node::update(delta_seconds);
	}
	
	RenderNode::RenderNode() : visible(true)
	{
		type = RENDER;
		attributes = ANIMATED;
	}
	
	SkeletalNode::SkeletalNode()
	{
		type = scenegraph::SKELETON;
		attributes = ANIMATED | SKELETON;
	}

	void SkeletalNode::setup_skeleton()
	{
		// This creates a local array of bone data
		// used to store this instance's bone transforms.

		assert(mesh != 0);
		if (mesh->total_bones > 0)
		{
			final_transforms.allocate(mesh->total_bones);
			transforms.allocate(mesh->total_bones);

			for (size_t bone_index = 0; bone_index < mesh->total_bones; ++bone_index)
			{
				// Iterate over each bone and calculate the global transform
				// for each bone.
				assets::Bone* bone = &mesh->bones[bone_index];
				AnimatedNode* node = CREATE(AnimatedNode);
				node->post_processing(mesh, bone_index);
				add_child(node);
				node->name = bone->name.c_str();
				//node->local_to_world = bone->local_transform;
			}
		}
	}

	void SkeletalNode::update(float delta_seconds)
	{
		RenderNode::update(delta_seconds);
		if (this->visible)
		{
			update_skeleton();
		}
	}

	static void print_mat4(glm::mat4& m)
	{
		glm::vec4& a = m[0];
		glm::vec4& b = m[1];
		glm::vec4& c = m[2];
		glm::vec4& d = m[3];
		LOGV("MAT\n"
			"[%2.2g, %2.2g, %2.2g, %2.2g]\n"
			"[%2.2g, %2.2g, %2.2g, %2.2g]\n"
			"[%2.2g, %2.2g, %2.2g, %2.2g]\n"
			"[%2.2g, %2.2g, %2.2g, %2.2g]\n\n",
			 a.x, a.y, a.z, a.w,
			 b.x, b.y, b.z, b.w,
			 c.x, c.y, c.z, c.w,
			 d.x, d.y, d.z, d.w);
	}

	// ref: http://ehc.ac/p/assimp/discussion/817654/thread/a7bf155b/
	void SkeletalNode::update_skeleton()
	{
		int child_offset = 1;
		glm::vec3 start, end;
		for (size_t bone_index = 0; bone_index < mesh->total_bones; ++bone_index)
		{
			// Iterate over each bone and calculate the global transform
			// for each bone.
			assets::Bone* bone = &mesh->bones[bone_index];
			AnimatedNode* node = (AnimatedNode*)children[child_offset+bone_index];
			glm::mat4& tr = transforms[bone_index];

			if (bone->parent_index == -1)
			{
				tr = node->local_to_world;
				start = glm::vec3(glm::column(this->world_transform * tr, 3));
			}
			else
			{
				tr = transforms[bone->parent_index] * node->local_to_world;
				end = glm::vec3(glm::column(this->world_transform * this->local_to_world * tr, 3));
			}
			
			
			final_transforms[bone_index] = this->local_to_world * tr * bone->inverse_bind_matrix;

//			glm::mat4 center_bone = this->world_transform * this->local_to_world * tr;
			glm::mat4 center_bone = bone->world_transform;
			glm::vec3 bone_center = glm::vec3(glm::column(center_bone, 3));
			debugdraw::sphere(bone_center, Color(0, 128, 255, 255), 0.10f, 0.0f);
			debugdraw::axes(this->world_transform * this->local_to_world * tr, 0.5f, 0.0f);
			debugdraw::line(start, end, Color(255,255,0,255), 0.0f);
			start = end;
		}
	}
}; // namespace scenegraph
