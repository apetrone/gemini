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

// Why WASAPI?:
// - a. With XAudio2, there are three different versions for Windows 7, 8, and 10.
//		Headers that ship with Windows 8.1 kit (which can be installed
//		on Windows 7 via VS2015, link to XAudio2_8.dll by default.
//		This requires that the application load the XAudio2_7.dll at
//		runtime and dynamically link the functions.
// - b. This leads down the rabbit hole of having to duplicate what the
//		June 2010 DX SDK headers do for XAudio2Create
//		via COM, initializing the instance, etc.
// - c. XAudio2 cannot capture audio from devices -- such as a Microphone.
// - d. WASAPI still leaves the door open for Windows Store and
//		Phone Apps (in case that ever needs to be a thing).

// Therefore, I'm going to attempt to use the WASAPI which debuted with
// Windows Vista. If there's ever a need to support Windows XP, I can hook
// up DirectSound later on.

//
// REFERENCES
//
// Sample rate conversion is not performed inside WASAPI.
// Exclusive mode vs Shared mode with the device. Exclusive is a direct
// connection -- no other applications can use the device.
// http://mark-dot-net.blogspot.com/2008/06/what-up-with-wasapi.html

// Need to add in a bit of latency when writing audio
// https://hero.handmadedev.org/forum/code-discussion/442-using-wasapi-for-sound#2902

// REFERENCE_TIME is in hundreds of nanoseconds.
// https://blogs.windows.com/buildingapps/2014/05/15/real-time-audio-in-windows-store-and-windows-phone-apps/

// https://msdn.microsoft.com/en-us/library/windows/desktop/dd316756(v=vs.85).aspx

#include <platform/audio.h>
#include <platform/platform_internal.h>

#include <core/atomic.h>
#include <core/logging.h>

#pragma warning(push)
// Cleanup after messy windows headers.
#pragma warning(disable: 4917) // a GUID can only be associated with a class, interface or namespace
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <AudioSessionTypes.h>
#include <functiondiscoverykeys_devpkey.h> // for endpoint device property keys
#pragma warning(pop)

#include <objbase.h>

template <class T>
void safe_release(T** ptr)
{
	if (*ptr)
	{
		(*ptr)->Release();
		*ptr = nullptr;
	}
}

// See this for obtaining number of channels for a device:
// https://msdn.microsoft.com/en-us/library/windows/desktop/dd368227(v=vs.85).aspx

#if defined(GEMINI_ENABLE_AUDIO)

namespace platform
{
	struct win32_audio_device
	{
		IMMDevice* endpoint;
	};

	// state we need for audio.
	struct win32_wasapi_state
	{
		uint32_t sample_rate_hz;
		uint32_t buffer_frame_count;
		audio_sound_callback audio_pull_callback;
		void* context;
		uint64_t last_frame_count;
		float audio_clock_frequency_denominator;
		Array<audio_device*, PlatformAllocatorType> devices;

		IAudioClient* audio_client;
		IAudioRenderClient* render_client;
		IAudioClock* audio_clock;
		HANDLE audio_event_handle;
		HANDLE submission_event_handle;
		HANDLE submission_thread_handle;
		HANDLE submission_exited_thread;

		win32_wasapi_state()
			: devices(16, get_platform_allocator())
		{
			sample_rate_hz = 0;
			buffer_frame_count = 0;
			audio_pull_callback = nullptr;
			context = nullptr;
			last_frame_count = 0;
			audio_clock_frequency_denominator = 0.0f;

			audio_client = nullptr;
			render_client = nullptr;
			audio_clock = nullptr;
			audio_event_handle = INVALID_HANDLE_VALUE;
			submission_event_handle = INVALID_HANDLE_VALUE;
			submission_thread_handle = INVALID_HANDLE_VALUE;
			submission_exited_thread = INVALID_HANDLE_VALUE;
		}
	};

	win32_wasapi_state* audio_state;

