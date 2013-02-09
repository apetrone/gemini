// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM,OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
// -------------------------------------------------------------
#pragma once


namespace audio
{
	enum EventType
	{
		Invalid,
		DetachContext,
		ReattachContext
	}; // EventType
	
	enum DriverType
	{
		Default,
		OpenAL
	}; // DriverType
	
	typedef unsigned int SoundHandle;
	typedef int SoundSource;
	const unsigned int AUDIO_EMITTER_BUFFERS = 2;
	const unsigned int AUDIO_MAX_EMITTERS = 64;
	const unsigned int AUDIO_EMITTER_BUFFER_SIZE = 8192;
	
	// http://stackoverflow.com/questions/2871905/openal-determine-maximum-sources
	// determine maximum number of sources
	// iPhone supports 32, Windows DirectSound limited to 31. Some hardware has support for 256.
	// Assume 16 - 32 here.
	const unsigned int AUDIO_MAX_SOURCES = 32;
	const unsigned int AUDIO_MAX_SOUNDS = 16;
	
	enum
	{
		SF_NONE 		= 0,
		SF_PLAYING 		= 1,
		SF_STOP			= 2,
	};
#if 0
	enum
	{
		EF_NONE,
		EF_STREAM,
		EF_OPEN
	};
#endif
	
	
	// generic interface for audio-decoding
	class IAudioDecoder
	{
	public:
		virtual ~IAudioDecoder() {}
		
		// called to initialize and reset an existing stream
		virtual void reset() = 0;
		
		// decode stream up to data_length into data
		// return bytes decoded, or 0 on EOF
		virtual int decode( void * data, int data_length ) = 0;
		
		// open a stream/sound from memory
		// returns 1 on success; 0 on failure
		virtual int open( unsigned char * data, int data_length ) = 0;
		
		virtual void close() = 0;
		
		virtual int channels() = 0;
		virtual int frequency() = 0;
		virtual void rewind() = 0;
	}; // IAudioDecoder
	
	
	struct AudioSource
	{
		unsigned int source_id;
		unsigned int index;
		bool has_buffers;
		bool should_stop;
		short flags;
		int num_repeats;
		
		unsigned int buffers[ AUDIO_EMITTER_BUFFERS ];
		IAudioDecoder * _decoder;
	}; // AudioSource
	
	class IAudioDriver
	{
	public:
		virtual ~IAudioDriver() {}
		virtual void event( EventType event ) = 0;
		
		// this is called prior to calling play_source; allocate any needed buffers; setup default values, etc
		// fill the first buffer(s)
		virtual void prepare_source( AudioSource * source ) = 0;
		
		// play audio for a given source
		virtual void play_source( AudioSource * source ) = 0;
		
		// update this source; re-buffer, etc
		virtual void update_source( AudioSource * source ) = 0;
		
		// stop this source from playing
		virtual void stop_source( AudioSource * source ) = 0;
		
		// clean up resources for this source
		virtual void clean_source( AudioSource * source ) = 0;
	}; // class IAudioDriver







	
	// interface
	void startup();
	void shutdown();
	
	// update all sources
	void update();
	
	// handle an event
	void event( EventType type );
	
	// stop all sounds
	void stop_all_sounds();
	
	// resume any previously playing sources
	void resume();
	
	// return the number of sources currently being used
	int count_used_sources();
	
	SoundHandle create_sound( const char * filename );
	SoundHandle create_stream( const char * filename );
	SoundSource play( SoundHandle handle, int num_repeats = 0 );
	void stop( SoundSource source_id );
	
	
}; // namespace audio