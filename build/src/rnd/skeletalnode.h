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
#pragma once

#include <core/fixedarray.h>
#include <renderer/renderer.h>
#include "scene_graph.h"
#include "skeletalnode.h"

namespace gemini
{
	namespace scenegraph
	{
		struct AnimatedNode : public Node
		{
			Channel<glm::vec3> scale_channel;
			Channel<glm::quat> rotation_channel;
			Channel<glm::vec3> translation_channel;

			AnimatedNode();
			AnimatedNode(AnimatedNode& other);

			//void post_processing(assets::Mesh* mesh, int32_t node_index);

			virtual void update(float delta_seconds);

			virtual Node* clone() { return CREATE(AnimatedNode, *this); }
		};

		struct RenderNode : public AnimatedNode
		{
			uint16_t material_id;
			uint16_t shader_id;

			renderer::Geometry* geometry;
			//assets::Mesh* mesh;

			bool visible;

			RenderNode();

			virtual Node* clone() { return CREATE(RenderNode, *this); }
		};

		struct SkeletalNode : public RenderNode
		{
			// TODO: This should be moved into an animation controller
			// which can manage multiple animation skeletal transforms
			// instead of one.
			core::FixedArray<glm::mat4> transforms;
			core::FixedArray<glm::mat4> final_transforms;

			SkeletalNode();
			void setup_skeleton();
			virtual void update(float delta_seconds);
			void update_skeleton();

			virtual Node* clone() { return CREATE(SkeletalNode, *this); }
		};
	} // namespace scenegraph
} // namespace gemini
