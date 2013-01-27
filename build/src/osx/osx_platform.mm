#include "osx_platform.h"

#if TARGET_OS_MAC && !TARGET_OS_IPHONE
	#import <Cocoa/Cocoa.h>
#endif

namespace platform
{
	NSAutoreleasePool * pool;
	
	core::Error osx_startup()
	{
		pool = [[NSAutoreleasePool alloc] init];
		return core::Error(0);
	}
	
	void osx_shutdown()
	{
		[pool release];
	}
	
	core::Error osx_programDirectory( char * path, size_t size )
	{
		core::Error error(0);
		NSString * bundle_path = [[NSBundle mainBundle] bundlePath];
		if (bundle_path)
		{
			[bundle_path getCString:path maxLength:size encoding:NSUTF8StringEncoding];
		}
		else
		{
			error = core::Error( core::Error::Failure, "Unable mainBundle reference is invalid!" );
		}
		return error;
	}
};