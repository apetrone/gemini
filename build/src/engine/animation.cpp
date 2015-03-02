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
#include "animation.h"
#include <platform/mem.h>

#include <core/interpolation.h>
#include <core/mathlib.h>
#include <core/dictionary.h>
#include <core/logging.h>

#include <vector>

namespace gemini
{
	namespace animation
	{
		//
		// KeyframeList
		//
		
		KeyframeList::KeyframeList() : keys(0), total_keys(0), duration_seconds(0.0f)
		{}
		
		KeyframeList::~KeyframeList()
		{
			deallocate();
		}
		
		void KeyframeList::allocate(size_t key_count)
		{
			total_keys = key_count;
			keys = CREATE_ARRAY(Keyframe, total_keys);
		}
		
		void KeyframeList::deallocate()
		{
			DESTROY_ARRAY(Keyframe, keys, total_keys);
			total_keys = 0;
		}
		
		void KeyframeList::set_key(const size_t index, const float seconds, const float value)
		{
			keys[index].seconds = seconds;
			keys[index].value = value;
		}
	
		//
		// Channel
		//

		Channel::Channel(float* target, bool should_wrap) : value(target)
		{
			keyframelist = 0;
			local_time_seconds = 0.0f;
			current_keyframe = 0;
			wrap = should_wrap;
		}
		
		Channel::~Channel()
		{}
		
		void Channel::set_target(float *target)
		{
			value = target;
		}
		
		void Channel::set_keyframe_list(KeyframeList* source_keyframe_list)
		{
			keyframelist = source_keyframe_list;
		}
		
		void Channel::advance(float delta_seconds)
		{
			if (!keyframelist)
			{
				return;
			}
			
			if (!value)
			{
				return;
			}
			
			if (keyframelist->duration_seconds == 0)
			{
				*value = 0;
				return;
			}
//			assert(keyframelist->duration_seconds > 0.0f);
			
			local_time_seconds += delta_seconds;
			
			// determine the next keyframe; this must not wrap
			uint32_t next_keyframe = current_keyframe + 1;
			if (next_keyframe > keyframelist->total_keys)
			{
				next_keyframe = keyframelist->total_keys;
			}
			
			// our local time has passed the sequence duration
			if (local_time_seconds > keyframelist->duration_seconds)
			{
				current_keyframe = 0;
				if (wrap)
				{
					local_time_seconds -= keyframelist->duration_seconds;
				}
				else
				{
					local_time_seconds = keyframelist->duration_seconds;
				}
			}
			
			float last_keyframe_time = keyframelist->keys[current_keyframe].seconds;
			float next_time = keyframelist->keys[next_keyframe].seconds;
			assert(next_time != 0.0f);
			
			// alpha is calculated by dividing the deltas: (a/b)
			// a. The delta between the current simulation time and the last key frame's time
			// b. The delta between the next key frame's time and the last key frame's time.
			float alpha = (local_time_seconds - last_keyframe_time) / (next_time - last_keyframe_time);
			alpha = glm::clamp(alpha, 0.0f, 1.0f);
			
			// update value
			float prev_value = keyframelist->keys[current_keyframe].value;
			float next_value;
//			if ((current_keyframe+1) >= keyframelist->total_keys)
//			{
//				// TODO: should use post-infinity here
//				// For now, just use the last keyframe.
//				next_value = keyframelist->keys[current_keyframe].value;
//			}
//			else
			{
				next_value = keyframelist->keys[next_keyframe].value;
			}
			
			// lerp the value
			*value = core::lerp(prev_value, next_value, alpha);
			
			// determine if we should advance the keyframe index
			if (local_time_seconds >= next_time)
			{
				// advance the frame
				++current_keyframe;
				
				// catch out of frame bounds
				if (current_keyframe == keyframelist->total_keys-1)
				{
					current_keyframe = 0;
					local_time_seconds -= next_time;
				}
			}
		} // advance
		
		void Channel::reset()
		{
			current_keyframe = 0;
			local_time_seconds = 0;
		} // reset
		
		float Channel::operator()() const
		{
			return *value;
		} // operator()
	
	
	
		// AnimatedInstance
		
		AnimatedInstance::AnimatedInstance() :
			local_time_seconds(0.0),
			enabled(true)
		{
		}
		
