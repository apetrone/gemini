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

int OpenAL::check_al_error()
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

void OpenAL::update()
{
	
} // update

void OpenAL::event( audio::EventType event )
{
	if ( event == audio::DetachContext )
	{
		
	}
	else if ( event == audio::ReattachContext )
	{
	
	}
} // event

void OpenAL::stop_all_sounds()
{
	
} // stop_all_sounds

void OpenAL::resume()
{
	
} // resume