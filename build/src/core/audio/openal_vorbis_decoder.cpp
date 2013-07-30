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
#include <slim/xlog.h>
#include "openal_vorbis_decoder.hpp"


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