	static HRESULT check_wasapi_error(HRESULT input)
	{
		switch (input)
		{
			// No error.
		case S_OK:
			break;

		case AUDCLNT_E_ALREADY_INITIALIZED:
			LOGV("AudioClient is already initialized\n");
			break;

		case AUDCLNT_E_BUFFER_ERROR:
			LOGV("GetBuffer failed to retrieve a data buffer\n");
			break;

		case AUDCLNT_E_WRONG_ENDPOINT_TYPE:
			LOGV("Wrong Endpoint Type\n");
			break;

		case AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED:
			LOGV("BufferSize is not aligned\n");
			break;

		case AUDCLNT_E_BUFFER_SIZE_ERROR:
			LOGV("Buffer duration for exclusive mode is out of range\n");
			break;

		case AUDCLNT_E_UNSUPPORTED_FORMAT:
			LOGV("Unsupported format\n");
			break;

		case AUDCLNT_E_BUFFER_TOO_LARGE:
			LOGV("NumFramesRequested value exceeds available buffer space (buffer size minus padding size)\n");
			break;

		default:
			LOGV("Unknown Error\n");
			break;
		}

		return input;
	} // check_wasapi_error

	DWORD __stdcall win32_audio_submission_thread(LPVOID data)
	{
		win32_wasapi_state* state = reinterpret_cast<win32_wasapi_state*>(data);

		assert(state->sample_rate_hz != 0);
		assert(state->buffer_frame_count != 0);
		assert(state->audio_pull_callback != 0);

		for (;;)
		{
			// block until the next audio buffer is signaled
			WaitForSingleObject(audio_state->audio_event_handle, INFINITE);

			if (WaitForSingleObject(audio_state->submission_event_handle, 0) == WAIT_OBJECT_0)
			{
				// This thread was signaled to exit.
				break;
			}
			else if (audio_state->audio_client)
			{
				// update the sample position from the device
				UINT64 position;
				HRESULT clock_fetch = audio_state->audio_clock->GetPosition(&position, NULL);
				assert(SUCCEEDED(clock_fetch));
				float seconds_pos = (position * audio_state->audio_clock_frequency_denominator);
				state->last_frame_count = static_cast<uint64_t>(state->sample_rate_hz * seconds_pos);
				//LOGV("seconds_pos %i [%2.2f] (actual: %2.2f)\n", state->last_frame_count, state->last_frame_count/static_cast<float>(state->sample_rate_hz), seconds_pos);

				UINT32 padding_frames = 0;
				UINT32 frames_available = 0;

				// get padding in existing buffer
				assert(S_OK == audio_state->audio_client->GetCurrentPadding(&padding_frames));

				// get available frames
				BYTE* new_buffer;
				frames_available = state->buffer_frame_count - padding_frames;
				HRESULT get_buffer_result = audio_state->render_client->GetBuffer(
					frames_available,
					&new_buffer
				);
				assert(check_wasapi_error(get_buffer_result) == S_OK);

				// fill the buffer
				state->audio_pull_callback(new_buffer, frames_available, state->sample_rate_hz, state->context);

				HRESULT release_buffer_result = audio_state->render_client->ReleaseBuffer(frames_available, 0);
				assert(check_wasapi_error(release_buffer_result) == S_OK);
			}
		}

		// signal to the main thread that the submission thread is exiting.
		SetEvent(audio_state->submission_exited_thread);
		return 0;
	} // win32_audio_submission_thread

