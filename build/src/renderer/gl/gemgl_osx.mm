// -------------------------------------------------------------
// Copyright (C) 2004-2011 Adam Petrone

//	This file is part of the aengine source code.
//
//  aengine is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.

//  This source is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//  GNU General Public License for more details.

//  You should have received a copy of the GNU General Public License
//  along with this source.  If not, see <http://www.gnu.org/licenses/>.
// -------------------------------------------------------------
#import <CoreFoundation/CoreFoundation.h>

#import "gemgl.h"

namespace gemini
{
	CFBundleRef gemgl_bundle = 0;
		
	int gemgl_osx_startup( void )
	{
		gemgl_bundle = 0;
		
		// get bundle ref
#if TARGET_OS_IPHONE
		gemgl_bundle = CFBundleGetBundleWithIdentifier( CFSTR( "com.apple.opengles" ) );
#else
		gemgl_bundle = CFBundleGetBundleWithIdentifier( CFSTR( "com.apple.opengl" ) );
#endif
		if ( !gemgl_bundle )
		{
			printf( "FATAL ERROR: Unable to get OpenGL bundle ref.\n" );
			return 0;
		}
		
		// following The Get Rule
		CFRetain(gemgl_bundle);
		
		return 1;
	} // gemgl_osx_startup
		
	void gemgl_osx_shutdown( void )
	{
		if ( gemgl_bundle )
		{
			CFRelease( gemgl_bundle );
			gemgl_bundle = 0;
		}
	} // gemgl_osx_shutdown
		
	void * gemgl_native_findsymbol( const char * name )
	{
		CFStringRef symbol_name = CFStringCreateWithCString( kCFAllocatorDefault, name, kCFStringEncodingASCII );
		void * symbol = CFBundleGetFunctionPointerForName( gemgl_bundle, symbol_name );
		CFRelease( symbol_name );
		return symbol;
	} // gemgl_native_findsymbol

} // namespace gemini