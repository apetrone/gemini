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
#include <core/stackstring.h>
#include <core/typedefs.h>

namespace gemini
{
	struct Allocator;

	const size_t ANIMATION_KEYFRAME_VALUES_MAX = 7;

	struct Mesh;


	namespace animation
	{
		struct Keyframe
		{
			// absolute time for the keyframe
			float seconds;

			// value at seconds
			float value;
		}; // Keyframe

		struct KeyframeList
		{
			Keyframe* keys;
			uint32_t total_keys;
			float duration_seconds;
			gemini::Allocator& allocator;

			KeyframeList(gemini::Allocator& _allocator);
			~KeyframeList();

			void allocate(size_t key_count);
			void deallocate();
			void set_key(const size_t index, const float seconds, const float value);
		}; // KeyframeList

		// This contains a stateful representation of a KeyframeList.
		// It only uses the key frame list as a source of data.
		class Channel
		{
		private:
			// source keyframe list
			KeyframeList* keyframelist;

			// local time in seconds [0, keyframelist->duration_seconds]
			float local_time_seconds;

			// current key frame index [0, keyframelist->total_keys-1]
			uint32_t current_keyframe;

			// value by reference
			float* value;

			// is this a loopable anim; if so, we ignore the last keyframe and wrap
			// because keyframe[first] == keyframe[last]
			bool wrap;

		public:
			Channel(float* target = 0, bool should_wrap = true);
			~Channel();

			void set_target(float* target);
			void set_keyframe_list(KeyframeList* source_keyframe_list);
			void advance(float delta_seconds);
			void reset();

			float operator()() const;
		}; // Channel


		//
		// supporting structures
		//

		typedef int32_t SequenceId;
		struct Sequence
		{
			// length of this sequence in seconds
			float duration_seconds;
			float frame_delay_seconds;

			core::StackString<64> name;
			SequenceId index;
			gemini::Allocator& allocator;

			FixedArray<KeyframeList> animation_set;

			Sequence(gemini::Allocator& allocator);
		}; // Sequence


		struct AnimatedInstance
		{
			SequenceId index;
			float local_time_seconds;
			SequenceId sequence_index;
			FixedArray<float> animation_set;
			FixedArray<Channel> channel_set;
			bool enabled;

			AnimatedInstance(gemini::Allocator& allocator);
			virtual ~AnimatedInstance() {}

			virtual void initialize(Sequence* sequence);
			virtual void advance(float delta_seconds);
			virtual bool is_playing() const { return enabled; }
			virtual void reset_channels();
		}; // AnimatedInstance

		//
		// animation system
		//
		void startup(gemini::Allocator& allocator);
		void shutdown();
		void update(float delta_seconds);

		SequenceId load_sequence(gemini::Allocator& allocator, const char* name, Mesh* mesh);
		SequenceId find_sequence(const char* name);
		Sequence* get_sequence_by_index(SequenceId index);

		AnimatedInstance* create_sequence_instance(gemini::Allocator& allocator, SequenceId index);
		void destroy_sequence_instance(gemini::Allocator& allocator, AnimatedInstance* instance);
		AnimatedInstance* get_instance_by_index(SequenceId index);
	}
} // namespace gemini