	static void populate_audio_device_from_endpoint(audio_device& device, IMMDevice* endpoint)
	{
		IPropertyStore* properties;
		if (SUCCEEDED(endpoint->OpenPropertyStore(STGM_READ, &properties)))
		{
			//PROPVARIANT friendly_name;
			//PropVariantInit(&friendly_name);
			//if (SUCCEEDED(properties->GetValue(PKEY_DeviceInterface_FriendlyName, &friendly_name)))
			//{
			//	LOGV("endpoint: %i '%S'\n", index, friendly_name.pwszVal);
			//	PropVariantClear(&friendly_name);
			//}

			//PROPVARIANT device_desc;
			//PropVariantInit(&device_desc);
			//if (SUCCEEDED(properties->GetValue(PKEY_Device_DeviceDesc, &device_desc)))
			//{
			//	LOGV("endpoint: %i '%S'\n", index, device_desc.pwszVal);
			//	PropVariantClear(&device_desc);
			//}

			// For now, just assume these are ONLY output devices.
			device.type = AudioDeviceType::Output;

			// combined: FriendlyName (DeviceInterface)
			PROPVARIANT device_name;
			PropVariantInit(&device_name);
			if (SUCCEEDED(properties->GetValue(PKEY_Device_FriendlyName, &device_name)))
			{
				size_t name_length = wcslen(device_name.pwszVal);

				// This assert is hit if this string won't fit in our StackString.
				assert(device.name.max_size() - 1 >= name_length);

				int conversion_result = WideCharToMultiByte(CP_UTF8,
					0, // CP_UTF8 requires flags to be 0.
					device_name.pwszVal,
					static_cast<int>(name_length),
					&device.name[0],
					static_cast<int>(device.name.max_size() - 1),
					NULL,
					NULL
				);

				assert(conversion_result != 0);

				PropVariantClear(&device_name);
			}

			safe_release(&properties);
		}
	} // populate_audio_device_from_endpoint

	platform::Result audio_enumerate_devices(Array<audio_device*>& devices)
	{
		//devices.resize(audio_state->devices.size());
		for (size_t index = 0; index < audio_state->devices.size(); ++index)
		{
			audio_device* device = audio_state->devices[index];
			audio_device dev = *device;
			devices.push_back(device);
		}

		return platform::Result::success();
	} // audio_enumerate_devices

	platform::Result audio_open_output_device(audio_device* device)
	{
		// If you hit this, the audio_client was already valid before open.
		assert(audio_state->audio_client == nullptr);

		assert(device->type == AudioDeviceType::Output);

		// Access the win32 instance we stored behind the audio_device.
		win32_audio_device* win32_instance = reinterpret_cast<win32_audio_device*>(device + 1);
		IMMDevice* endpoint = win32_instance->endpoint;
		assert(endpoint);

		// try to activate the render device.
		if (FAILED(endpoint->Activate(__uuidof(IAudioClient),
			CLSCTX_ALL,
			0,
			reinterpret_cast<LPVOID*>(&audio_state->audio_client))))
		{
			return platform::Result::failure("Output device Activation failed");
		}

		//
		// setup the audio format:
		// 44.1kHz, 2 channels, 16-bits per sample.
		audio_state->sample_rate_hz = 44100;

		WAVEFORMATEX* mix_format;
		audio_state->audio_client->GetMixFormat(&mix_format);

		const UINT32 mix_sample_size = mix_format->nBlockAlign;
		assert(mix_format->nSamplesPerSec == audio_state->sample_rate_hz);
		WAVEFORMATEX waveformat;
		memset(&waveformat, 0, sizeof(WAVEFORMATEX));
		waveformat.nChannels = 2;
		waveformat.wFormatTag = WAVE_FORMAT_PCM;
		waveformat.nSamplesPerSec = audio_state->sample_rate_hz;
		waveformat.wBitsPerSample = 16;
		waveformat.nBlockAlign = static_cast<uint32_t>((waveformat.nChannels * waveformat.wBitsPerSample) / 8);
		waveformat.nAvgBytesPerSec = (waveformat.nSamplesPerSec * waveformat.nBlockAlign);
		waveformat.cbSize = 0;

		const UINT32 block_align = mix_sample_size * waveformat.nBlockAlign;

		//
		// Check to see if the format is supported in a certain share mode.
		//
		//WAVEFORMATEX* shared_format;
		//HRESULT format_supported = audio_client->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, &waveformat, &shared_format);
		//if (SUCCEEDED(format_supported))
		//{
		//	LOGV("format IS supported. setting...\n");
		//	CoTaskMemFree(shared_format);
		//}
		//else
		//{
		//	// format is NOT supported for EXCLUSIVE mode.
		//	assert(0);
		//}

		const DWORD stream_flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK;
		HRESULT stream_initialize = audio_state->audio_client->Initialize(
			AUDCLNT_SHAREMODE_SHARED,
			stream_flags,
			0,
			0, // always zero in SHARED mode.
			&waveformat,
			NULL
		);
		assert(check_wasapi_error(stream_initialize) == S_OK);

		//REFERENCE_TIME stream_latency;
		//HRESULT stream_latency_result = audio_client->GetStreamLatency(&stream_latency);
		//assert(stream_latency_result == S_OK);

		//REFERENCE_TIME default_device_period, min_device_period;
		//HRESULT get_device_period = audio_client->GetDevicePeriod(&default_device_period, &min_device_period);
		//assert(get_device_period == S_OK);

		//const double seconds_period = (default_device_period * 1.0e2) * SecondsPerNanosecond;
		//LOGV("period: %2.2f; hz: %2.2f\n", seconds_period, (1.0 / seconds_period));

		// assign the audio_event_handle created on startup to the audio client.
		HRESULT set_event_result = audio_state->audio_client->SetEventHandle(audio_state->audio_event_handle);
		assert(set_event_result == S_OK);

		// cache the buffer size
		HRESULT get_buffer_size_result = audio_state->audio_client->GetBufferSize(&audio_state->buffer_frame_count);
		assert(get_buffer_size_result == S_OK);
		//LOGV("buffer size: %i\n", audio_state->buffer_frame_count);

		// clean up mix format
		CoTaskMemFree(mix_format);

		audio_state->audio_clock = nullptr;
		HRESULT service_result = audio_state->audio_client->GetService(
			__uuidof(IAudioClock),
			reinterpret_cast<LPVOID*>(&audio_state->audio_clock)
		);
		assert(SUCCEEDED(service_result));

		UINT64 audio_clock_frequency;
		audio_state->audio_clock->GetFrequency(&audio_clock_frequency);
		audio_state->audio_clock_frequency_denominator = (1.0f / static_cast<float>(audio_clock_frequency));

		// Next; we need to create a render client to output audio.
		HRESULT get_service_result = audio_state->audio_client->GetService(
			__uuidof(IAudioRenderClient),
			reinterpret_cast<LPVOID*>(&audio_state->render_client)
		);
		assert(get_service_result == S_OK);

		HRESULT start_result = audio_state->audio_client->Start();
		assert(start_result == S_OK);

		DWORD thread_id;
		audio_state->submission_thread_handle = CreateThread(
			NULL,										// security attributes
			0,											// initial size of stack in bytes
			win32_audio_submission_thread,				// start address
			static_cast<LPVOID>(audio_state),			// thread's data parameter
			0,											// creation flags
			&thread_id									// thread id
		);

		return platform::Result::success();
	} // audio_open_output_device

