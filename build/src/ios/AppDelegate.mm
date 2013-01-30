//
//  AppDelegate.m
//  test
//
//  Created by Adam Petrone on 1/26/12.
//  Copyright (c) 2012. All rights reserved.
//

#import "AppDelegate.h"
#import "ViewController.h"
//#import <ios_kernel.hpp>
//#include <log.h>
#include "kernel_ios.h"



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



-(void)didRotate:(NSNotification*)notification
{
	//NSLog( @"didRotate" );
//	kernel_ios_didRotate( (int)[self.viewController interfaceOrientation] );
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    self.window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
	[[UIApplication sharedApplication] setStatusBarHidden:YES withAnimation:UIStatusBarAnimationFade];
	
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
	
//	KernelCallbacks cb;
//	
//	DECLARE_KERNEL( Audio, cb );	
//	DECLARE_KERNEL( OpenGL, cb );
//	DECLARE_KERNEL( GUI, cb );
//	DECLARE_KERNEL( Test, cb );
//
//	KERNEL( Audio, cb );

	// start generating orientation notifications
	[[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(didRotate:) name:UIDeviceOrientationDidChangeNotification object: nil];

	iOSKernel * mobile_kernel = new iOSKernel();
	self->kernel = mobile_kernel;
	
	// get the current status bar notification and send that to the kernel on startup
	UIInterfaceOrientation startup_orientation = [[UIApplication sharedApplication] statusBarOrientation];
//	kernel_ios_startup( cb, [[self.viewController view] bounds].size.width, [[self.viewController view] bounds].size.height, startup_orientation );	
	if ( kernel::startup( mobile_kernel, "TestMobile" ) != kernel::NoError )
	{
		NSLog( @"kernel startup failed!" );
	}
	else
	{
		mobile_kernel->setInterfaceOrientation( startup_orientation );
		
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
//	kernel_resign_active();
	[[UIDevice currentDevice] endGeneratingDeviceOrientationNotifications];
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
	[[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
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
	
	delete (iOSKernel*)self->kernel;
	self->kernel = 0;
	
}

@end





