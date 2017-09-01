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

#include <core/freelist.h>
#include <core/mem.h>
#include <core/linearfreelist.h>
#include <core/logging.h>
#include <core/mathlib.h>
#include <core/stackstring.h>

#include <platform/platform.h>

#include <hashset.h>

#include <runtime/configloader.h>
#include <runtime/mesh.h>
#include <runtime/assets.h>

#include <runtime/transform_graph.h>

#include <renderer/debug_draw.h>
#include <renderer/scene_renderer.h>

#include <vector>

using namespace platform;

#define GEMINI_INTERPOLATE_KEYFRAMES 1

namespace gemini
{
	namespace animation
	{
		typedef Freelist<Sequence*> SequenceFreelist;
		typedef Freelist<AnimatedInstance*> AnimatedInstanceFreelist;
		typedef Array<AnimationController> AnimationControllerArray;

		struct AnimationState
		{
			Allocator* allocator;
			SequenceFreelist sequences;
			AnimatedInstanceFreelist instances;
			AnimationControllerArray controllers;

			AnimationState(Allocator& in_allocator)
				: allocator(&in_allocator)
				, sequences(in_allocator)
				, instances(in_allocator)
				, controllers(in_allocator)
			{
			}
		};

		AnimationState* _animation_state = nullptr;



		//
		// KeyframeList
		//

		template <class T>
		KeyframeList<T>::KeyframeList(gemini::Allocator& _allocator)
			: allocator(_allocator)
			, keys(nullptr)
			, total_keys(0)
			, duration_seconds(0.0f)
		{
		}

		template <class T>
		KeyframeList<T>::~KeyframeList()
		{
			deallocate();
		}

		template <class T>
		void KeyframeList<T>::allocate(size_t key_count)
		{
			total_keys = static_cast<uint32_t>(key_count);
			keys = MEMORY2_NEW_ARRAY(Keyframe<T>, allocator, total_keys);
		}

		template <class T>
		void KeyframeList<T>::deallocate()
		{
			MEMORY2_DELETE_ARRAY(allocator, keys);
			total_keys = 0;
		}

		template <class T>
		void KeyframeList<T>::set_key(const size_t index, const float seconds, const T& value)
		{
			keys[index].seconds = seconds;
			keys[index].value = value;
		}

		//
		// Channel
		//

		template <class T>
		Channel<T>::Channel(float* target, bool should_wrap)
		{
			keyframelist = 0;
			wrap = should_wrap;
		}

		template <class T>
		Channel<T>::~Channel()
		{
		}

		template <class T>
		void Channel<T>::set_keyframe_list(KeyframeList<T>* source_keyframe_list)
		{
			keyframelist = source_keyframe_list;
		}

		template <class T>
		T Channel<T>::evaluate(float t_seconds, float frame_delay_seconds, bool looping) const
		{
			if (keyframelist->duration_seconds == 0)
			{
				return T();
			}

			uint32_t last_key = (keyframelist->total_keys - 1);

			// TODO: Perform a direct lookup into the array with some math.
			// this hasn't been a performance problem yet, fix it when it comes up.
			float delta;
			T value_a;
			T value_b;
			for (uint32_t key = 0; key < keyframelist->total_keys; ++key)
			{
				// alpha is calculated by dividing the deltas: (a/b)
				// a. The delta between the current simulation time and the last key frame's time
				// b. The delta between the next key frame's time and the last key frame's time.
				gemini::animation::Keyframe<T>* keyframe = &keyframelist->keys[key];
				uint32_t next_key = key + 1;
				if (t_seconds < keyframe->seconds)
				{
					//if (key == 0)
					//{
					//	// can't get previous; lerp forward
					//	gemini::animation::Keyframe<T>* next = &keyframelist->keys[next_key];
					//	delta = (next->seconds - keyframe->seconds);
					//	value_a = next->value;
					//	value_b = keyframe->value;
					//	return gemini::interpolate(value_a, value_b, (delta / frame_delay_seconds));
					//}
					//else
					{
						// This assumes that the animation is evenly sampled
						// across key frames by frame_delay_seconds.
						// If it isn't, we could use
						// (keyframe->seconds - prev_keyframe->seconds) as
						// the denominator instead of frame_delay_seconds.
						gemini::animation::Keyframe<T>* prev_keyframe = &keyframelist->keys[key - 1];
#if GEMINI_INTERPOLATE_KEYFRAMES
						float alpha = (t_seconds - prev_keyframe->seconds) / frame_delay_seconds;
						return gemini::interpolate(prev_keyframe->value, keyframe->value, alpha);
#else
						return prev_keyframe->value;
#endif
					}
				}
				else if (last_key == key)
				{
					// if the animation loops, we must do this lerp.
					// next key would wrap: We may just be able to
					// return the last/first value.
					if (looping)
					{

						gemini::animation::Keyframe<T>* last_keyframe = &keyframelist->keys[last_key];
#if GEMINI_INTERPOLATE_KEYFRAMES
						gemini::animation::Keyframe<T>* first_keyframe = &keyframelist->keys[0];
						return gemini::interpolate(last_keyframe->value, first_keyframe->value, ((t_seconds - last_keyframe->seconds) / frame_delay_seconds));
#else
						return last_keyframe->value;
#endif
					}

					// if the animation doesn't loop. this is fine.
					return keyframe->value;
				}
			}

			return T();
		} // evaluate

