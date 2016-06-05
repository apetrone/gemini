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

#include <core/typedefs.h>
//#include <core/util.h> // for std::function
//#include <platform/platform.h>

#if 0
Audio Decompression
* Decoding audio on the fly
* Decoding to cache

* Using 3D surfaces as audio emitters
- points, lines, spheres, boxes, etc.

* Audio Signal Processing on the GPU

* MultiStream
- vocoders
- convolution reverb
- granular synthesis vs cross - fading loops
- volume parameters per stream per channel
- Frequency domain processing(FFT / iFFT)
- Choosing the correct window size : (high frequency->large window)
* Streaming :
	-Don't handle data loading from within the audio engine.
	- currently playing streams must be updated first(to avoid skipping)
	- newly requested streams can be updated next
	- data load for the game happens last
	* Surround Sound
	- User can supply XYZ coordinates of listener and source
	- User can supply angle and distance for source vs listener position
	* Head Related Transfer Functions(HRTFs) and Reverberation: https://en.wikipedia.org/wiki/3D_audio_effect
-http ://www.bitoutsidethebox.com/shabda/hrtf-info/

	* Channel syncing
	- SyncOn / SyncOff to remember Play and Pause requests which should be handled
	during the next audio update.
	* DSP Effects
	- Radio(to simulate radio chatter distortion)
	* Ducker / Side Chain Compressor for ducking less important audio
	* Splitting source audio into separate pieces and dynamically combining at run - time.
	* Rank buffer to prioritize sounds.
	* Memory Snapshots and Categories to organize and craft the levels of audio for scenes.
	* Remote tuning application Game->Remote Application->MIDI Control Surface
	* Automatic Gain Control
	* Cascade System to select sounds based on pitch range.

	* These days, it seems like a lot of people use Headphones,
	so there doesn't seem to be an immediate need to support more than 2 channels.
	However, we should still allow for that in case I expand this in the future.

	Number of output channels
	Maximum number of internal audio channels or sources.
#endif

// This should be part of the game interface for sounds

namespace gemini
{
	namespace assets
	{
		struct Sound;
	}

	namespace audio
	{
		typedef size_t SoundHandle_t;


		// The following assumptions are made:
		// For loaded sounds; they must be TWO-CHANNEL STEREO sounds.
		// Sounds are 16-bit.
		// Frequency is 44.1 kHz.
		const size_t AUDIO_FREQUENCY_HZ = 44100;
		const size_t AUDIO_MAX_OUTPUT_CHANNELS = 2;

		// Samples are stored as floats in memory for mixing.
		typedef float InMemorySampleType;
		const float InMemorySampleValueMax = 32767.0f;

		// precache/load sound; returns an id for that sound
		SoundHandle_t load_sound(const char* asset_name);
		//SoundHandle precache_sound(const char* asset_name);

		// start playing sound [repeats]
		SoundHandle_t play_sound(gemini::assets::Sound*, int32_t repeats = 0);

		// stop playing sound
		void stop_sound(SoundHandle_t handle);

		// stop playing ALL sounds
		void stop_all_sounds();

		// set master volume/gain
		void set_master_volume(float new_volume);
		float get_master_volume();

		// should be called each frame.
		// audio will starve if it cannot maintain 10Hz.
		void tick(float delta_seconds);

		void startup();
		void shutdown();

		// How do we identify when a sound has stopped playing? Rather,
		// how do we expose this to the game? we'll know when the samples have all
		// played, but there must be a better way than a callback.

		size_t get_total_playing_sounds();
	} // namespace audio
} // namespace gemini