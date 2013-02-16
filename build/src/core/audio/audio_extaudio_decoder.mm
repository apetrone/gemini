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
#include "audio_extaudio_decoder.hpp"
#include "log.h"
#include "xstr.h"

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