		// Sequence
		Sequence::Sequence(gemini::Allocator& _allocator)
			: allocator(_allocator)
			, translations(allocator)
			, rotations(allocator)
		{
		}

		// AnimatedInstance
		AnimatedInstance::AnimatedInstance(gemini::Allocator& allocator)
			: local_time_seconds(0.0)
			, flags(Flags::Idle)
			, translation_channel(allocator)
			, rotation_channel(allocator)
		{
		}

		AnimatedInstance::~AnimatedInstance()
		{
			translation_channel.clear();
			rotation_channel.clear();
		}

		void AnimatedInstance::initialize(Sequence* sequence)
		{
			sequence_index = sequence->index;

			// reserve enough space
			translation_channel.allocate(sequence->translations.size());
			for (size_t bone_index = 0; bone_index < sequence->translations.size(); ++bone_index)
			{
				Channel<glm::vec3>& channel = translation_channel[bone_index];
				channel.set_keyframe_list(&sequence->translations[bone_index]);
			}

			rotation_channel.allocate(sequence->rotations.size());
			for (size_t bone_index = 0; bone_index < sequence->rotations.size(); ++bone_index)
			{
				Channel<glm::quat>& channel = rotation_channel[bone_index];
				channel.set_keyframe_list(&sequence->rotations[bone_index]);
			}
		}

		void AnimatedInstance::advance(float delta_seconds)
		{
			local_time_seconds += delta_seconds;

			Sequence* sequence = animation::get_sequence_by_index(sequence_index);
			assert(sequence != 0);

			if (local_time_seconds > sequence->duration_seconds)
			{
				if (sequence->looping)
				{
					local_time_seconds = 0;
				}
				flags = Flags::Finished;
			}
		}

		bool AnimatedInstance::is_playing() const
		{
			return flags == Flags::Playing;
		}

