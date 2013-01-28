//
//  ViewController.m
//  test
//
//  Created by Adam Petrone on 1/26/12.

#import "ViewController.h"
//#import <ios_kernel.hpp>
//using namespace aengine;


@interface ViewController () {
}
@property (strong, nonatomic) EAGLContext *context;
@end

@implementation ViewController

@synthesize context = _context;

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	//NSLog(@"touchesBegan");
	//NSLog( @"# Touches: %i", [[touches allObjects] count] );

	for( int i = 0; i < [touches count]; ++i )
	{
		UITouch * t = [[touches allObjects] objectAtIndex: i];
		CGPoint pt = [t locationInView: [self view]];		
//		kernel_touch_start( i, (int)pt.x, (int)pt.y );		
	}

	//NSLog( @"Point: %g, %g", pt.x, pt.y );
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
	//NSLog(@"touchesMoved");	
	
	for( int i = 0; i < [touches count]; ++i )
	{
		UITouch * t = [[touches allObjects] objectAtIndex: i];
		CGPoint pt = [t locationInView: [self view]];		
//		kernel_touch_drag( i, (int)pt.x, (int)pt.y );		
	}
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	//NSLog(@"touchesEnded");
	for( int i = 0; i < [touches count]; ++i )
	{
		UITouch * t = [[touches allObjects] objectAtIndex: i];
		CGPoint pt = [t locationInView: [self view]];		
//		kernel_touch_end( i, (int)pt.x, (int)pt.y );
	}
}

- (void)viewDidLoad
{
	NSLog( @"ViewController.m - viewDidLoad" );
	// the default is 30FPS
	self.preferredFramesPerSecond = 60;
	
    [super viewDidLoad];
    self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
	
	// shared context
	//self.context2 = [[EAGLContext alloc] initWIthAPI:kEAGLRenderingAPIOpenGLES2 sharegroup: [self.context sharegroup]];

    if (!self.context) {
        NSLog(@"Failed to create ES context");
    }
    
    GLKView *view = (GLKView *)self.view;

    view.context = self.context;
    view.drawableDepthFormat = GLKViewDrawableDepthFormat24;
	
	// allow multiple touches; "By default, a view ignores all but the first touch during a multitouch sequence."
	view.multipleTouchEnabled = YES;
	
    [EAGLContext setCurrentContext:self.context];
	
	// ios6
	// this is set on a per-view basis.

//	[self setWantsBestResolutionOpenGLSurface: YES];
//	NSRect pixelBounds = [[self view] convertRectToBacking:[ [self view ]bounds] ];
}

- (void)viewDidUnload
{	
	NSLog( @"ViewController.m - viewDidUnload" );
    [super viewDidUnload];
    
    [EAGLContext setCurrentContext:self.context];
    
    if ([EAGLContext currentContext] == self.context) {
        [EAGLContext setCurrentContext:nil];
    }
	self.context = nil;
}

- (void)didReceiveMemoryWarning
{
//	NSLog( @"ViewController.m - didReceiveMemoryWarning" );
//	kernel_ios_memoryWarning();
    [super didReceiveMemoryWarning];
    // Release any cached data, images, etc. that aren't in use.
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
//	return kernel_ios_shouldChangeOrientation( interfaceOrientation );
	return YES;
}


- (void)update
{
	//NSLog( @"ViewController.m - update" );
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
//	kernel_tick();
}


@end
