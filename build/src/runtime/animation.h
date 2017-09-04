// -------------------------------------------------------------
// Copyright (C) 2015- Adam Petrone
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
#include <core/mathlib.h>
#include <core/stackstring.h>
#include <core/typedefs.h>

#include <shared/shared_constants.h>


namespace gemini
{
	struct Allocator;
	struct Mesh;

	namespace animation
	{
		template <class T>
		struct Keyframe
		{
			// absolute time for the keyframe
			float seconds;

			// value at seconds
			T value;
		}; // Keyframe

		template <class T>
		struct KeyframeList
		{
			Keyframe<T>* keys;
			uint32_t total_keys;
			float duration_seconds;
			gemini::Allocator& allocator;

			KeyframeList(gemini::Allocator& _allocator);
			~KeyframeList();

			void allocate(size_t key_count);
			void deallocate();
			void set_key(const size_t index, const float seconds, const T& value);
		}; // KeyframeList

		// This contains a stateful representation of a KeyframeList.
		// It only uses the key frame list as a source of data.
		template <class T>
		class Channel
		{
		private:
			// source key frame list
			KeyframeList<T>* keyframelist;

			// is this a loopable anim; if so, we ignore the last keyframe and wrap
			// because keyframe[first] == keyframe[last]
			bool wrap;

		public:
			Channel(float* target = 0, bool should_wrap = true);
			~Channel();

			void set_keyframe_list(KeyframeList<T>* source_keyframe_list);

			// evaluate this channel at time t_seconds
			// behavior will vary based on whether or not the animation is looping.
			T evaluate(float t_seconds, float frame_delay_seconds, bool looping = false) const;
		}; // Channel


		//
		// supporting structures
		//

		struct Pose
		{
			glm::vec3 pos[MAX_BONES];
			glm::quat rot[MAX_BONES];
		};

		typedef int32_t SequenceId;
		struct Sequence
		{
			// length of this sequence in seconds
			float duration_seconds;
			float frame_delay_seconds;

			uint32_t looping;

			core::StackString<64> name;
			SequenceId index;
			gemini::Allocator& allocator;

			FixedArray<KeyframeList<glm::vec3>> translations;
			FixedArray<KeyframeList<glm::quat>> rotations;

			Sequence(gemini::Allocator& allocator);
		}; // Sequence

		struct AnimatedInstance
		{
			enum class Flags
			{
				Idle,		// has not played yet
				Playing,	// enabled
				Finished	// finished playing
			};

			SequenceId index;
			float local_time_seconds;
			SequenceId sequence_index;
			FixedArray<Channel<glm::vec3>> translation_channel;
			FixedArray<Channel<glm::quat>> rotation_channel;

			Flags flags;

			AnimatedInstance(gemini::Allocator& allocator);
			virtual ~AnimatedInstance();

			virtual void initialize(Sequence* sequence);
			virtual void advance(float delta_seconds);
			virtual bool is_playing() const;
			virtual bool is_finished() const;
			virtual void reset_channels();
		}; // AnimatedInstance

		//
		// animation system
		//
		void startup(gemini::Allocator& allocator);
		void shutdown();
		void update(float delta_seconds);

		Sequence* load_sequence_from_file(gemini::Allocator& allocator, const char* name, Mesh* mesh);
		SequenceId find_sequence(const char* name);
		Sequence* get_sequence_by_index(SequenceId index);

		// caller must clean up the returned instance by calling destroy_sequence_instance.
		AnimatedInstance* create_sequence_instance(gemini::Allocator& allocator);
		void destroy_sequence_instance(gemini::Allocator& allocator, AnimatedInstance* instance);
		AnimatedInstance* get_instance_by_index(SequenceId index);

		void animated_instance_get_pose(AnimatedInstance* instance, Pose& pose);

		//void animation_interpolate_pose(Pose& out, Pose& last_pose, Pose& curr_pose, float t);
	}
} // namespace gemini

namespace gemini
{
	struct TransformNode;
	struct AnimatedMeshComponent;

	struct AnimationController
	{
		// Target is the AnimatedEntity transform node.
		// It is assumed that target contains child nodes
		// that will be populated directly from an animation.
		TransformNode* target;

		AnimatedMeshComponent* component;
	};

	void animation_controller_transfer(AnimationController* controller);
	void animation_controller_extract(AnimationController* controller);

	void animation_link_transform_and_component(TransformNode* node, AnimatedMeshComponent* component);

	void animation_update_transform_nodes();
	void animation_update_components();
} // namespace gemini