	void audio_close_output_device()
	{
		// If you hit this, close device was called before opening a device.
		assert(audio_state->audio_client);

		audio_state->audio_client->Stop();
		safe_release(&audio_state->audio_client);
	} // audio_close_output_device

	void create_device_list(win32_wasapi_state* state)
	{
		// try to create the device enumerator instance.
		IMMDeviceEnumerator* device_enumerator;
		HRESULT create_enumerator_result = CoCreateInstance(
			__uuidof(MMDeviceEnumerator),
			0,
			CLSCTX_ALL,
			__uuidof(IMMDeviceEnumerator),
			reinterpret_cast<LPVOID*>(&device_enumerator)
		);

		if (FAILED(create_enumerator_result))
		{
			// platform::Result::failure("IMMDeviceEnumerator creation failed");
			return;
		}

		// let's enumerate the audio endpoint devices.
		const EDataFlow device_type = eRender; // eRender, eCapture, eAll.
		const DWORD state_mask = DEVICE_STATE_ACTIVE;
		IMMDeviceCollection* device_collection;
		HRESULT enum_result = device_enumerator->EnumAudioEndpoints(device_type, state_mask, &device_collection);
		if (FAILED(enum_result))
		{
			//platform::Result::failure("Failed to enumerate devices");
			return;
		}
		else
		{
			// Retrieve the default device.
			// This should be listed first in the device array.
			IMMDevice* default_render_device;
			if (FAILED(device_enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &default_render_device)))
			{
				//platform::Result::failure("Get default render device failed");
				return;
			}

			UINT device_count;
			device_collection->GetCount(&device_count);

			state->devices.resize(static_cast<size_t>(device_count));

			wchar_t* default_device_uuid = nullptr;
			default_render_device->GetId(&default_device_uuid);

			for (UINT index = 0; index < device_count; ++index)
			{
				IMMDevice* device;
				if (SUCCEEDED(device_collection->Item(index, &device)))
				{
					wchar_t* device_uuid = nullptr;
					device->GetId(&device_uuid);

					const size_t audio_device_size = sizeof(audio_device) + sizeof(win32_audio_device);

					char* memory = static_cast<char*>(MEMORY_ALLOC(audio_device_size, get_platform_allocator()));
					assert(memory);

					audio_device* instance = new (memory) audio_device();
					win32_audio_device* win32_instance = new (memory + sizeof(audio_device)) win32_audio_device();
					win32_instance->endpoint = device;

					state->devices[index] = instance;

					populate_audio_device_from_endpoint(*state->devices[index], device);

					if (wcscmp(device_uuid, default_device_uuid) == 0)
					{
						// Skip the default device we already populated and
						// make sure it appears first in the list.
						state->devices.swap(index, 0);
					}

					CoTaskMemFree(device_uuid);
				}
			}

			// release default device resources
			assert(default_device_uuid);
			CoTaskMemFree(default_device_uuid);
			safe_release(&default_render_device);

			// finally, free the device_collection.
			safe_release(&device_collection);
		}

