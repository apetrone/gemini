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
	
	typedef unsigned int SoundHandle;
	typedef int SoundSource;
	
	class IAudioDriver
	{
	public:
		virtual ~IAudioDecoder() {}
		virtual void update() = 0;
		virtual void event( EventType event ) = 0;
		virtual void stop_all_sounds() = 0;
		virtual void resume() = 0;
		
	}; // class IAudioDriver


	// -------------------------------------------------------------
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

	
	// interface
	void startup( const char * driver_name );
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