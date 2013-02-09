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
#include "audio.hpp"
#include "openal.hpp"
#include "log.h"

namespace audio
{
	audio::IAudioDriver * _audio_driver = 0;
	namespace _internal
	{
		
	}; // namespace _internal


	// interface
	void startup()
	{
		typedef Factory<IAudioDriver, 4> AudioDriverFactory;
		AudioDriverFactory factory;
		
		
		factory.register_class( OpenAL::creator, "OpenAL" );
		
		
		AudioDriverFactory::Record * record = factory.find_class( "OpenAL" );
		if ( record )
		{
			_audio_driver = record->creator();
			if ( !_audio_driver )
			{
				LOGE( "Unable to create instance of audio driver '%s'\n", record->class_name );
				return;
			}
			
			LOGV( "Initialized audio device: '%s'\n", record->class_name );
		}
		else
		{
			LOGE( "Unable to find a suitable audio driver for this platform!\n" );
		}
		
		return;
	} // startup
	
	void shutdown()
	{
		if ( _audio_driver )
		{
			DEALLOC( IAudioDriver, _audio_driver );
			_audio_driver = 0;
		}
	} // shutdown
	
	// update all sources
	void update()
	{
		if ( _audio_driver )
		{
			_audio_driver->update();
		}
	} // update
	
	// handle an event
	void event( EventType type )
	{
		if ( _audio_driver )
		{
			_audio_driver->event( type );
		}
	} // event
	
	// stop all sounds
	void stop_all_sounds()
	{
		if ( _audio_driver )
		{
			_audio_driver->stop_all_sounds();
		}
	} // stop_all_sounds
	
	// resume any previously playing sources
	void resume()
	{
		if ( _audio_driver )
		{
			_audio_driver->resume();
		}
	} // resume
	
	// return the number of sources currently being used
	int count_used_sources()
	{
		return 0;
	} // count_used_sources
	
	SoundHandle create_sound( const char * filename )
	{
		return 0;
	} // create_sound
	
	SoundHandle create_stream( const char * filename )
	{
		return 0;
	} // create_stream
	
	SoundSource play( SoundHandle handle, int num_repeats )
	{
		return 0;
	} // play
	
	void stop( SoundSource source_id )
	{
		
	} // stop
}; // namespace audio