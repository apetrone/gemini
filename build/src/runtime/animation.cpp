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

#include <core/mem.h>
#include <core/linearfreelist.h>
#include <core/logging.h>
#include <core/interpolation.h>
#include <core/mathlib.h>
#include <core/stackstring.h>

#include <platform/platform.h>

#include <hashset.h>

#include <runtime/configloader.h>
#include <runtime/mesh.h>

#include <renderer/debug_draw.h>


#include <vector>

using gemini::animation::Keyframe;

using namespace platform;

namespace gemini
{
	namespace animation
	{
		struct AnimationState
		{
			Allocator* allocator;
			LinearFreeList<Sequence> sequences;
			LinearFreeList<AnimatedInstance*> instances;

			AnimationState(Allocator& in_allocator)
				: allocator(&in_allocator)
				, sequences(in_allocator)
				, instances(in_allocator)
			{
			}
		};

		AnimationState* _animation_state = nullptr;



		//
		// KeyframeList
		//

		KeyframeList::KeyframeList(gemini::Allocator& _allocator)
			: allocator(_allocator)
			, keys(0)
			, total_keys(0)
			, duration_seconds(0.0f)
		{
		}

		KeyframeList::~KeyframeList()
		{
			deallocate();
		}

		void KeyframeList::allocate(size_t key_count)
		{
			total_keys = static_cast<uint32_t>(key_count);
			keys = MEMORY2_NEW_ARRAY(allocator, Keyframe, total_keys);
		}

