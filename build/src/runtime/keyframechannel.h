// -------------------------------------------------------------
// Copyright (C) 2004- Adam Petrone
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
#include <core/logging.h>

#include <renderer/render_utilities.h>

#include <core/typedefs.h>

#include <cmath> // for isnan

namespace gemini
{
	template <class Type>
	struct KeyframeData
	{
		FixedArray<Type> keys;
		FixedArray<float> time; // the time, in seconds, for this keyframe
	};
	// -------------------------------------------------------------
	template <class Type, class Interpolator=Interpolator<Type> >
	class Channel
	{
		Type& value;
		KeyframeData<Type>* data_source;
		static Interpolator interpolator;
		uint16_t current_frame;
		float current_time_seconds;
		float next_advance_timeleft;
		float frame_delay_seconds;

	public:
		Channel(Type& value_in, KeyframeData<Type>* source = 0) :
			value(value_in), data_source(source)
		{
			current_frame = 0;
			current_time_seconds = 0;
			next_advance_timeleft = 0;
			frame_delay_seconds = 0;
		}
		~Channel() {}

		void set_data_source(KeyframeData<Type>* datasource, float frame_delay)
		{
			data_source = datasource;
			assert(data_source != 0);

			frame_delay_seconds = frame_delay;
		}

		KeyframeData<Type>* get_data_source() { return data_source; }
		float get_frame_delay() const { return frame_delay_seconds; }

		void update_value(uint32_t frame, float alpha)
		{
			alpha = glm::clamp(alpha, 0.0f, 1.0f);

			if (!data_source || data_source->keys.empty())
			{
				return;
			}

			frame = clamp_frame(frame);

			assert(data_source && data_source->keys.size() > 0);

			const Type& last = data_source->keys[frame];
			Type next;
			if ((frame+1) >= data_source->keys.size())
			{
				// TODO: Should use post-infinity here
				// For now, just use the max.
				next = data_source->keys[frame];
			}
			else
			{
				next = data_source->keys[frame+1];
			}

			// interpolate between frame and frame+1
			value = Channel<Type, Interpolator>::interpolator(last, next, alpha);

	//		assert(!isnan(value.x));
	//		assert(!isnan(value.y));
	//		assert(!isnan(value.z));
		}

		void update_with_time2(float delta_seconds)
		{
			// determine the next frame; don't let this wrap
			uint16_t next_frame = clamp_frame(current_frame+1);

			float t0 = data_source->time[current_frame];
			float t1 = data_source->time[next_frame];

			float start_time = (current_frame * frame_delay_seconds);


			float alpha = (current_time_seconds - start_time) / (t1 - t0);
	//		LOGV("alpha: %g\n", alpha);

			assert(!isnan(alpha));
			update_value(current_frame, alpha);


			if (current_time_seconds >= t1 || (next_frame >= data_source->keys.size()-1))
			{
				// advance the frame
				++current_frame;

				// catch out of frame bounds
				if ((current_frame >= data_source->keys.size()))
				{
					current_frame = 0;
					current_time_seconds = 0;
				}

	//			LOGV("frame: %i\n", current_frame);
			}
		}

		void update_with_time(float delta_seconds)
		{
			next_advance_timeleft -= delta_seconds;

			// determine the next frame; don't let this wrap
			uint16_t next_frame = clamp_frame((current_frame+1));

			float last_keyframe_time = data_source->time[current_frame];
			float next_time = data_source->time[next_frame];
			assert(next_time != 0.0f);

			// alpha is calculated by dividing the deltas: (a/b)
			// a. The delta between the current simulation time and the last key frame's time
			// b. The delta between the next key frame's time and the last key frame's time.
			float alpha = (next_time - current_time_seconds)/(next_time - last_keyframe_time);
			alpha = glm::clamp(alpha, 0.0f, 1.0f);


			alpha = 1.0f;
			update_value(current_frame, alpha);

			if (current_time_seconds >= next_time)
			{
				// advance the frame
				++current_frame;

				// catch out of frame bounds
				if (current_frame == data_source->keys.size()-1)
				{
					current_frame = 0;
					current_time_seconds -= next_time;
				}

	//			LOGV("frame: %i\n", current_frame);
			}
		}

		void update_sampled(float delta_seconds)
		{
			next_advance_timeleft -= delta_seconds;

			float start_time = (current_frame * frame_delay_seconds);
			float t = (current_time_seconds - start_time) / frame_delay_seconds;

			t = glm::clamp(t, 0.0f, 1.0f);

			update_value(current_frame, t);

			if (next_advance_timeleft <= 0.01f)
			{
				// advance the frame
				++current_frame;

				next_advance_timeleft = frame_delay_seconds;

				// catch out of frame bounds
				if (current_frame == data_source->keys.size()-1)
				{
					current_frame = 0;
					current_time_seconds = 0;
				}
			}
		}

