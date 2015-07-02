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

#include <assets/asset_mesh.h>

#include <core/mem.h>

#include <hashset.h>

#include <runtime/logging.h>
#include <runtime/configloader.h>

#include <core/interpolation.h>
#include <core/mathlib.h>
#include <core/stackstring.h>

#include <vector>

using namespace gemini::assets;

using gemini::animation::Keyframe;

using namespace platform;

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
			keys = MEMORY_NEW_ARRAY(Keyframe, total_keys, core::memory::global_allocator());
		}
		
		void KeyframeList::deallocate()
		{
			MEMORY_DELETE_ARRAY(keys, core::memory::global_allocator());
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
			enabled(false)
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
		typedef HashSet<std::string, Sequence*> SequenceHash;
		SequenceHash* _sequences_by_name;
		namespace detail
		{
			
			typedef std::vector<Sequence*> SequenceArray;
			typedef std::vector<AnimatedInstance*> InstanceArray;
			

			SequenceArray _sequences;
			InstanceArray _instances;
			
			struct AnimationSequenceLoadData
			{
				Sequence* sequence;
				Mesh* mesh;
			};
			
			static bool validate_node(const Json::Value& node, const char* error_message)
			{
				if (node.isNull())
				{
					LOGW("node is null: '%s'\n", error_message);
					return false;
				}
				
				return true;
			}
			
			template <class Type>
			void from_json(Type& output, const Json::Value& input);
			
			template <>
			void from_json(glm::vec3& output, const Json::Value& input)
			{
				output = glm::vec3(input[0].asFloat(), input[1].asFloat(), input[2].asFloat());
			}
			
			template <>
			void from_json(glm::quat& output, const Json::Value& input)
			{
				output = glm::quat(input[3].asFloat(), input[0].asFloat(), input[1].asFloat(), input[2].asFloat());
			}


			core::util::ConfigLoadStatus load_animation_from_json(const Json::Value& root, void* data)
			{
				AnimationSequenceLoadData* load_data = static_cast<AnimationSequenceLoadData*>(data);
				Mesh* mesh = load_data->mesh;
				Sequence* sequence = load_data->sequence;
				
				core::util::ConfigLoadStatus result = core::util::ConfigLoad_Failure;
				
				if (!mesh->has_skeletal_animation)
				{
					LOGW("Tried to attach an animation to a non-animated model!\n");
					return result;
				}
				
				if (root.isNull())
				{
					return result;
				}
				
				const Json::Value& bones_array = root["nodes"];
				if (!validate_node(bones_array, "bones"))
				{
					LOGW("No bones!\n");
					return result;
				}
				
				assert(bones_array.size() == mesh->skeleton.size());
				
				if (bones_array.size() != mesh->skeleton.size())
				{
					LOGV("# animated bones does not match model's skeleton!\n");
					return result;
				}
				
				const Json::Value& frames_per_second = root["frames_per_second"];
				if (!validate_node(frames_per_second, "frames_per_second"))
				{
					return result;
				}
				
				const Json::Value& animation_name = root["name"];
				if (!validate_node(animation_name, "name"))
				{
					return result;
				}
				
				
				const Json::Value& duration_seconds = root["duration_seconds"];
				if (!validate_node(duration_seconds, "duration_seconds"))
				{
					return result;
				}
				
				sequence->duration_seconds = duration_seconds.asFloat();
				
				std::string animation_title = animation_name.asString();
				LOGV("animation: \"%s\"\n", animation_title.c_str());
				
				int32_t fps_rate = frames_per_second.asInt();
				sequence->frame_delay_seconds = (1.0f/(float)fps_rate);

				// 1. allocate enough space for each bone
				sequence->AnimationSet.allocate(bones_array.size());

				Json::ValueIterator node_iter = bones_array.begin();
				size_t node_index = 0;
				for (; node_iter != bones_array.end(); ++node_iter)
				{
					const Json::Value& jnode = (*node_iter);
					std::string node_name = jnode["name"].asString().c_str();

					const Json::Value& scale_keys = jnode["scale"];
					const Json::Value& rotation_keys = jnode["rotation"];
					const Json::Value& translation_keys = jnode["translation"];
					assert(!scale_keys.isNull() && !rotation_keys.isNull() && !translation_keys.isNull());

					Joint* joint = mesh->find_bone_named(node_name.c_str());
					assert(joint != 0);
					
					LOGV("reading keyframes for bone \"%s\", joint->index = %i\n", joint->name(), joint->index);
					
					FixedArray<KeyframeList>& kfl = sequence->AnimationSet[joint->index];
					kfl.allocate(7);

					// translation
					{
//						LOGV("translation keyframes\n");
						const Json::Value& tr_values = translation_keys["value"];
						const Json::Value& tr_times = translation_keys["time"];
						
						assert(!tr_values.isNull() && !tr_times.isNull());
						
						// In Json, there are EQUAL entries for each scale/rotation/translation tracks.
						// This MAY NOT be like this in other formats -- so revisit this later.
						int total_keys = tr_values.size();
						
						KeyframeList& tx = kfl[0];
						tx.allocate(total_keys); tx.duration_seconds = 2.0f;
						KeyframeList& ty = kfl[1];
						ty.allocate(total_keys); ty.duration_seconds = 2.0f;
						KeyframeList& tz = kfl[2];
						tz.allocate(total_keys); tz.duration_seconds = 2.0f;
						
						for (unsigned int index = 0; index < tr_values.size(); ++index)
						{
							const Json::Value& value = tr_values[index];
							const Json::Value& time = tr_times[index];
							
							float x = value[0].asFloat();
							float y = value[1].asFloat();
							float z = value[2].asFloat();
							
							float t = time.asFloat();
							
							tx.set_key(index, t, x);
							ty.set_key(index, t, y);
							tz.set_key(index, t, z);
//							LOGV("t=%2.2f, %g %g %g\n", t, x, y, z);
						}
					}
					
					// rotation
					{
//						LOGV("rotation keyframes\n");
						const Json::Value& values = rotation_keys["value"];
						const Json::Value& times = rotation_keys["time"];
						
						assert(!values.isNull() && !times.isNull());
						
						// In Json, there are EQUAL entries for each scale/rotation/translation tracks.
						// This MAY NOT be like this in other formats -- so revisit this later.
						int total_keys = values.size();
						
						KeyframeList& rx = kfl[3];
						rx.allocate(total_keys); rx.duration_seconds = 2.0f;
						KeyframeList& ry = kfl[4];
						ry.allocate(total_keys); ry.duration_seconds = 2.0f;
						KeyframeList& rz = kfl[5];
						rz.allocate(total_keys); rz.duration_seconds = 2.0f;
						KeyframeList& rw = kfl[6];
						rw.allocate(total_keys); rw.duration_seconds = 2.0f;
						
						for (unsigned int index = 0; index < values.size(); ++index)
						{
							const Json::Value& value = values[index];
							const Json::Value& time = times[index];
							
							float x = value[0].asFloat();
							float y = value[1].asFloat();
							float z = value[2].asFloat();
							float w = value[3].asFloat();
							
							float t = time.asFloat();
							
							rx.set_key(index, t, x);
							ry.set_key(index, t, y);
							rz.set_key(index, t, z);
							rw.set_key(index, t, w);
//							LOGV("t=%2.2f, %g %g %g %g\n", t, x, y, z, w);
						}
					}

					++node_index;
				}

				return core::util::ConfigLoad_Success;
			}
			
			
			Sequence* load_sequence_from_file(const char* name, Mesh* mesh)
			{
				if (_sequences_by_name->has_key(name))
				{
					Sequence* data = 0;
					data = _sequences_by_name->get(name);
					return data;
				}
				
				
				Sequence* sequence = MEMORY_NEW(Sequence, core::memory::global_allocator());
				core::StackString<MAX_PATH_SIZE> filepath = name;
				filepath.append(".animation");
				AnimationSequenceLoadData data;
				data.mesh = mesh;
				data.sequence = sequence;
				sequence->name = name;
				if (core::util::ConfigLoad_Success == core::util::json_load_with_callback(filepath(), load_animation_from_json, &data, true))
				{
					sequence->index= _sequences.size();
					_sequences_by_name->insert(SequenceHash::value_type(name, sequence));
					_sequences.push_back(sequence);
				}
				else
				{
					MEMORY_DELETE(sequence, core::memory::global_allocator());
				}
			
				return sequence;
			} // load_sequence_from_file
		}
		
		
		void startup()
		{
			_sequences_by_name = MEMORY_NEW(SequenceHash, core::memory::global_allocator());
		}
		
		void shutdown()
		{
			for (Sequence* sequence : detail::_sequences)
			{
				MEMORY_DELETE(sequence, core::memory::global_allocator());
			}
			detail::_sequences.clear();
			
			for (AnimatedInstance* instance : detail::_instances)
			{
				destroy_sequence_instance(instance);
			}
			detail::_instances.clear();
			
			MEMORY_DELETE(_sequences_by_name, core::memory::global_allocator());
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

		
		SequenceId load_sequence(const char* name, Mesh* mesh)
		{
			Sequence* sequence = detail::load_sequence_from_file(name, mesh);
			if (sequence)
			{
				AnimatedInstance* instance = create_sequence_instance(sequence->index);
				return instance->index;				
			}

			return -1;
		}
		
		SequenceId find_sequence(const char* name)
		{
			if (_sequences_by_name->has_key(name))
			{
				Sequence* data = 0;
				data = _sequences_by_name->get(name);
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
			AnimatedInstance* instance = MEMORY_NEW(AnimatedInstance, core::memory::global_allocator());
			instance->initialize(source);
		
			instance->index = detail::_instances.size();
			detail::_instances.push_back(instance);
		
			return instance;
		}
		
		void destroy_sequence_instance(AnimatedInstance* instance)
		{
			MEMORY_DELETE(instance, core::memory::global_allocator());
		}
		
		AnimatedInstance* get_instance_by_index(SequenceId index)
		{
			assert(index < detail::_instances.size());
			return detail::_instances[index];
		}
		
	} // namespace animation
} // namespace gemini