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

#include <vector>

#include <core/mathlib.h>
#include "keyframechannel.h"

namespace gemini
{
	namespace scenegraph
	{
		typedef std::vector< struct Node*, CustomPlatformAllocator<Node*> > NodeVector;

		enum NodeType
		{
			SCENEROOT 		= 0, 	// the scene root
			NODE			= 1, 	// generic node
			STATIC_MESH		= 2,	// geometry/mesh node
			RENDER			= 4,	// is a render node
			SKELETON		= 8,	// render a skeleton
			ANIMATED		= 16,	// has animatable position/rotation/scale
		};

		struct Node
		{
			typedef uint16_t AttributeType;

			// decomposed pieces
			glm::vec3 local_scale;
			glm::quat local_rotation;
			glm::vec3 local_position;

			// the local to world transform
			glm::mat4 local_to_world;

			// local to pivot point vector
			glm::vec3 local_to_pivot;

			// the final world-transform for this model
			glm::mat4 world_transform;

			glm::vec3 scale;
			glm::quat rotation;
			glm::vec3 translation;

			String name;

			NodeVector children;
			Node* parent;

			NodeType type;
			AttributeType attributes;

			Node();
			Node(const Node& other);
			virtual ~Node();

			void add_child(Node* child);
			void remove_child(Node* child);
			Node* find_child_named(const String& name);
			virtual void update(float delta_seconds);
			void update_transforms();
			void clear();
			NodeType get_type() const { return type; }

			bool has_attributes(AttributeType flags) { return (flags & attributes); }
			virtual Node* clone() { return CREATE(Node, *this); }
		};

		struct Visitor
		{
			virtual ~Visitor() {}
			virtual int32_t visit(Node* node) = 0;
		};


		void create_scene(Node* root);
		void visit_nodes(Node* root, Visitor* visitor);
		void destroy_scene(Node* root);
		void print_tree(Node* root);

		// TODO: move these?
		scenegraph::Node* clone_to_scene(scenegraph::Node* template_node, scenegraph::Node* root);
	} // namespace scenegraph
} // namespace gemini