		void KeyframeList::deallocate()
		{
			MEMORY2_DELETE_ARRAY(allocator, keys);
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

		Channel::Channel(float* target, bool should_wrap)
		{
			keyframelist = 0;
			wrap = should_wrap;
		}

		Channel::~Channel()
		{}

		//void Channel::set_target(float *target)
		//{
		//	value = target;
		//}

#if 0
		animation::SequenceId instance_index = animations[index];
		animation::AnimatedInstance* instance = animation::get_instance_by_index(instance_index);

		// reset all the channels
		instance->reset_channels();

		// force an advance, to fetch the first frame
		// but don't advance time.
		instance->advance(0.0f);
#endif

		void Channel::set_keyframe_list(KeyframeList* source_keyframe_list)
		{
			keyframelist = source_keyframe_list;
		}

		float Channel::evaluate(float t_seconds, float frame_delay_seconds) const
		{
			if (keyframelist->duration_seconds == 0)
			{
				return 0.0f;
			}

			uint32_t last_key = (keyframelist->total_keys - 1);

			// TODO: Perform a direct lookup into the array with some math.
			// this hasn't been a performance problem yet, fix it when it comes up.
			float delta;
			float value_a;
			float value_b;
			for (uint32_t key = 0; key < keyframelist->total_keys; ++key)
			{
				// alpha is calculated by dividing the deltas: (a/b)
				// a. The delta between the current simulation time and the last key frame's time
				// b. The delta between the next key frame's time and the last key frame's time.
				Keyframe* keyframe = &keyframelist->keys[key];
				uint32_t next_key = key + 1;
				if (t_seconds < keyframe->seconds)
				{
					if (key == 0)
					{
						// can't get previous; lerp forward
						Keyframe* next = &keyframelist->keys[next_key];
						delta = (next->seconds - keyframe->seconds);
						value_a = next->value;
						value_b = keyframe->value;

						float alpha = (delta / frame_delay_seconds);
						return gemini::lerp(value_a, value_b, alpha);
					}
					else
					{
						// This assumes that the animation is evenly sampled
						// across key frames by frame_delay_seconds.
						// If it isn't, we could use
						// (keyframe->seconds - prev_keyframe->seconds) as
						// the denominator instead of frame_delay_seconds.
						Keyframe* prev_keyframe = &keyframelist->keys[key - 1];
						float alpha = (t_seconds - prev_keyframe->seconds) / frame_delay_seconds;
						return gemini::lerp(prev_keyframe->value, keyframe->value, alpha);
					}
				}
				else if (last_key == key)
				{
					// next key would wrap: We may just be able to
					// return the last/first value.
					next_key = 0;
					Keyframe* next = &keyframelist->keys[last_key];
					return next->value;
				}
			}

			return 0.0f;
		} // evaluate

		// Sequence
		Sequence::Sequence(gemini::Allocator& _allocator)
			: allocator(_allocator)
			, animation_set(allocator)
		{
		}

		// AnimatedInstance
		AnimatedInstance::AnimatedInstance(gemini::Allocator& allocator)
			: local_time_seconds(0.0)
			, enabled(false)
			, animation_set(allocator)
			, channel_set(allocator)
		{
		}

		void AnimatedInstance::initialize(Sequence* sequence)
		{
			sequence_index = sequence->index;

			// reserve enough space
			animation_set.allocate(sequence->animation_set.size());
			channel_set.allocate(sequence->animation_set.size());

			for (size_t index = 0; index < sequence->animation_set.size(); ++index)
			{
				Channel& channel = channel_set[index];
				channel.set_keyframe_list(&sequence->animation_set[index]);
			}
		}

		void AnimatedInstance::advance(float delta_seconds)
		{
			local_time_seconds += delta_seconds;

			Sequence* sequence = animation::get_sequence_by_index(sequence_index);
			assert(sequence != 0);

			if (local_time_seconds > sequence->duration_seconds)
			{
				local_time_seconds -= sequence->duration_seconds;
			}
		}

		void AnimatedInstance::reset_channels()
		{
			local_time_seconds = 0.0f;
		}

		//
		// animation system stuff
		//
		typedef HashSet<std::string, Sequence*> SequenceHash;
		SequenceHash* _sequences_by_name;
		gemini::Allocator* _allocator = nullptr;

		struct AnimationSequenceLoadData
		{
			Sequence* sequence;
			Mesh* mesh;
		};

		namespace detail
		{
			typedef std::vector<Sequence*> SequenceArray;
			typedef std::vector<AnimatedInstance*> InstanceArray;

			SequenceArray _sequences;
			InstanceArray _instances;

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

				//if (!mesh->has_skeletal_animation)
				//{
				//	LOGW("Tried to attach an animation to a non-animated model!\n");
				//	return result;
				//}

				if (root.isNull())
				{
					return result;
				}

				const Json::Value& bones_array = root["children"];
				if (!validate_node(bones_array, "bones"))
				{
					LOGW("No bones!\n");
					return result;
				}

				LOGV("bones_array.size() == %i, mesh->skeleton.size() == %i\n",
					bones_array.size(), mesh->skeleton.size());
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
				sequence->animation_set.allocate(bones_array.size() * ANIMATION_KEYFRAME_VALUES_MAX, KeyframeList(sequence->allocator));

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

#if 1
					Joint* joint = mesh_find_bone_named(mesh, node_name.c_str());
					assert(joint != 0);

//					LOGV("reading keyframes for bone \"%s\", joint->index = %i\n", joint->name(), joint->index);

					KeyframeList* kfl = &sequence->animation_set[joint->index * ANIMATION_KEYFRAME_VALUES_MAX];

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
						tx.allocate(total_keys); tx.duration_seconds = duration_seconds.asFloat();
						KeyframeList& ty = kfl[1];
						ty.allocate(total_keys); ty.duration_seconds = duration_seconds.asFloat();
						KeyframeList& tz = kfl[2];
						tz.allocate(total_keys); tz.duration_seconds = duration_seconds.asFloat();

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
						rx.allocate(total_keys); rx.duration_seconds = duration_seconds.asFloat();
						KeyframeList& ry = kfl[4];
						ry.allocate(total_keys); ry.duration_seconds = duration_seconds.asFloat();
						KeyframeList& rz = kfl[5];
						rz.allocate(total_keys); rz.duration_seconds = duration_seconds.asFloat();
						KeyframeList& rw = kfl[6];
						rw.allocate(total_keys); rw.duration_seconds = duration_seconds.asFloat();

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
#endif
					++node_index;
				}

				return core::util::ConfigLoad_Success;
			}
		} // namespace detail

		Sequence* load_sequence_from_file(gemini::Allocator& allocator, const char* name, Mesh* mesh)
		{
			if (_sequences_by_name->has_key(name))
			{
				Sequence* data = 0;
				data = _sequences_by_name->get(name);
				return data;
			}

			Sequence* sequence = MEMORY2_NEW(allocator, Sequence)(allocator);
			platform::PathString filepath = name;
			filepath.append(".animation");
			AnimationSequenceLoadData data;
			data.mesh = mesh;
			data.sequence = sequence;
			sequence->name = name;
			LOGV("loading animation %s\n", filepath());
			if (core::util::ConfigLoad_Success == core::util::json_load_with_callback(filepath(), detail::load_animation_from_json, &data, true))
			{
				sequence->index = detail::_sequences.size();
				_sequences_by_name->insert(SequenceHash::value_type(name, sequence));
				detail::_sequences.push_back(sequence);
			}
			else
			{
				MEMORY2_DELETE(allocator, sequence);
			}

			return sequence;
		} // load_sequence_from_file

		void startup(gemini::Allocator& allocator)
		{
			_allocator = &allocator;
			_sequences_by_name = MEMORY2_NEW(allocator, SequenceHash)(allocator);

			_animation_state = MEMORY2_NEW(allocator, AnimationState)(allocator);
		}

