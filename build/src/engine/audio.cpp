// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone
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
#include "audio.h"

#include <core/factory.h>
#include <core/typedefs.h>
#include <core/stackstring.h>
#include <core/logging.h>

#include <runtime/filesystem.h>

#include <sdk/audio_api.h>

#if defined(PLATFORM_IPHONEOS)
	#include "audio_extaudio_decoder.h"
	#include "openal.h"
	namespace gemini
	{
		typedef ExtAudioDecoder AudioDecoderType;
	} // namespace gemini
	#define DRIVER_NAME "OpenAL"
	#define DRIVER_CREATOR OpenAL::creator
#elif defined(PLATFORM_ANDROID)
	#define DRIVER_NAME "OpenSLES"
	#define DRIVER_CREATOR OpenSLES::creator

	namespace gemini
	{
		class AndroidDecoderNoop : public audio::IAudioDecoder
		{
		public:
			virtual void reset() {}
			virtual int decode( void * data, int data_length ) { return 0; }
			virtual int open( unsigned char * data, int data_length ) { return 0; }
			virtual void close() {}
			virtual int channels() { return 0; }
			virtual int frequency() { return 0; }
			virtual void rewind() {}
		};
		typedef AndroidDecoderNoop AudioDecoderType;
	} // namespace gemini
#elif defined(PLATFORM_LINUX) || defined(PLATFORM_WINDOWS) || defined(PLATFORM_MACOSX)
#else
	#error Unknown platform or audio library!
#endif

namespace gemini
{
	namespace audio
	{
		IAudioDriver* _audio_driver = 0;
	} // namespace audio
} // namespace gemini