		void AnimatedInstance::initialize(Sequence* sequence)
		{
			sequence_index = sequence->index;
		
			// reserve enough space
			AnimationSet.allocate(sequence->AnimationSet.size());
			ChannelSet.allocate(sequence->AnimationSet.size());
			
			// associate the channels with the keyframe lists
			size_t index = 0;
			for (const KeyframeListArray& array : sequence->AnimationSet)
			{
				AnimationSet[index].allocate(sequence->AnimationSet[index].size());
				ChannelSet[index].allocate(sequence->AnimationSet[index].size());
				for (size_t channel_index = 0; channel_index < sequence->AnimationSet[index].size(); ++channel_index)
				{
					Channel& channel = ChannelSet[index][channel_index];
					channel.set_keyframe_list(&sequence->AnimationSet[index][channel_index]);
					channel.set_target(&AnimationSet[index][channel_index]);
				 }
				
				++index;
			}
		}
		
		void AnimatedInstance::advance(float delta_seconds)
		{
			Sequence* sequence = animation::get_sequence_by_index(sequence_index);
			assert(sequence != 0);

			for (auto& channelset : ChannelSet)
			{
				for (Channel& channel : channelset)
				{
					channel.advance(delta_seconds);
				}
			}
		
//			for (size_t index = 0; index < ChannelSet.size(); ++index)
//			{
//				for (size_t channel_index = 0; channel_index < ChannelSet[index].size(); ++channel_index)
//				{
//					Channel& channel = ChannelSet[index][channel_index];
//					channel.advance(delta_seconds);
//				}
//			}
		}
	
		void AnimatedInstance::reset_channels()
		{
			Sequence* sequence = animation::get_sequence_by_index(sequence_index);
			assert(sequence != 0);
			
			for (auto& channelset : ChannelSet)
			{
				for (Channel& channel : channelset)
				{
					channel.reset();
				}
			}
		}
	
		//
		// animation system stuff
		//
		
		namespace detail
		{
			typedef core::Dictionary<Sequence*> SequenceHash;
			typedef std::vector<Sequence*> SequenceArray;
			typedef std::vector<AnimatedInstance*> InstanceArray;
			
