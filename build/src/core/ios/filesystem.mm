// -------------------------------------------------------------
// Copyright (C) 2012- Adam Petrone

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
#include "filesystem.hpp"
#include <slim/xstr.h>
#include "platform.hpp"
#include <stdio.h> // for printf
#include <sys/stat.h> // for fs::FileExists
#include "xfile.h"
#include "log.h"
#include "memory.hpp"

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

	
	void * mobile_audio_file_to_buffer( const char * filename, int & buffer_length )
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
