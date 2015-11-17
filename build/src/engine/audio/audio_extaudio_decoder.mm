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
#include "audio_extaudio_decoder.h"
#include <slim/xlog.h>
#include <slim/xstr.h>

using namespace audio;

ExtAudioDecoder::ExtAudioDecoder()
{

}

ExtAudioDecoder::~ExtAudioDecoder()
{
	//free( _src_data );
	_src_data = 0;
	_src_data_length = 0;
}

void ExtAudioDecoder::reset()
{
	_src_data = 0;
	_src_data_length = 0;
	_sampleRate = 0;
	_numChannels = 0;
	_currentOffset = 0;
}

int ExtAudioDecoder::decode( void * data, int data_length )
{
	// requested read would go past data
	if ( (data_length + _currentOffset) >= _src_data_length )
	{
		data_length = (_src_data_length-_currentOffset);
	}

	//printf( "(max: %i) read offset: %i, length: %i\n", _src_data_length, _currentOffset, data_length );
	if ( data_length > 0 )
	{
		memcpy( data, ((unsigned char*)_src_data)+(_currentOffset), data_length );
		_currentOffset += data_length;
	}

	//printf( "data_length %i bytes.\n", data_length );
	return data_length;
}

int ExtAudioDecoder::open( unsigned char * data, int data_length )
{
	reset();
	_src_data = data;
	_src_data_length = data_length;

	return 1;
}

void ExtAudioDecoder::close()
{
	//stb_vorbis_close( _stream );
	reset();
}

int ExtAudioDecoder::channels()
{
	return _numChannels;
}

int ExtAudioDecoder::frequency()
{
	return _sampleRate;
}

void ExtAudioDecoder::rewind()
{
	//stb_vorbis_seek_start( _stream );
	_currentOffset = 0;
}
