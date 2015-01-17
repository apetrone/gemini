// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone
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
#include <core/typedefs.h>
#include <core/logging.h>

#include "debugdraw.h"

#include "scene_graph.h"
#include "skeletalnode.h"
//#include "core/assets/asset_mesh.h"

namespace gemini
{
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
	#if 0
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
	#endif
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
	#if 0
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
	#endif
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
	#if 0
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
	#endif
		}
	} // namespace scenegraph
} // namespace gemini