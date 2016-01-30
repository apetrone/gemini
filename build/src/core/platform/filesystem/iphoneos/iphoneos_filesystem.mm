// -------------------------------------------------------------
// Copyright (C) 2012- Adam Petrone
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//      * Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.

//      * Redistributions in binary form must reproduce the above copyright notice,
//      this list of conditions and the following disclaimer in the documentation
//      and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//       SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------
#include <gemini/typedefs.h>
#include "filesystem.h"
#include <slim/xstr.h>
#include "platform.h"
#include <stdio.h> // for printf
#include <sys/stat.h> // for fs::FileExists
#include "xfile.h"
#include <slim/xlog.h>
#include "memory.h"

#include <CoreFoundation/CoreFoundation.h>
#include <AudioToolbox/AudioToolbox.h>
#include <AudioToolbox/ExtendedAudioFile.h>
#include <UIKit/UIKit.h>

NSString * getBundleStringForLocalPath( const char * filename )
{
	NSString * nsfilename = [NSString stringWithCString:filename encoding:NSUTF8StringEncoding];
	NSString * path = [[NSBundle mainBundle] pathForResource: [nsfilename stringByDeletingPathExtension] ofType: [nsfilename pathExtension]];
	return path;
}

bool FileExists( const char * filename, bool path_is_relative )
{
	return [[NSFileManager defaultManager] fileExistsAtPath: getBundleStringForLocalPath(filename)];
}

NSURL * getURLForLocalPath( const char * filename )
{

	NSString * nsfilename = [NSString stringWithCString:filename encoding:NSUTF8StringEncoding];
	NSString * resource_path = [[NSBundle mainBundle] pathForResource:[nsfilename stringByDeletingPathExtension] ofType:[nsfilename pathExtension]];

	if ( resource_path )
	{
		return [NSURL fileURLWithPath:resource_path];
	}

	return 0;
}


namespace fs
{


	void * mobile_audio_file_to_buffer( const char * filename, size_t & buffer_length )
	{
		unsigned char * ptr;
		AudioFileID fileID;
		if ( !FileExists( filename, false ) )
			return 0;
		NSURL * url = getURLForLocalPath( filename );
		CFURLRef fileURL = (CFURLRef)url;

		//http://stackoverflow.com/questions/6673051/audiotoolbox-openal-extaudiofile-to-play-compressed-audio
		OSStatus result = AudioFileOpenURL( fileURL, kAudioFileReadPermission, 0, &fileID );
		if ( result != noErr )
		{
			printf( "failed to open %s\n", filename );
		}

		UInt32 fileSize = 0;
		UInt32 propertySize = sizeof(UInt64);
		AudioFileGetProperty( fileID, kAudioFilePropertyAudioDataByteCount, &propertySize, &fileSize );

		buffer_length = fileSize;
		ptr = (unsigned char*)ALLOC( fileSize );

		AudioFileReadBytes(fileID, false, 0, &fileSize, ptr );
		AudioFileClose( fileID );

		// finished with this
//		CFRelease( fileURL );

		return ptr;
	}
}; // namespace fs
