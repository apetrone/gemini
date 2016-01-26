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
#include "openal.h"
#include <core/logging.h>
#include "openal_vorbis_decoder.h"


void stb_vorbis_decoder::reset()
{
	_src_data = 0;
	_src_data_length = 0;

}

int stb_vorbis_decoder::decode( void * data, int data_length )
{
#ifdef AENGINE_DEBUG
	if ( !_stream )
	{
		LogError( "Trying to decode with a null stream!\n" );
		return 0;
	}
#endif
	// 2 is a magic number which tells this to use 16-bit data. d'oh
	int bytes_read = stb_vorbis_get_samples_short_interleaved( _stream, _info.channels, (short*)data, data_length/2);
	if ( bytes_read > 0 )
	{
		bytes_read *= _info.channels * 2;
	}

	return bytes_read;
}

int stb_vorbis_decoder::open( unsigned char * data, int data_length )
{
	close();

	reset();
	_src_data = data;
	_src_data_length = data_length;
	if ( !_src_data )
	{
		LOGV( "data is INVALID\n" );
		return 0;
	}

	int error = 0;
	_stream = stb_vorbis_open_memory( _src_data, _src_data_length, &error, 0 );

	if ( !_stream )
	{
		LOGV( "stb_vorbis_decoder::open failed with error: %i\n", error );
		return 0;
	}

	_info = stb_vorbis_get_info( _stream );
	return 1;
}

void stb_vorbis_decoder::close()
{
	if (_stream)
		stb_vorbis_close( _stream );
}

int stb_vorbis_decoder::channels()
{
	return _info.channels;
}

int stb_vorbis_decoder::frequency()
{
	return _info.sample_rate;
}

void stb_vorbis_decoder::rewind()
{
	stb_vorbis_seek_start( _stream );
}
