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
#include <core/typedefs.h>
#include <core/stackstring.h>
#include <core/filesystem.h>

#include "audio.h"
#include "assets.h"
#include <core/factory.h>


#include <sdk/audio_api.h>

#if !PLATFORM_IS_MOBILE
	#include "openal.h"
	#include "openal_vorbis_decoder.h"
	namespace gemini
	{
		typedef stb_vorbis_decoder AudioDecoderType;
	} // namespace gemini
	#define DRIVER_NAME "OpenAL"
	#define DRIVER_CREATOR OpenAL::creator

#elif PLATFORM_IOS
	#include "audio_extaudio_decoder.h"
	#include "openal.h"
	namespace gemini
	{
		typedef ExtAudioDecoder AudioDecoderType;
	} // namespace gemini
	#define DRIVER_NAME "OpenAL"
	#define DRIVER_CREATOR OpenAL::creator
#elif PLATFORM_ANDROID

	#include "opensles.h"
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
#else
	#error Unknown platform!
#endif

namespace gemini
{
	namespace audio
	{


		IAudioDriver * _audio_driver = 0;
		
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
					_sources[i]._decoder = MEMORY_NEW(AudioDecoderType, platform::memory::global_allocator());
				}
			}
			
			// -------------------------------------------------------------
			typedef struct
			{
				unsigned char * data;
				size_t data_size;
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
					_sounds[i].data_size = 0;
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
					if ( source->_decoder != 0 )
					{
						source->_decoder->close();
						_audio_driver->clean_source( source );
					}
					
					MEMORY_DELETE(source->_decoder, platform::memory::global_allocator());
					source->_decoder = 0;
				}
				
				Sound * sound;
				for( unsigned int i = 0; i < AUDIO_MAX_SOUNDS; ++i )
				{
					sound = &_sounds[ i ];
					if ( sound->is_used )
					{
						MEMORY_DEALLOC(sound->data, platform::memory::global_allocator());
						sound->data = 0;
						sound->data_size = 0;
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
				
				core::StackString<MAX_PATH_SIZE> path = filename;
				core::StackString<MAX_PATH_SIZE> extension;
				assets::append_asset_extension( assets::SoundAsset, path );
	//			assets::sounds()->append_extension( path );
				
				sound->data = (unsigned char*)core::filesystem::audiofile_to_buffer( path(), sound->data_size );
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
				
				LOGV( "audio::create_new_sound -> [data=%p,dataSize=%i,filename='%s']\n", sound->data, sound->data_size, sound->filename );
				
				return sound->index;
			} // init_sound
		}; // namespace _internal





		class AudioInterface : public IAudioInterface
		{
		public:
			virtual gemini::AudioHandle load(const char* path)
			{
				return (gemini::AudioHandle)create_sound(path);
			}
			
			virtual gemini::AudioSource play(AudioHandle handle, int num_repeats)
			{
				return gemini::audio::play((SoundHandle)handle, num_repeats);
			}
			
			virtual void stop(gemini::AudioSource source)
			{
				gemini::audio::stop((SoundSource)source);
			}
			
			virtual void stop_all_sounds()
			{
				audio::stop_all_sounds();
			}
		};





		// interface
		void startup()
		{
			typedef core::Factory<IAudioDriver> AudioDriverFactory;
			AudioDriverFactory factory;
			
			audio::instance = MEMORY_NEW(AudioInterface, platform::memory::global_allocator());
			
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
				MEMORY_DELETE(_audio_driver, platform::memory::global_allocator());
				_audio_driver = 0;
			}
			
			IAudioInterface* interface = audio::instance();
			MEMORY_DELETE(interface, platform::memory::global_allocator());
			audio::instance = 0;
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
			
			if ( source->_decoder->open( (unsigned char*)sound->data, sound->data_size ) )
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
	} // namespace audio
} // namespace gemini