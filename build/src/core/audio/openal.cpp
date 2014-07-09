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
#include "audio.h"
#include "openal.h"
#include <slim/xlog.h>

OpenAL::OpenAL()
{
	device = alcOpenDevice( 0 );
	check_alc_error();
	
	context = alcCreateContext( device, 0 );
	check_alc_error();
	
	alcMakeContextCurrent( context );
	check_alc_error();
	
	// setup the openal listener
	alListener3f( AL_POSITION, 0, 0, 0 );
	alListener3f( AL_POSITION, 0, 0, 0 );
	
	// atx, aty, atz, upx, upy, upz
	float orientation[6] = { 0, 0, 0, 0, 1.0, 0 };
	alListenerfv( AL_ORIENTATION, orientation );
}

OpenAL::~OpenAL()
{
	if ( context != 0 )
	{
		alcMakeContextCurrent( 0 );
		alcDestroyContext( context );
		context = 0;
	}
	
	if ( device != 0 )
	{
		alcCloseDevice( device );
		device = 0;
	}
}

int OpenAL::check_alc_error()
{
	ALCenum err = alcGetError( device );
	if ( err != ALC_NO_ERROR )
	{
		LOGE( "Error: %x\n", err );
	}
	
	return err;
} // check_alc_error

int OpenAL::check_al_error( const char * context )
{
	ALenum errorCode = alGetError();
	
	if ( errorCode != AL_NO_ERROR )
	{
		// make a fuss about it
		switch( errorCode )
		{
			case AL_INVALID_NAME: LOGE( "OpenAL Error: AL_INVALID_NAME\n" ); break;
			case AL_INVALID_ENUM: LOGE( "OpenAL Error: AL_INVALID_ENUM\n" ); break;
			case AL_INVALID_VALUE: LOGE( "OpenAL Error: AL_INVALID_VALUE\n" ); break;
			case AL_INVALID_OPERATION: LOGE( "OpenAL Error: AL_INVALID_OPERATION\n" ); break;
			case AL_OUT_OF_MEMORY: LOGE( "OpenAL Error: AL_OUT_OF_MEMORY\n" ); break;
			default: LOGE( "OpenAL Error: An OpenAL error occurred.\n" ); break;
		}
	}
	
	if ( errorCode != AL_NO_ERROR )
	{
		LOGE( "OpenAL Error: %i - %s\n", errorCode, context );
	}
	
	return errorCode;
} // check_al_error

const char * OpenAL::source_state_to_string( int source_state )
{
	switch( source_state )
	{
		case AL_PLAYING: return "AL_PLAYING"; break;
		case AL_STOPPED: return "AL_STOPPED"; break;
		case AL_PAUSED: return "AL_PAUSED"; break;
		case AL_INITIAL: return "AL_INITIAL"; break;
	}
	
	return "Unknown";
} // source_state_to_string

void OpenAL::buffer_source( AudioSource *source )
{
	if ( !source->flags & SF_PLAYING )
		return;
	
	for( unsigned int i = 0; i < AUDIO_EMITTER_BUFFERS; ++i )
	{
		stream_source( source, source->buffers[i] );
	}
	
	alSourceQueueBuffers( source->source_id, AUDIO_EMITTER_BUFFERS, source->buffers );
	check_al_error( "OpenAL::buffer_source - alSourceQueueBuffers" );
}

void OpenAL::stream_source( AudioSource * source, int bufferid )
{
	int bytes = 0;
	int ret = 0;
	char pcmbuffer[ AUDIO_EMITTER_BUFFER_SIZE ];
	ALenum format = AL_FORMAT_MONO16;
	memset( pcmbuffer, 0, AUDIO_EMITTER_BUFFER_SIZE );
	
	switch( source->_decoder->channels() )
	{
		case 1: format = AL_FORMAT_MONO16; break;
		case 2: format = AL_FORMAT_STEREO16; break;
	}
	
	do
	{
		ret = source->_decoder->decode( pcmbuffer+bytes, AUDIO_EMITTER_BUFFER_SIZE-bytes );
		if ( ret == 0 )
		{
			if ( source->num_repeats == 0 )
				break;
			
			// either repeating for num_repeats or repeating forever
			if ( source->num_repeats > 0 || source->num_repeats < 0 )
			{
				if ( source->num_repeats > 0 )
				{
					--source->num_repeats;
				}
				source->_decoder->rewind();
			}
			
			ret = source->_decoder->decode( pcmbuffer+bytes, AUDIO_EMITTER_BUFFER_SIZE-bytes );
		}
		
		bytes += ret;
	} while( (bytes > 0) && (bytes < AUDIO_EMITTER_BUFFER_SIZE) );
	
	if ( source->flags & SF_STOP )
		return;
	
	alBufferData( bufferid, format, pcmbuffer, bytes, source->_decoder->frequency() );
	check_al_error( xstr_format("OpenAL::stream_source - alBufferData [%i, %i, %i, %i]", source->source_id, (source->flags & SF_PLAYING), source->num_repeats, bytes) );
} // stream_source

void OpenAL::event( audio::EventType event )
{
	if ( event == audio::DetachContext )
	{
		alcMakeContextCurrent( 0 );
	}
	else if ( event == audio::ReattachContext )
	{
		alcMakeContextCurrent( context );
	}
} // event

