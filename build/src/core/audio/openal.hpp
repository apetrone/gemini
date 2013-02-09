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

#include "factory.hpp" // for DECLARE_FACTORY_CLASS

#if _WIN32
	#include <al.h>
	#include <alc.h>
#elif LINUX
	#include <AL/al.h>
	#include <AL/alc.h>
#elif __APPLE__
	#include <OpenAL/al.h>
	#include <OpenAL/alc.h>
#endif



class OpenAL : public audio::IAudioDriver
{
	DECLARE_FACTORY_CLASS( OpenAL, audio::IAudioDriver );
	ALCdevice * device;
	ALCcontext * context;
	
	int check_alc_error();
	int check_al_error();
	const char * source_state_to_string( int source_state );
	
public:
	OpenAL();
	~OpenAL();
	
	virtual void update();
	virtual void event( audio::EventType event );
	virtual void stop_all_sounds();
	virtual void resume();
}; // OpenAL