		void shutdown()
		{
			for (Sequence* sequence : detail::_sequences)
			{
				MEMORY2_DELETE(*_allocator, sequence);
			}
			detail::_sequences.clear();
			detail::_instances.clear();

			MEMORY2_DELETE(*_allocator, _sequences_by_name);

			MEMORY2_DELETE(*_animation_state->allocator, _animation_state);
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

		SequenceId load_sequence(gemini::Allocator& allocator, const char* name, Mesh* mesh)
		{
			Sequence* sequence = load_sequence_from_file(allocator, name, mesh);
			if (sequence)
			{
				AnimatedInstance* instance = create_sequence_instance(allocator, sequence->index);
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
			assert(static_cast<size_t>(index) < detail::_sequences.size());
			return detail::_sequences[index];
		}

		AnimatedInstance* create_sequence_instance(gemini::Allocator& allocator, SequenceId index)
		{
			Sequence* source = get_sequence_by_index(index);
			AnimatedInstance* instance = MEMORY2_NEW(allocator, AnimatedInstance)(allocator);
			instance->initialize(source);

			instance->index = detail::_instances.size();
			detail::_instances.push_back(instance);

			return instance;
		}

		void destroy_sequence_instance(gemini::Allocator& allocator, AnimatedInstance* instance)
		{
			//std::vector<AnimatedInstance*>::iterator it = instance->index + detail::_instances.begin();
			//detail::_instances.erase(it);
			MEMORY2_DELETE(allocator, instance);
		}

		AnimatedInstance* get_instance_by_index(SequenceId index)
		{
			assert(static_cast<size_t>(index) < detail::_instances.size());
			return detail::_instances[index];
		}








//#define GEMINI_DEBUG_BONES 1





		void animated_instance_get_pose(AnimatedInstance* instance, Pose& pose)
		{
#if defined(GEMINI_DEBUG_BONES)
			const glm::vec2 origin(10.0f, 30.0f);
#endif

			const size_t total_joints = instance->animation_set.size() / ANIMATION_KEYFRAME_VALUES_MAX;

			// If you hit this, there are more joints than expected in this animation_set.
			assert(total_joints < MAX_BONES);

			float frame_delay_seconds = detail::_sequences[instance->sequence_index]->frame_delay_seconds;

			for (size_t bone_index = 0; bone_index < total_joints; ++bone_index)
			{
				animation::Channel* channel = &instance->channel_set[bone_index * ANIMATION_KEYFRAME_VALUES_MAX];

				const animation::Channel& tx = channel[0];
				const animation::Channel& ty = channel[1];
				const animation::Channel& tz = channel[2];
				glm::vec3& pos = pose.pos[bone_index];
				pos = glm::vec3(tx.evaluate(instance->local_time_seconds, frame_delay_seconds),
								ty.evaluate(instance->local_time_seconds, frame_delay_seconds),
								tz.evaluate(instance->local_time_seconds, frame_delay_seconds));

				const animation::Channel& rx = channel[3];
				const animation::Channel& ry = channel[4];
				const animation::Channel& rz = channel[5];
				const animation::Channel& rw = channel[6];
				glm::quat& rot = pose.rot[bone_index];
				rot = glm::quat(rw.evaluate(instance->local_time_seconds, frame_delay_seconds),
								rx.evaluate(instance->local_time_seconds, frame_delay_seconds),
								ry.evaluate(instance->local_time_seconds, frame_delay_seconds),
								rz.evaluate(instance->local_time_seconds, frame_delay_seconds));

#if defined(GEMINI_DEBUG_BONES)
				debugdraw::text(origin.x,
					origin.y + (12.0f * bone_index),
					core::str::format("%2i) '%s' | rot: [%2.2f, %2.2f, %2.2f, %2.2f]", bone_index,
						"",//mesh->skeleton[bone_index].name(),
						rot.x, rot.y, rot.z, rot.w),
					Color(0.0f, 0.0f, 0.0f));
#endif
			}
		} // animation_instance_get_pose


		void animation_interpolate_pose(Pose& out, Pose& last_pose, Pose& curr_pose, float t)
		{
#if 1
			for (size_t index = 0; index < MAX_BONES; ++index)
			{
				out.pos[index] = lerp(last_pose.pos[index], curr_pose.pos[index], t);
				out.rot[index] = slerp(last_pose.rot[index], curr_pose.rot[index], t);

				//				LOGV("out.rot[%i] = %2.2f, %2.2f, %2.2f, %2.2f | curr = %2.2f, %2.2f, %2.2f, %2.2f\n",
				//					 index,
				//					 out.rot[index].x, out.rot[index].y, out.rot[index].z, out.rot[index].w,
				//					 curr.rot[index].x, curr.rot[index].y, curr.rot[index].z, curr.rot[index].w
				//					 );
			}
#else
			out = curr_pose;
#endif
		} // animation_interpolate_pose

	} // namespace animation
} // namespace gemini