void OpenAL::prepare_source( AudioSource * source )
{
	if ( !source->has_buffers )
	{
		source->has_buffers = true;
		//LogMsg( "-> Creating buffers for source: %i\n", source->index );
		source->source_id = 0;
		memset( source->buffers, 0, sizeof(unsigned int)*AUDIO_EMITTER_BUFFERS);
		alGenBuffers( AUDIO_EMITTER_BUFFERS, source->buffers );
		//LogMsg( "-> Generated buffers: %i %i\n", source->buffers[0], source->buffers[1] );
		check_al_error( "OpenAL::setup_source - alGenBuffers" );
		alGenSources( 1, &source->source_id );
		check_al_error( "OpenAL::setup_source - alGenSources" );
	}

	// set basic properties
	float pos[] = {0, 0, 0};
	float vel[] = {0, 0, 0};
	
	alSourcef( source->source_id, AL_PITCH, 1.0f );
	alSourcef( source->source_id, AL_GAIN,  1.0f );
	alSourcefv( source->source_id, AL_POSITION, pos );
	alSourcefv( source->source_id, AL_VELOCITY, vel );
	alSourcei( source->source_id, AL_LOOPING, false );
	//alSourcef( sourceid, AL_REFERENCE_DISTANCE, 5.0f );
	check_al_error( "OpenAL::setup_source - source setup" );
	
	this->buffer_source( source );
} // prepare_source

void OpenAL::play_source( AudioSource * source )
{
	if ( !(source->flags & SF_PLAYING) )
		return;
	
	ALint state;
	alGetSourcei( source->source_id, AL_SOURCE_STATE, &state );
	if ( state == AL_STOPPED )
	{
		alSourceStop( source->source_id );
		
		ALint queued = -1;
		ALuint bufferid = 0;
		
		// if it's currently playing, stop this emitter
		alSourceStop( source->source_id );
		check_al_error( "OpenAL::play_source - alSourceStop" );
		
		alGetSourcei( source->source_id, AL_BUFFERS_QUEUED, &queued );
		check_al_error( "OpenAL::play_source - alGetSourcei (AL_BUFFERS_QUEUED)" );
		
		while( queued-- )
		{
			alSourceUnqueueBuffers( source->source_id, 1, &bufferid );
			check_al_error( "OpenAL::play_source - alSourceUnqueueBuffers" );
		}
		
		source->_decoder->rewind();
		buffer_source( source );
		alSourcePlay( source->source_id );
	}
	else if ( state != AL_PLAYING )
	{
		alSourcePlay( source->source_id );
		check_al_error( "OpenAL::play_source - alSourcePlay" );
	}
} // play_source

void OpenAL::update_source( AudioSource * source )
{
	ALint state = 0;
	ALint processed;
	ALuint bufferid;
	alGetSourcei( source->source_id, AL_SOURCE_STATE, &state );
	check_al_error( "OpenAL::update_source - alGetSourcei (AL_SOURCE_STATE)" );
	
	if ( state == AL_STOPPED && source->num_repeats != 0 )
	{
		LOGV( "source is stopped, but should still play. nudging it...\n" );
		alSourcePlay( source->source_id );
	}
	else if ( state != AL_STOPPED )
	{
		// attempt updating from the sound file...
		alGetSourcei( source->source_id, AL_BUFFERS_PROCESSED, &processed );
		check_al_error( "OpenAL::update_source - alGetSourcei (AL_BUFFERS_PROCESSED)" );
		
		while( processed > 0 )
		{
			alSourceUnqueueBuffers( source->source_id, 1, &bufferid );
			check_al_error( "OpenAL::update_source - alSourceUnqueueBuffers" );
			
			stream_source( source, bufferid );
			
			alSourceQueueBuffers( source->source_id, 1, &bufferid );
			check_al_error( "OpenAL::update_source - alSourceQueueBuffers" );
			
			--processed;
		}
	}
	else
	{
		//LogMsg( "source %i has stopped playing. Source state is %s\n", source->index, SourceStateToString( state ) );
		stop_source( source );
		source->flags = SF_NONE;
	}
} // update_source

void OpenAL::stop_source( AudioSource * source )
{
	if ( source->flags == 0 )
		return;
	
	ALint num_buffers = -1;
	
	// if it's currently playing, stop this emitter
	alSourceStop( source->source_id );
	check_al_error( "OpenAL::stop_source - alSourceStop" );
	
	// step one: unqueue any buffers attached to the source
	alGetSourcei( source->source_id, AL_BUFFERS_QUEUED, &num_buffers );
	check_al_error( "OpenAL::stop_source - alGetSourcei (AL_BUFFERS_QUEUED)" );
	
	alSourceUnqueueBuffers( source->source_id, num_buffers, source->buffers );
	check_al_error( "OpenAL::stop_source - alSourceUnqueueBuffers (queued)" );
	
	source->flags = 0;
} // stop_source


void OpenAL::clean_source( AudioSource * source )
{
	ALint queued = -1;
	
	// if it's currently playing, stop this emitter
	alSourceStop( source->source_id );
	check_al_error( "OpenAL::clean_source - alSourceStop" );
	
	alGetSourcei( source->source_id, AL_BUFFERS_QUEUED, &queued );
	check_al_error( "OpenAL::clean_source - alGetSourcei (AL_BUFFERS_QUEUED)" );
	
	alSourceUnqueueBuffers( source->source_id, queued, source->buffers );
	check_al_error( "OpenAL::clean_source - alSourceUnqueueBuffers" );
	
	
	// check to see if the buffers are indeed buffers
	if ( !alIsBuffer( source->buffers[0] ) )
	{
		LOGV( "OpenAL::clean_source - emitter->buffers[0] is not a buffer!\n" );
	}
	if ( !alIsBuffer( source->buffers[1] ) )
	{
		LOGV( "OpenAL::clean_source - emitter->buffers[1] is not a buffer!\n" );
	}
	
	alDeleteBuffers( AUDIO_EMITTER_BUFFERS, &source->buffers[0] );
	check_al_error( "OpenAL::clean_source - alDeleteBuffers" );
	
	alDeleteSources( 1, &source->source_id );
	check_al_error( "OpenAL::clean_source - alDeleteSources" );
} // clean_source