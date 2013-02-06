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
#import "gemgl.h"
#import <CoreFoundation/CoreFoundation.h>

CFBundleRef _gl_bundle;
	
int gemgl_osx_startup( void )
{
	_gl_bundle = 0;
	
	// get bundle ref
#if TARGET_OS_IPHONE
	_gl_bundle = CFBundleGetBundleWithIdentifier( CFSTR( "com.apple.opengles" ) );
#else
	_gl_bundle = CFBundleGetBundleWithIdentifier( CFSTR( "com.apple.opengl" ) );
#endif
	if ( !_gl_bundle )
	{
		printf( "FATAL ERROR: Unable to get OpenGL bundle ref.\n" );
		return 0;
	}
	
	return 1;
} // osx_gemgl_startup
	
void gemgl_osx_shutdown( void )
{
	if ( _gl_bundle )
	{
		CFRelease( _gl_bundle );
		_gl_bundle = 0;
	}
} // osx_gemgl_shutdown
	
void * gemgl_native_findsymbol( const char * name )
{
	CFStringRef symbol_name = CFStringCreateWithCString( kCFAllocatorDefault, name, kCFStringEncodingASCII );
	void * symbol = CFBundleGetFunctionPointerForName( _gl_bundle, symbol_name );
	CFRelease( symbol_name );
	return symbol;
} // native_gl_findsymbol

