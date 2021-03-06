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
#import "AppDelegate.h"
#import "ViewController.h"
//#import <ios_kernel.hpp>
//#include <log.h>
#include "kernel_ios.h"
#include "memory.h"



#if ENABLE_TESTFLIGHT
	#import <TestFlight.h>
	NSString * ARCFUSION_TEAM_TOKEN = @"22e405dfbe5ee70fc2e60842fa78f089_Nzg5NTcyMDEyLTA0LTA4IDEzOjU3OjMxLjQ2ODQ1NQ";

extern "C"
{
	void tflogger_message( log_handler_t * handler, const char * message, const char * filename, const char * function, int line, int type );
	int tflogger_open( log_handler_t * handler );
	void tflogger_close( log_handler_t * handler );

	void tflogger_message( log_handler_t * handler, const char * message, const char * filename, const char * function, int line, int type )
	{
		TFLog( @"%s | %s:%i", message, filename, line );
	}

	int tflogger_open( log_handler_t * handler )
	{
		// disable the Apple System Log
		[TestFlight setOptions:[NSDictionary dictionaryWithObject:[NSNumber numberWithBool:NO] forKey:@"logToConsole"]];

		// disable stderr log
		[TestFlight setOptions:[NSDictionary dictionaryWithObject:[NSNumber numberWithBool:NO] forKey:@"logToSTDERR"]];

		return 1;
	}

	void tflogger_close( log_handler_t * handler )
	{

	}
};
#endif



@implementation AppDelegate

@synthesize window = _window;
@synthesize viewController = _viewController;

-(void)device_orientation_changed:(NSNotification*)notification
{
	NSLog( @"device_orientation_changed" );
	iOSKernel * mobile_kernel = (iOSKernel*)self->kernel;
	assert( mobile_kernel != 0 );
	if ( mobile_kernel )
	{
		mobile_kernel->device_orientation_changed( [self.viewController interfaceOrientation] );
	}
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    self.window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];

    // Override point for customization after application launch.
	if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone) {
	    self.viewController = [[ViewController alloc] initWithNibName:@"ViewController_iPhone" bundle:nil];
	} else {
	    self.viewController = [[ViewController alloc] initWithNibName:@"ViewController_iPad" bundle:nil];
	}
	self.window.rootViewController = self.viewController;
    [self.window makeKeyAndVisible];


#if ENABLE_TESTFLIGHT
	// prevent anonymous users by setting this before calling 'takeOff'
	#if ENABLE_TESTFLIGHT_UNIQUEID
		#pragma clang dianostic push
		#pragma clang diagnostic ignored "-Wdeprecated-declarations"
		[TestFlight setDeviceIdentifier:[[UIDevice currentDevice] uniqueIdentifier]];
		#pragma clang diagnostic pop
	#endif

	// launch test flight with team token
	[TestFlight takeOff:ARCFUSION_TEAM_TOKEN];

#endif

	NSLog( @"AppDelegate.m - didFinishLaunchingWithOptions" );

	// startup memory subsystem
	memory::startup();

	// allocate and assign the kernel to an ivar
	iOSKernel * mobile_kernel = CREATE(iOSKernel);
	self->kernel = mobile_kernel;

	// startup the kernel instance
	if ( kernel::startup( mobile_kernel ) != kernel::NoError )
	{
		NSLog( @"kernel startup failed!" );
	}
	else
	{
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(device_orientation_changed:) name:UIDeviceOrientationDidChangeNotification object: nil];

		// set our view size
		mobile_kernel->set_view_size( [[self.viewController view] bounds].size.width, [[self.viewController view] bounds].size.height );
		[self.viewController setKernel: mobile_kernel];
	}

	//UIInterfaceOrientation initialOrientation = [self.viewController interfaceOrientation];
	//[[UIApplication sharedApplication] setStatusBarOrientation: UIInterfaceOrientationPortrait animated:NO];
    return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application
{
	/*
	 Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
	 Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
	 */

	NSLog( @"AppDelegate.m - applicationWillResignActive" );
	// after this is 'DidEnterBackground'

	iOSKernel * mobile_kernel = (iOSKernel*)self->kernel;
	if ( mobile_kernel )
	{
		mobile_kernel->will_resign_active();
	}
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
	/*
	 Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
	 If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
	 */
	NSLog( @"AppDelegate.m - applicationDidEnterBackground" );
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
	/*
	 Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
	 */

	NSLog( @"AppDelegate.m - applicationWillEnterForeground" );
	// after this is 'DidBecomeActive'
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
	/*
	 Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
	 */
	NSLog( @"AppDelegate.m - applicationDidBecomeActive" );
	iOSKernel * mobile_kernel = (iOSKernel*)self->kernel;
	if ( mobile_kernel )
	{
		mobile_kernel->did_become_active();
	}
}

- (void)applicationWillTerminate:(UIApplication *)application
{
	/*
	 Called when the application is about to terminate.
	 Save data if appropriate.
	 See also applicationDidEnterBackground:.
	 */

	NSLog( @"AppDelegate.m - applicationWillTerminate" );
	iOSKernel * mobile_kernel = (iOSKernel*)self->kernel;
	if ( mobile_kernel )
	{
		mobile_kernel->will_terminate();
	}

	iOSKernel * kernel_pointer = (iOSKernel*)self->kernel;
	DESTROY( iOSKernel, kernel_pointer );
	self->kernel = 0;

	memory::shutdown();
}

@end