		void update(float delta_seconds)
		{
			if (!data_source)
			{
				return;
			}



			update_with_time(delta_seconds);

	//		update_with_time2(delta_seconds);

	//		update_sampled(delta_seconds);

			current_time_seconds += delta_seconds;
		}

	private:
		uint32_t clamp_frame(uint32_t frame)
		{
			assert(data_source != 0);
			if (frame > data_source->keys.size())
			{
				frame = data_source->keys.size();
			}
			return frame;
		}
	};

	template <class Type, class Interpolator>
	Interpolator Channel<Type, Interpolator>::interpolator;


	template <class Type, class Interpolator=Interpolator<Type> >
	struct KeyframeChannel
	{
		unsigned int total_samples;
		float frame_delay;
		Type pre_infinity;
		Type post_infinity;
		Type * samples;
		static Interpolator interpolator;

		KeyframeChannel()
		{
			samples = 0;
			total_samples = 0;
		}

		~KeyframeChannel()
		{
			if ( samples )
			{
				MEMORY_DELETE_ARRAY(samples, core::memory::global_allocator());
				samples = 0;
			}

			total_samples = 0;
		}

		void create( unsigned int max_samples, const Type * values, float framedelay, Type pre_infinity_value = Type(), Type post_infinity_value = Type() )
		{
			total_samples = max_samples;
			samples = MEMORY_NEW_ARRAY(Type, total_samples, core::memory::global_allocator());


			set_keys( values, total_samples );

			pre_infinity = samples[0];
			post_infinity = samples[ total_samples-1 ];

			//_frametime = 1.0 / (float)(total_samples-1);
			frame_delay = framedelay;
		}

		void set_keys( const Type * ptr, unsigned int num_samples, unsigned int offset = 0 )
		{
			unsigned int samples_to_copy = num_samples;
			if ( samples_to_copy > total_samples )
				samples_to_copy = total_samples;

			memcpy( &samples[offset], ptr, samples_to_copy*sizeof(Type) );
		}

		void set_keyframe( unsigned int index, const Type & value )
		{
			if ( index < total_samples )
			{
				samples[ index ] = value;
			}
		}

		const Type get_value( float t )
		{
			if (t < 0)
			{
				return pre_infinity;
			}
			else if (t > 1.0)
			{
				return post_infinity;
			}

			// find the index of the sample, roughly matching with t
			//		int index = (t * ((float)_total_samples-1));
			unsigned int index = (t / frame_delay);

			// calculate the start time of the frame at index
			float start_time = (index * frame_delay);

			// calculate [0,1] how far between index and index+1 keyframes
			float n = (t - start_time) / frame_delay;

			if (index >= (total_samples-1))
			{
				return post_infinity;
			}

			// now just interpolate using n
			return interpolator(samples[index], samples[index+1], n);
		}
	}; // KeyframeChannel

	template <class Type, class Interpolator >
	Interpolator KeyframeChannel<Type, Interpolator>::interpolator;

	template <class Type>
	struct Keyframe
	{
		float seconds;
		Type value;
	};

	template <class Type>
	struct KeyChannel
	{
		FixedArray< Keyframe<Type> > keys;
		float length_seconds;

		KeyChannel() :
			length_seconds(0.0f)
		{
		}

		Type value_at_time(float time_seconds)
		{
//			assert(!keys.empty());
			if (keys.empty())
			{
				return Type();
			}

			// 1. for an arbitrary time value t,
			//		map it to a channel's keyframe time length bounds.
			// 		such that (0 >= t < length_seconds).
			Type value;

			float last_key_seconds = keys[keys.size()-1].seconds;

			while(time_seconds > last_key_seconds)
				time_seconds -= last_key_seconds;

			assert(time_seconds <= last_key_seconds);

			// 2. Find the keyframes that bound t.
			Keyframe<Type>* prev = 0;
			for (size_t key_index = 0; key_index < keys.size(); ++key_index)
			{
				Keyframe<Type>& key = keys[key_index];
				if (time_seconds > key.seconds)
				{
					prev = &key;
					continue;
				}


				float alpha;
				Type last_value;
				if (prev)
				{
					float dt = key.seconds - prev->seconds;
					last_value = prev->value;
					alpha = (time_seconds-prev->seconds)/dt;
				}
				else
				{
					last_value = key.value;
					alpha = time_seconds;
				}

				// 3. lerp the value
				value = Interpolator<Type>()(last_value, key.value, alpha);
//				value = key.value;
				break;
			}

			return value;
		}
	};

	template <class Type>
	struct KeyframeChannelReference
	{
		float& target;
		const KeyChannel<Type>& channel;
		float local_time;

		KeyframeChannelReference(const KeyChannel<Type>& input_channel, float& target_value) :
			channel(input_channel),
			target(target_value)
		{}
	};
} // namespace gemini
