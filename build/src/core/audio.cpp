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
#include "typedefs.h"
#include "audio.hpp"
#include <slim/xlog.h>
#include "stackstring.hpp"
#include "filesystem.hpp"
#include "assets.hpp"
#include "factory.hpp"

#if !PLATFORM_IS_MOBILE
	#include "openal.hpp"
	#include "openal_vorbis_decoder.hpp"
	typedef stb_vorbis_decoder AudioDecoderType;
	#define DRIVER_NAME "OpenAL"
	#define DRIVER_CREATOR OpenAL::creator

#elif __APPLE__ && PLATFORM_IS_MOBILE
	#include "audio_extaudio_decoder.hpp"
	#include "openal.hpp"
	typedef ExtAudioDecoder AudioDecoderType;
	#define DRIVER_NAME "OpenAL"
	#define DRIVER_CREATOR OpenAL::creator
#elif __ANDROID__
	#define DRIVER_NAME "OpenSLES"

	using namespace audio;
	class OpenSLES : public audio::IAudioDriver
	{
		DECLARE_FACTORY_CLASS( OpenSLES, audio::IAudioDriver );
	public:
		OpenSLES() {};
		~OpenSLES() {};
		
		virtual void event( audio::EventType event ) {};
		virtual void prepare_source( AudioSource * source ) {};
		virtual void play_source( AudioSource * source ) {};
		virtual void update_source( AudioSource * source ) {};
		virtual void stop_source( AudioSource * source ) {};
		virtual void clean_source( AudioSource * source ) {};
	}; // OpenSLES


	#define DRIVER_CREATOR OpenSLES::creator
	class AndroidDecoderNoop : public IAudioDecoder
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
#else
	#error Unknown platform!
#endif

namespace audio
{
	audio::IAudioDriver * _audio_driver = 0;
	namespace _internal
	{		
		int used_sources = 0;
		AudioSource _sources[ AUDIO_MAX_SOURCES ];
		
		AudioSource * find_unused_source()
		{
			for( int i = 0; i < AUDIO_MAX_SOURCES; ++i )
			{
				if ( _sources[i].flags == 0 )
				{
					_sources[i].index = i;
					return &_sources[i];
				}
			}
			
			return 0;
		} // find_unused_source
		
		
		void init_sources()
		{
			for( unsigned int i = 0; i < AUDIO_MAX_SOURCES; ++i )
			{
				_sources[i].flags = 0;
				_sources[i].has_buffers = 0;
				_sources[i]._decoder = CREATE(AudioDecoderType);
			}
		}
		
		// -------------------------------------------------------------
		typedef struct
		{
			unsigned char * data;
			int dataSize;
			short index;
			bool is_used;
			bool is_stream;
			
			// for debug:
			const char * filename;
//			unsigned int refcount;
		} Sound;
		Sound _sounds[ AUDIO_MAX_SOUNDS ];
		unsigned int current_sound = 0;
		
		Sound * find_sound( SoundHandle handle )
		{
			if ( handle < AUDIO_MAX_SOUNDS )
			{
				return &_sounds[ handle ];
			}
			
			return 0;
		} // find_sound
		
		Sound * find_unused_sound()
		{
			for( int i = 0; i < AUDIO_MAX_SOUNDS; ++i )
			{
				if ( !_sounds[i].is_used )
				{
					return &_sounds[i];
				}
			}
			
			return 0;
		} // find_unused_sound

		void init_sounds()
		{
			for( unsigned int i = 0; i < AUDIO_MAX_SOUNDS; ++i )
			{
				_sounds[i].is_used = 0;
				_sounds[i].is_stream = 0;
				_sounds[i].data = 0;
				_sounds[i].dataSize = 0;
				_sounds[i].filename = 0;
				_sounds[i].index = i;
//				_sounds[i].refcount = 0;
			}
		}
		
		void shutdown_sounds()
		{
			if ( !_audio_driver )
			{
				return;
			}
			
			AudioSource * source;
			for( unsigned int i = 0; i < AUDIO_MAX_SOURCES; ++i )
			{
				source = &_sources[ i ];
				if ( source->flags & SF_PLAYING )
				{
					source->_decoder->close();
					_audio_driver->clean_source( source );
				}
				
				DESTROY(IAudioDecoder, source->_decoder);
				source->_decoder = 0;
			}
			
			Sound * sound;
			for( unsigned int i = 0; i < AUDIO_MAX_SOUNDS; ++i )
			{
				sound = &_sounds[ i ];
				if ( sound->is_used )
				{
					DEALLOC(sound->data);
					sound->data = 0;
					sound->dataSize = 0;
				}
			}
		} // shutdown sounds
		
		
		SoundHandle create_new_sound( const char * filename, bool is_stream )
		{
			Sound * sound = find_unused_sound();
			if ( !sound )
			{
				LOGE( "AUDIO_MAX_SOUNDS reached!\n" );
				return 0;
			}
			
			StackString<MAX_PATH_SIZE> path = filename;
			StackString<MAX_PATH_SIZE> extension;
			assets::append_asset_extension( assets::SoundAsset, path );
//			assets::sounds()->append_extension( path );
			
			sound->data = (unsigned char*)fs::audiofile_to_buffer( path(), sound->dataSize );
			if ( !sound->data )
			{
				LOGE( "audio::create_new_sound - could not open file %s\n", path() );
				return 0;
			}
			
			// we're now using the sound
			sound->is_used = true;
//			sound->refcount = 0;
			sound->filename = filename;
			sound->is_stream = is_stream;
			
			LOGV( "audio::create_new_sound -> [data=%p,dataSize=%i,filename='%s']\n", sound->data, sound->dataSize, sound->filename );
			
			return sound->index;
		} // init_sound
	}; // namespace _internal


