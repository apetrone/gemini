// -------------------------------------------------------------
// Copyright (C) 2016- Adam Petrone
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//		* Redistributions of source code must retain the above copyright notice,
//		this list of conditions and the following disclaimer.

//		* Redistributions in binary form must reproduce the above copyright notice,
//		this list of conditions and the following disclaimer in the documentation
//		and/or other materials provided with the distribution.
//
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

#include <platform/audio.h>
#include <platform/platform.h>
#include "platform/backend/osx/cocoa_common.h"

#include <core/atomic.h>
#include <core/logging.h>
//
// REFERENCES
//
// http://kaniini.dereferenced.org/2014/08/31/CoreAudio-sucks.html
// https://github.com/jarikomppa/soloud/blob/master/src/backend/coreaudio/soloud_coreaudio.cpp
// https://gist.github.com/hngrhorace/1360885
// http://www.cocoawithlove.com/2010/10/ios-tone-generator-introduction-to.html
// http://atastypixel.com/blog/using-remoteio-audio-unit/
// https://developer.apple.com/library/mac/samplecode/HALExamples/Listings/ConfigDefaultOutput_c.html
// http://stackoverflow.com/questions/4575408/audioobjectgetpropertydata-to-get-a-list-of-input-devices

#if defined(GEMINI_ENABLE_AUDIO)

#include <AudioToolbox/AudioToolbox.h>

//#define GEMINI_COREAUDIO_PROPERTY_LISTENER 1

namespace platform
{
	struct coreaudio_state
	{
		Array<audio_device*> devices;
		AudioDeviceID previous_output_device;
		AudioQueueRef audio_queue;
		audio_sound_callback audio_pull_callback;
		void* context;
		uint32_t sample_rate_hz;
		uint32_t is_running;
		uint32_t buffer_size;
		uint32_t frame_size;
		platform::Thread* thread;

		coreaudio_state()
			: previous_output_device(0)
			, audio_queue(nullptr)
			, audio_pull_callback(nullptr)
			, context(nullptr)
			, sample_rate_hz(0)
			, is_running(0)
			, buffer_size(0)
			, frame_size(4)
			, thread(nullptr)
		{
		}
	};

	struct coreaudio_device
	{
		AudioDeviceID device_id;
	};

	coreaudio_state* audio_state = nullptr;

	void coreaudio_fill_buffer(void* user_data, AudioQueueRef audio_queue, AudioQueueBufferRef buffer)
	{
		coreaudio_state* state = reinterpret_cast<coreaudio_state*>(user_data);
		if (state->is_running)
		{
			assert(buffer->mAudioDataByteSize > 0);
			state->audio_pull_callback(buffer->mAudioData, (buffer->mAudioDataByteSize / state->frame_size), state->sample_rate_hz, state->context);

			OSStatus queue_status = AudioQueueEnqueueBuffer(audio_queue, buffer, 0, nil);
			if (queue_status == kAudioQueueErr_EnqueueDuringReset)
			{
				// System doesn't allow you to enqueue buffers during a:
				// AudioQueueReset, AudioQueueStop, or AudioQueueDispose operation.
			}
			else
			{
				assert(queue_status == 0);
			}
		}
	} // coreaudio_fill_buffer

#if defined(GEMINI_COREAUDIO_PROPERTY_LISTENER)
	void coreaudio_property_listener(void* /*user_data*/, AudioQueueRef audio_queue, AudioQueuePropertyID property)
	{
		if (property == kAudioQueueProperty_IsRunning)
		{
			uint32_t output;
			uint32_t data_size = sizeof(uint32_t);
			AudioQueueGetProperty(audio_queue, property, &output, &data_size);
			LOGV("audio queue is %s!\n", output ? "running" : "stopped");
		}
	} // coreaudio_property_listener
#endif