		bool AnimatedInstance::is_finished() const
		{
			return flags == Flags::Finished;
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

				const Json::Value& total_frames = root["total_frames"];

				std::string animation_title = animation_name.asString();
				LOGV("animation: \"%s\"\n", animation_title.c_str());

				int32_t fps_rate = frames_per_second.asInt();
				sequence->frame_delay_seconds = (1.0f/(float)fps_rate);

				// Don't read this from ASCII float -- data loss will result.
				// Instead, compute this here.
				if (validate_node(total_frames, "total_frames"))
				{
					sequence->duration_seconds = total_frames.asInt() / (float)fps_rate;
				}
				else
				{
					LOGW("LEGACY WARNING. This animation should be re-exported to correct duration_seconds.\n");
					sequence->duration_seconds = duration_seconds.asFloat();
				}

				LOGV("frames_per_second = %i (frame delay = %2.2f)\n", fps_rate, sequence->frame_delay_seconds);

				// 1. allocate enough space for each bone
				sequence->translations.allocate(bones_array.size(), KeyframeList<glm::vec3>(sequence->allocator));
				sequence->rotations.allocate(bones_array.size(), KeyframeList<glm::quat>(sequence->allocator));

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

					// When reading the values for each block, we'll keep an
					// increment of the physical time value so it isn't read
					// out of the JSON file and rounded.

					// translation
					{
//						LOGV("translation keyframes\n");
						const Json::Value& tr_values = translation_keys["value"];

						assert(!tr_values.isNull());

						// In Json, there are EQUAL entries for each scale/rotation/translation tracks.
						// This MAY NOT be like this in other formats -- so revisit this later.
						int total_keys = tr_values.size();

						KeyframeList<glm::vec3>* translation = &sequence->translations[joint->index];
						translation->allocate(tr_values.size());
						translation->duration_seconds = duration_seconds.asFloat();

						for (unsigned int index = 0; index < tr_values.size(); ++index)
						{
							const Json::Value& value = tr_values[index];

							float x = value[0].asFloat();
							float y = value[1].asFloat();
							float z = value[2].asFloat();

							const float time_seconds = (sequence->frame_delay_seconds * index);

							translation->set_key(index, time_seconds, glm::vec3(x, y, z));
//							LOGV("t=%2.2f, %2.2f %2.2f %2.2f\n", t, x, y, z);
						}
					}

					// rotation
					{
//						LOGV("rotation keyframes\n");
						const Json::Value& values = rotation_keys["value"];

						assert(!values.isNull());

						// In Json, there are EQUAL entries for each scale/rotation/translation tracks.
						// This MAY NOT be like this in other formats -- so revisit this later.
						int total_keys = values.size();

						KeyframeList<glm::quat>* rotation = &sequence->rotations[joint->index];
						rotation->allocate(values.size());
						rotation->duration_seconds = duration_seconds.asFloat();

						for (unsigned int index = 0; index < values.size(); ++index)
						{
							const Json::Value& value = values[index];

							float x = value[0].asFloat();
							float y = value[1].asFloat();
							float z = value[2].asFloat();
							float w = value[3].asFloat();

							const float time_seconds = (sequence->frame_delay_seconds * index);

							rotation->set_key(index, time_seconds, glm::quat(w, x, y, z));
//							LOGV("t=%2.2f, %2.2f %2.2f %2.2f %2.2f\n", t, x, y, z, w);
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
			sequence->index = -1;
			sequence->looping = 0;
			platform::PathString filepath = name;
			filepath.append(".animation");

			AnimationSequenceLoadData data;
			data.mesh = mesh;
			data.sequence = sequence;
			sequence->name = name;
			LOGV("loading animation \"%s\"\n", filepath());

			if (core::util::ConfigLoad_Success == core::util::json_load_with_callback(filepath(), detail::load_animation_from_json, &data, true))
			{
				sequence->index = _animation_state->sequences.acquire();
				_sequences_by_name->insert(SequenceHash::value_type(name, sequence));
				_animation_state->sequences.set(sequence->index, sequence);
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
			for (size_t index = 0; index < _animation_state->sequences.size(); ++index)
			{
				Sequence* instance = _animation_state->sequences.at(index);
				MEMORY2_DELETE(*_allocator, instance);
			}

			for (size_t index = 0; index < _animation_state->instances.size(); ++index)
			{
				AnimatedInstance* instance = _animation_state->instances.at(index);
				MEMORY2_DELETE(*_allocator, instance);
			}

			MEMORY2_DELETE(*_allocator, _sequences_by_name);
			MEMORY2_DELETE(*_animation_state->allocator, _animation_state);
		}

		void update(float delta_seconds)
		{
			AnimatedInstanceFreelist::Iterator anim = _animation_state->instances.begin();
			for (; anim != _animation_state->instances.end(); ++anim)
			{
				AnimatedInstance* instance = anim.data();
				if (instance->is_playing())
				{
					instance->advance(delta_seconds);
				}
			}
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
			return _animation_state->sequences.from_handle(index);
		}

		AnimatedInstance* create_sequence_instance(gemini::Allocator& allocator)
		{
			AnimatedInstance* instance = MEMORY2_NEW(allocator, AnimatedInstance)(allocator);
			instance->index = _animation_state->instances.acquire();
			instance->local_time_seconds = 0.0f;
			instance->sequence_index = -1;
			instance->flags = AnimatedInstance::Flags::Idle;
			_animation_state->instances.set(instance->index, instance);
			return instance;
		}

		void destroy_sequence_instance(gemini::Allocator& allocator, AnimatedInstance* instance)
		{
			_animation_state->instances.release(instance->index);
		}

		AnimatedInstance* get_instance_by_index(SequenceId index)
		{
			return _animation_state->instances.from_handle(index);
		}

//#define GEMINI_DEBUG_BONES 1

		float animated_instance_get_local_time(AnimatedInstance* instance)
		{
			return instance->local_time_seconds;
		}

		void animated_instance_get_pose(AnimatedInstance* instance, Pose& pose)
		{
#if defined(GEMINI_DEBUG_BONES)
			const glm::vec2 origin(10.0f, 30.0f);
#endif
			if (instance->sequence_index < 0)
			{
				return;
			}

			const size_t total_joints = instance->rotation_channel.size();

			// If you hit this, there are more joints than expected in this animation_set.
			assert(total_joints < MAX_BONES);

			Sequence* sequence = _animation_state->sequences.from_handle(instance->sequence_index);
			assert(sequence);
			float frame_delay_seconds = sequence->frame_delay_seconds;


			for (size_t bone_index = 0; bone_index < total_joints; ++bone_index)
			{
				animation::Channel<glm::vec3>* translation_channel = &instance->translation_channel[bone_index];
				animation::Channel<glm::quat>* rotation_channel = &instance->rotation_channel[bone_index];
				pose.pos[bone_index] = translation_channel->evaluate(instance->local_time_seconds, frame_delay_seconds, sequence->looping);
				pose.rot[bone_index] = rotation_channel->evaluate(instance->local_time_seconds, frame_delay_seconds, sequence->looping);

#if defined(GEMINI_DEBUG_BONES)
				debugdraw::text(origin.x,
					origin.y + (12.0f * bone_index),
					core::str::format("%2i) '%s' | rot: [%2.2f, %2.2f, %2.2f, %2.2f]", bone_index,
						"",//mesh->skeleton[bone_index].name(),
						pose.rot[bone_index].x, pose.rot[bone_index].y, pose.rot[bone_index].z, pose.rot[bone_index].w),
					Color(0.0f, 0.0f, 0.0f));
#endif
			}
		} // animation_instance_get_pose

	} // namespace animation
} // namespace gemini


namespace gemini
{
	void animation_controller_transfer(AnimationController* controller)
	{
		// AnimatedMeshComponent -> TransformNode

		// grab current pose from component; transfer to target.
		Mesh* mesh = mesh_from_handle(controller->component->mesh_handle);
		if (!mesh)
		{
			LOGW("Unable to get mesh to AnimationControlled component %p\n", controller->component);
			return;
		}

		// TODO: If there was root animation on this controller; it could
		// apply it to the animated_node or one of its children.

		animation::Pose pose;
		// get aggregate pose from component
		{
			AnimatedMeshComponent* component = controller->component;

			float blend_alpha = 0.5f;
			animation::Pose poses[MAX_ANIMATED_MESH_LAYERS];

			for (size_t layer_index = 0; layer_index < MAX_ANIMATED_MESH_LAYERS; ++layer_index)
			{
				animation::Pose* current_pose = &poses[layer_index];

				animation::AnimatedInstance* instance = component->sequence_instances[layer_index];
				assert(instance);

				animated_instance_get_pose(instance, *current_pose);
			}

			uint64_t bone_mask = 0;

			TransformNode* animated_node = controller->target;
			for (size_t index = 0; index < mesh->skeleton.size(); ++index)
			{
				// blend the poses
				if (bone_mask & index)
				{
					pose.rot[index] = gemini::interpolate(poses[0].rot[index], poses[1].rot[index], blend_alpha);
					pose.pos[index] = gemini::interpolate(poses[0].pos[index], poses[1].pos[index], blend_alpha);
				}
				else
				{
					pose.rot[index] = poses[0].rot[index];
					pose.pos[index] = poses[0].pos[index];
				}

				const Joint* joint = &mesh->skeleton[index];

				TransformNode* child = animated_node->bones[index];
				child->position = pose.pos[index];
				child->orientation = pose.rot[index];
				child->bind_pose_matrix = mesh->bind_poses[index];
			}
		}
	}

	void animation_controller_extract(AnimationController* controller)
	{
		// TransformNode -> AnimatedMeshComponent
		TransformNode* animated_node = controller->target;
		AnimatedMeshComponent* component = controller->component;

		Mesh* mesh = mesh_from_handle(controller->component->mesh_handle);
		if (!mesh)
		{
			LOGW("Unable to get mesh to AnimationControlled component %p\n", controller->component);
			return;
		}

		const size_t total_children = animated_node->bones.size();
		for (size_t index = 0; index < total_children; ++index)
		{
			TransformNode* child = animated_node->bones[index];
			// These are WORLD TRANSFORMS
			// which actually breaks the rest of our matrix evaluation because
			// we assume the AnimatedMeshComponent's model_matrix has to be
			// post-multiplied with each bone transform.
			//component->bone_transforms[index] = child->world_matrix;
			component->bone_transforms[index] = child->model_bone_matrix;
		}
	}

	void animation_link_transform_and_component(TransformNode* node, AnimatedMeshComponent* component)
	{
		AnimationController controller;
		controller.target = node;
		controller.component = component;
		animation::_animation_state->controllers.push_back(controller);
	} // animation_link_transform_and_component


	void animation_update_transform_nodes()
	{
		// transfer animation from active sequences to their transform nodes
		for (AnimationController& controller : animation::_animation_state->controllers)
		{
			animation_controller_transfer(&controller);
		}
	} // animation_update_transform_nodes

	void animation_update_components()
	{

		// extract world transforms from bone nodes
		// and copy them to the AnimatedMeshComponent for rendering
		for (AnimationController& controller : animation::_animation_state->controllers)
		{
			animation_controller_extract(&controller);
		}
	} // animation_update_components
} // namespace gemini