	// interface
	void startup()
	{
		typedef Factory<IAudioDriver, 4> AudioDriverFactory;
		AudioDriverFactory factory;
		
		
		factory.register_class( DRIVER_CREATOR, DRIVER_NAME );

		AudioDriverFactory::Record * record = factory.find_class( DRIVER_NAME );
		if ( record )
		{
			_audio_driver = record->creator();
			if ( !_audio_driver )
			{
				LOGE( "Unable to create instance of audio driver '%s'\n", record->class_name );
				return;
			}
			
			LOGV( "Initialized audio device: '%s'\n", record->class_name );
			
			_internal::init_sounds();
			_internal::init_sources();
		}
		else
		{
			LOGE( "Unable to find a suitable audio driver for this platform!\n" );
		}
		
		return;
	} // startup
	
	void shutdown()
	{
		stop_all_sounds();
		_internal::shutdown_sounds();
		
		if ( _audio_driver )
		{
			DESTROY( IAudioDriver, _audio_driver );
			_audio_driver = 0;
		}
	} // shutdown
	
	// update all sources
	void update()
	{
		if ( !_audio_driver )
		{
			return;
		}
		
		AudioSource * source;
		_internal::used_sources = 0;
		for( int i = 0; i < AUDIO_MAX_SOURCES; ++i )
		{
			source = &_internal::_sources[i];
			if ( source->flags & SF_STOP )
			{
				_audio_driver->stop_source( source );
				source->flags = 0;
			}
			else if ( source->flags & SF_PLAYING )
			{
				++_internal::used_sources;
				_audio_driver->update_source( source );
			}
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
		if ( !_audio_driver )
		{
			return;
		}
		
		AudioSource * source;
		unsigned int total_sources = 0;
		for( unsigned int i = 0; i < AUDIO_MAX_SOURCES; ++i )
		{
			source = &_internal::_sources[ i ];
			if ( source->flags & SF_PLAYING )
			{
				_audio_driver->stop_source( source );
				++total_sources;
			}
		}
		
		LOGV( "Stopped %i sources.\n", total_sources );
	} // stop_all_sounds
	
	// resume any previously playing sources
	void resume()
	{
		if ( !_audio_driver )
		{
			return;
		}
		
		AudioSource * source;
		unsigned int total_sources = 0;
		for( int i = 0; i < AUDIO_MAX_SOURCES; ++i )
		{
			source = &_internal::_sources[i];
			if ( source->flags & SF_PLAYING )
			{
				_audio_driver->play_source( source );
				++total_sources;
			}
		}
		
		LOGV( "Resumed %i sources\n", total_sources );
	} // resume
	
	// return the number of sources currently being used
	int count_used_sources()
	{
		return _internal::used_sources;
	} // count_used_sources
	
	SoundHandle create_sound( const char * filename )
	{
		return _internal::create_new_sound( filename, false );
	} // create_sound
	
	SoundHandle create_stream( const char * filename )
	{
		return _internal::create_new_sound( filename, true );
	} // create_stream
	
	SoundSource play( SoundHandle handle, int num_repeats )
	{
		int source_id = -1;
		
		if ( !_audio_driver )
		{
			LOGE( "No valid audio driver found.\n" );
			return source_id;
		}
		
		// locate the sound via handle
		_internal::Sound * sound = _internal::find_sound( handle );
		if ( !sound || !sound->is_used )
		{
			LOGE( "ERROR - Invalid sound handle\n" );
			return 0;
		}
		
		// try to allocate a new sound source
		AudioSource * source = _internal::find_unused_source();
		if ( !source )
		{
			LOGE( "AUDIO_MAX_SOURCES reached! %i\n", AUDIO_MAX_SOURCES );
			return source_id;
		}
		
		if ( source->_decoder->open( (unsigned char*)sound->data, sound->dataSize ) )
		{
			source->num_repeats = num_repeats;
			source_id = source->index;
//			sound->refcount++;
			source->flags = SF_PLAYING;
		}
		else
		{
			source->flags = 0;
			return source_id;
		}

		// tell the audio driver we're going to play this source
		_audio_driver->prepare_source( source );

		// tell the driver to play it
		_audio_driver->play_source( source );
		
		return source_id;
	} // play
	
	void stop( SoundSource source_id )
	{
		if (source_id < 0 || source_id > AUDIO_MAX_SOURCES )
		{
			LOGW( "source_id out of range.\n" );
			return;
		}
		
		AudioSource * source = &_internal::_sources[ source_id ];
		if ( source )
		{
			source->flags |= SF_STOP;
		}
		else
		{
			LOGW( "No matching sound found for %i\n", source_id );
		}
	} // stop
}; // namespace audio