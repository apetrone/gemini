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
#pragma once

#include <gemini/util/fixedarray.h>

#include "scene_graph.h"
#include "meshnode.h"
#include "assets/asset_mesh.h"
#include "keyframechannel.h"
#include "assets/asset_mesh.h"

namespace scenegraph
{
	struct AnimatedNode : public Node
	{
		glm::vec3 scale;
		Channel<glm::vec3> scale_channel;
		
		glm::quat rotation;
		Channel<glm::quat> rotation_channel;
		
		glm::vec3 translation;
		Channel<glm::vec3> translation_channel;
		
		
		AnimatedNode();
		
		void post_processing(assets::Mesh* mesh, int32_t node_index);
		
		virtual void update(float delta_seconds);
	};

	struct SkeletalNode : public MeshNode
	{
		// TODO: This should be moved into an animation controller
		// which can manage multiple animation skeletal transforms
		// instead of one.
		FixedArray<glm::mat4> transforms;

		FixedArray<glm::mat4> final_transforms;

		SkeletalNode();
		virtual ~SkeletalNode() {}
		void setup_skeleton();
		virtual void update(float delta_seconds);
		void update_skeleton();
	};
}; // namespace scenegraph