		safe_release(&device_enumerator);
	} // create_device_list

	platform::Result audio_startup()
	{
		//
		// We must initialize COM first. This requires objbase.h and ole32.lib.
		if (FAILED(CoInitializeEx(0, COINIT_MULTITHREADED)))
		{
			assert(!"COM initialization failed");
			return platform::Result::failure("COM initialization failed");
		}

		audio_state = MEMORY_NEW(win32_wasapi_state, get_platform_allocator());

		// this is used by IAudioClient to signal our worker thread
		// when we can populate the buffer.
		audio_state->audio_event_handle = CreateEvent(
			NULL,
			FALSE,
			FALSE,
			NULL
		);

		// this controls whether or not the submission thread will exit.
		audio_state->submission_event_handle = CreateEvent(
			NULL,
			TRUE,
			FALSE,
			NULL
		);

		// flags the main thread that the submission thread has exited.
		audio_state->submission_exited_thread = CreateEvent(
			NULL,
			TRUE,
			FALSE,
			NULL
		);

		// cache the list of devices on system.
		create_device_list(audio_state);

		return platform::Result::success();
	} // audio_startup

	void audio_shutdown()
	{
		// trigger the exit of the submission thread.
		SetEvent(audio_state->audio_event_handle);
		SetEvent(audio_state->submission_event_handle);

		// wait until the thread successfully exits.
		WaitForSingleObject(audio_state->submission_exited_thread, INFINITE);

		// It seems that we can introduce a sleep and have the
		// sound taper off smoothly as opposed to abruptly stopping.
		// Is there a better way to handle this?
		Sleep(300);

		// cleanup all events.
		CloseHandle(audio_state->audio_event_handle);
		CloseHandle(audio_state->submission_event_handle);
		CloseHandle(audio_state->submission_exited_thread);

		// now we can releases render and audio clients.
		safe_release(&audio_state->render_client);
		safe_release(&audio_state->audio_clock);
		safe_release(&audio_state->audio_client);

		for (size_t index = 0; index < audio_state->devices.size(); ++index)
		{
			audio_device* device = audio_state->devices[index];
			MEMORY_DEALLOC(device, get_platform_allocator());
		}

		MEMORY_DELETE(audio_state, get_platform_allocator());

		CoUninitialize();
	} // audio_shutdown

	void audio_set_callback(audio_sound_callback callback, void* context)
	{
		audio_state->audio_pull_callback = callback;
		audio_state->context = context;
	} // audio_set_callback

	uint64_t audio_frame_position()
	{
		return audio_state->last_frame_count;
	} // audio_frame_position
} // namespace platform

#endif // GEMINI_ENABLE_AUDIO