	void audio_thread(platform::Thread* thread)
	{
		coreaudio_state* state = reinterpret_cast<coreaudio_state*>(thread->user_data);
		while(state->is_running)
		{
			CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.10, false);
		}
	} // audio_thread

	platform::Result audio_enumerate_devices(Array<audio_device*>& devices)
	{
		devices = audio_state->devices;
		return platform::Result::success();
	} // audio_enumerate_devices

	platform::Result audio_open_output_device(audio_device* device)
	{
		assert(device->type == AudioDeviceType::Output);
		coreaudio_device* coreaudio_instance = reinterpret_cast<coreaudio_device*>(device + 1);

		AudioObjectPropertyAddress property_address = {
			kAudioHardwarePropertyDevices,
			kAudioObjectPropertyScopeGlobal,
			kAudioObjectPropertyElementMaster
		};

		UInt32 property_size = sizeof(CFStringRef);
		property_address.mSelector = kAudioObjectPropertyName;
		property_address.mScope = kAudioDevicePropertyScopeOutput;
		property_address.mElement = kAudioObjectPropertyElementMaster;

		OSStatus status = kAudioHardwareNoError;

		CFStringRef device_manufacturer;
		property_size = sizeof(CFStringRef);

		status = AudioObjectGetPropertyData(coreaudio_instance->device_id, &property_address, 0, nil, &property_size, &device_manufacturer);
		assert(status == kAudioHardwareNoError);

//		CFShow(device_manufacturer);

		CFRelease(device_manufacturer);

		property_address.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
		status = AudioObjectSetPropertyData(kAudioObjectSystemObject, &property_address, 0, nil, sizeof(AudioDeviceID), &coreaudio_instance->device_id);
		assert(status == kAudioHardwareNoError);

		// setup the audio queue
		const uint32_t total_channels = 2;
		audio_state->is_running = 0;
		audio_state->buffer_size = 2048;
		audio_state->frame_size = 4;
		audio_state->sample_rate_hz = 44100;

		AudioStreamBasicDescription format;
		format.mSampleRate = audio_state->sample_rate_hz;
		format.mFormatID = kAudioFormatLinearPCM;
		format.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
		// "In uncompressed audio, a Packet is
		// one frame, (mFramesPerPacket == 1)"
		format.mFramesPerPacket = 1;
		format.mBytesPerFrame = audio_state->frame_size;
		format.mChannelsPerFrame = total_channels;
		format.mBitsPerChannel = (format.mBytesPerFrame / 2) * 8;
		format.mBytesPerPacket = format.mBytesPerFrame * format.mFramesPerPacket;
		format.mReserved = 0;

		OSStatus result = AudioQueueNewOutput(
			&format,
			coreaudio_fill_buffer,
			audio_state,
			nil,
			kCFRunLoopCommonModes,
			0, // reserved: must be 0.
			&audio_state->audio_queue
		);

		if (result != kAudioHardwareNoError)
		{
			assert(!"Unable to create new audio queue");
		}

		// allocate and prime audio buffers
		for (size_t index = 0; index < 3; ++index)
		{
			AudioQueueBufferRef buffer;
			OSStatus buffer_result = AudioQueueAllocateBuffer(
				audio_state->audio_queue,
				audio_state->buffer_size,
				&buffer
			);
			assert(buffer_result == 0);

			buffer->mAudioDataByteSize = audio_state->buffer_size;
			memset(buffer->mAudioData, 0, buffer->mAudioDataByteSize);
			AudioQueueEnqueueBuffer(audio_state->audio_queue, buffer, 0, nil);
		}

#if defined(GEMINI_COREAUDIO_PROPERTY_LISTENER)
		// setup audio queue property listener
		AudioQueueAddPropertyListener(audio_state->audio_queue, kAudioQueueProperty_IsRunning, coreaudio_property_listener, &audio_state);
#endif

		OSStatus play_result = AudioQueueStart(audio_state->audio_queue, nil);
		assert(play_result == 0);
		audio_state->is_running = (play_result == kAudioHardwareNoError);

		// create the thread
		audio_state->thread = platform::thread_create(audio_thread, audio_state);

		return platform::Result::success();
	} // audio_open_output_device

	void audio_close_output_device()
	{
		audio_state->is_running = 0;
		platform::thread_join(audio_state->thread);
		platform::thread_destroy(audio_state->thread);
		audio_state->thread = nullptr;

		// shutdown
		AudioQueueStop(audio_state->audio_queue, true);
		AudioQueueDispose(audio_state->audio_queue, false);
		audio_state->audio_queue = nullptr;


		// get the device list
		AudioObjectPropertyAddress property_address = {
			kAudioHardwarePropertyDevices,
			kAudioObjectPropertyScopeGlobal,
			kAudioObjectPropertyElementMaster
		};

		UInt32 data_size;
		OSStatus status;

		// fetch the default output device id.
		property_address.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
		data_size = sizeof(AudioDeviceID);
		status = AudioObjectGetPropertyData(kAudioObjectSystemObject, &property_address, 0, nil, &data_size, &audio_state->previous_output_device);
		assert(status == kAudioHardwareNoError);
	} // audio_close_output_device

	platform::Result audio_startup()
	{
		audio_state = MEMORY_NEW(coreaudio_state, get_platform_allocator());

		// get the device list
		AudioObjectPropertyAddress property_address = {
			kAudioHardwarePropertyDevices,
			kAudioObjectPropertyScopeGlobal,
			kAudioObjectPropertyElementMaster
		};

		UInt32 data_size;
		OSStatus status;

		// fetch the default output device id.
		property_address.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
		AudioDeviceID default_audio_device;
		data_size = sizeof(AudioDeviceID);
		status = AudioObjectGetPropertyData(kAudioObjectSystemObject, &property_address, 0, nil, &data_size, &default_audio_device);
		assert(status == kAudioHardwareNoError);

		// cache old output device.
		audio_state->previous_output_device = default_audio_device;

		// get a list of all devices.
		property_address.mSelector = kAudioHardwarePropertyDevices;
		status = AudioObjectGetPropertyDataSize(
			kAudioObjectSystemObject,
			&property_address,
			0,
			nil,
			&data_size
		);

		if (status != kAudioHardwareNoError)
		{
			return platform::Result::failure("Unable to fetch audio device list size");
		}

		// find out how many devices are on the system
		UInt32 device_count = static_cast<UInt32>(data_size / sizeof(AudioDeviceID));
		AudioDeviceID* devices = static_cast<AudioDeviceID*>(calloc(data_size, sizeof(AudioDeviceID)));
		if (devices == nullptr)
		{
			return platform::Result::failure("Could not query number of audio devices");
		}

		status = AudioObjectGetPropertyData(
			kAudioObjectSystemObject,
			&property_address,
			0,
			nil,
			&data_size,
			devices
		);

		if (status != kAudioHardwareNoError)
		{
			return platform::Result::failure("Failed to fetch audio devices");
		}

		audio_state->devices.resize(device_count);

		CFStringRef device_name;
		for (uint32_t index = 0; index < device_count; ++index)
		{
			// create a new instance
			const size_t requested_bytes = sizeof(audio_device) + sizeof(coreaudio_device);
			char* memory = static_cast<char*>(MEMORY_ALLOC(requested_bytes, get_platform_allocator()));
			assert(memory);
			audio_device* device = new (memory) audio_device();
			coreaudio_device* coreaudio_instance = new (memory + sizeof(audio_device)) coreaudio_device();
			audio_state->devices[index] = device;
			coreaudio_instance->device_id = devices[index];

			UInt32 property_size = sizeof(CFStringRef);
			property_address.mSelector = kAudioObjectPropertyName;
			property_address.mScope = kAudioDevicePropertyScopeOutput;
			property_address.mElement = kAudioObjectPropertyElementMaster;

			status = AudioObjectGetPropertyData(devices[index], &property_address, 0, nil, &property_size, &device_name);
			assert(status == kAudioHardwareNoError);

			bool name_conversion_result = platform::cocoa::cfstringref_to_buffer(device_name, &device->name[0], device->name.max_size());
			assert(name_conversion_result == true);

			CFRelease(device_name);

			property_address.mSelector = kAudioDevicePropertyStreamConfiguration;
			status = AudioObjectGetPropertyDataSize(devices[index], &property_address, 0, nil, &property_size);
			if (status != kAudioHardwareNoError)
			{
				LOGW("Querying DevicePropertyStreamConfiguration failed\n");
				continue;
			}

			AudioBufferList* audio_buffers = static_cast<AudioBufferList*>(malloc(data_size));
			assert(audio_buffers);

			status = AudioObjectGetPropertyData(devices[index], &property_address, 0, nil, &property_size, audio_buffers);
			if (status != kAudioHardwareNoError || audio_buffers->mNumberBuffers == 0)
			{
				free(audio_buffers);
				audio_buffers = nullptr;
				continue;
			}

			free(audio_buffers);

			property_address.mSelector = kAudioDevicePropertyStreams;
			status = AudioObjectGetPropertyDataSize(devices[index], &property_address, 0, nil, &property_size);
			UInt32 total_streams = (property_size / sizeof(AudioStreamID));
			if (total_streams > 0)
			{
				device->type = AudioDeviceType::Output;
			}
			else
			{
				device->type = AudioDeviceType::Input;
			}

			property_address.mSelector = kAudioDevicePropertyDeviceUID;
			CFStringRef deviceuid;
			property_size = sizeof(CFStringRef);
			status = AudioObjectGetPropertyData(devices[index], &property_address, 0, nil, &property_size, &deviceuid);
			if (status == kAudioHardwareNoError)
			{
//				CFShow(deviceuid);
			}

			if (devices[index] == default_audio_device)
			{
				audio_state->devices.swap(index, 0);
			}
		}
		
		free(devices);

		return platform::Result::success();
	} // audio_startup

	void audio_shutdown()
	{
		assert(audio_state->thread == nullptr);

		for (size_t index = 0; index < audio_state->devices.size(); ++index)
		{
			audio_device* device = audio_state->devices[index];
			MEMORY_DEALLOC(device, get_platform_allocator());
		}
		MEMORY_DELETE(audio_state, get_platform_allocator());
	} // audio_shutdown

	void audio_set_callback(audio_sound_callback callback, void* context)
	{
		// If you hit this, the audio device wasn't opened before this call.
		assert(audio_state);

		audio_state->audio_pull_callback = callback;
		audio_state->context = context;
	} // audio_set_callback

	uint64_t audio_frame_position()
	{
		AudioTimeStamp timestamp;
		AudioQueueGetCurrentTime(audio_state->audio_queue, nil, &timestamp, nil);
		return static_cast<uint64_t>(timestamp.mSampleTime);
	} // audio_frame_position
} // namespace platform

#endif // GEMINI_ENABLE_AUDIO