			SequenceHash _sequences_by_name;
			SequenceArray _sequences;
			InstanceArray _instances;
			
			
			Sequence* load_sequence_from_file(const char* name)
			{
				if (_sequences_by_name.has_key(name))
				{
					Sequence* data = 0;
					_sequences_by_name.get(name, data);
					return data;
				}
				
				// TODO: do file loading
				Sequence* sequence = CREATE(Sequence);
				sequence->index = _sequences.size();
				
				// load the data into the sequence
				sequence->AnimationSet.allocate(2);
				sequence->AnimationSet[0].allocate(6);
				sequence->AnimationSet[1].allocate(6);
				KeyframeList& tx = sequence->AnimationSet[0][0];
				tx.allocate(2);
				tx.set_key(0, 0.0f, 0.0f);
				tx.set_key(1, 1.0f, 0.0f);
				tx.duration_seconds = 1.0f;
				
				KeyframeList& ty = sequence->AnimationSet[0][1];
				ty.allocate(2);
				ty.set_key(0, 0.0f, 0.0f);
				ty.set_key(1, 1.0f, 0.0f);
				ty.duration_seconds = 1.0f;
				
				KeyframeList& tz = sequence->AnimationSet[0][2];
				tz.allocate(2);
				tz.set_key(0, 0.0f, 0.0f);
				tz.set_key(1, 1.0f, 0.0f);
				tz.duration_seconds = 1.0f;
				
				
				KeyframeList& rx = sequence->AnimationSet[0][3];
//				rx.allocate(2);
//				rx.set_key(0, 0.0f, 0.0f);
//				rx.set_key(1, 60.0f, mathlib::degrees_to_radians(360.0f));
//				rx.duration_seconds = 60.0f;
				
//				KeyframeList& ry = sequence->AnimationSet[0][4];
//				ry.allocate(2);
//				ry.set_key(0, 0.0f, 0.0f);
//				ry.set_key(1, 3.0f, mathlib::degrees_to_radians(45.0f));
//				ry.duration_seconds = 3.0f;
//				
//				KeyframeList& rz = sequence->AnimationSet[0][5];
//				rz.allocate(2);
//				rz.set_key(0, 0.0f, 0.0f);
//				rz.set_key(1, 1.0f, 0.0f);
//				rz.duration_seconds = 1.0f;
				
				
				
				// bone 1
				{
					KeyframeList& tx = sequence->AnimationSet[1][0];
					tx.allocate(2);
					tx.set_key(0, 0.0f, 0.0f);
					tx.set_key(1, 1.0f, 0.0f);
					tx.duration_seconds = 1.0f;
					
					KeyframeList& ty = sequence->AnimationSet[1][1];
					ty.allocate(2);
					ty.set_key(0, 0.0f, 0.0f);
					ty.set_key(1, 1.0f, 0.0f);
					ty.duration_seconds = 1.0f;
					
					KeyframeList& tz = sequence->AnimationSet[1][2];
					tz.allocate(2);
					tz.set_key(0, 0.0f, 0.0f);
					tz.set_key(1, 1.0f, 0.0f);
					tz.duration_seconds = 1.0f;
					
					KeyframeList& rx = sequence->AnimationSet[1][3];
					rx.allocate(2);
					rx.set_key(0, 0.0f, 0.0f);
					rx.set_key(1, 1.0f, mathlib::degrees_to_radians(360.0f));
					rx.duration_seconds = 1.0f;
					
//					KeyframeList& ry = sequence->AnimationSet[1][4];
//					ry.allocate(2);
//					ry.set_key(0, 0.0f, 0.0f);
//					ry.set_key(1, 1.0f, mathlib::degrees_to_radians(0.0f));
//					ry.duration_seconds = 1.0f;
//					
//					KeyframeList& rz = sequence->AnimationSet[1][5];
//					rz.allocate(2);
//					rz.set_key(0, 0.0f, 0.0f);
//					rz.set_key(1, 1.0f, 0.0f);
//					rz.duration_seconds = 1.0f;
				}
				
				
				sequence->name = "animations/idle";
				_sequences.push_back(sequence);
				_sequences_by_name.insert("animations/idle", sequence);
				
				{
					Sequence* s = CREATE(Sequence);
					s->index = _sequences.size();
					s->name = "animations/walk";
					_sequences.push_back(s);
					_sequences_by_name.insert("animations/walk", s);
					
					s->AnimationSet.allocate(2);
					s->AnimationSet[0].allocate(3);
					s->AnimationSet[1].allocate(3);
					
					{
						KeyframeList& ty = s->AnimationSet[0][1];
						ty.allocate(2);
						ty.duration_seconds = 1.0f;
						ty.set_key(0, 0.0f, 0.0f);
						ty.set_key(1, 1.0f, 5.0f);
					}
					
					
				}
				
				Sequence* data;
				_sequences_by_name.get(name, data);
				return data;
			} // load_sequence_from_file
		}
		
		
		void startup()
		{
		}
		
		void shutdown()
		{
			for (Sequence* sequence : detail::_sequences)
			{
				DESTROY(Sequence, sequence);
			}
			detail::_sequences.clear();
			
			for (AnimatedInstance* instance : detail::_instances)
			{
				destroy_sequence_instance(instance);
			}
			detail::_instances.clear();
			
			detail::_sequences_by_name.clear();
		}
		
		
		void update(float delta_seconds)
		{
			for (AnimatedInstance* instance : detail::_instances)
			{
				if (instance->enabled)
				{
					instance->advance(delta_seconds);
				}
			}
		}

		
		SequenceId load_sequence(const char* name)
		{
			Sequence* sequence = detail::load_sequence_from_file(name);
			AnimatedInstance* instance = create_sequence_instance(sequence->index);
			return instance->index;
		}
		
		SequenceId find_sequence(const char* name)
		{
			if (detail::_sequences_by_name.has_key(name))
			{
				Sequence* data = 0;
				detail::_sequences_by_name.get(name, data);
				return data->index;
			}
			
			return -1;
		}
		
		Sequence* get_sequence_by_index(SequenceId index)
		{
			assert(index < detail::_sequences.size());
			return detail::_sequences[index];
		}
		
		AnimatedInstance* create_sequence_instance(SequenceId index)
		{
			Sequence* source = get_sequence_by_index(index);
			AnimatedInstance* instance = CREATE(AnimatedInstance);
			instance->initialize(source);
		
			instance->index = detail::_instances.size();
			detail::_instances.push_back(instance);
		
			return instance;
		}
		
		void destroy_sequence_instance(AnimatedInstance* instance)
		{
			DESTROY(AnimatedInstance, instance);
		}
		
		AnimatedInstance* get_instance_by_index(SequenceId index)
		{
			assert(index < detail::_instances.size());
			return detail::_instances[index];
		}
		
	} // namespace animation
} // namespace gemini