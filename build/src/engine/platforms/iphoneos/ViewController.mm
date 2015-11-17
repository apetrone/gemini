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
#import "ViewController.h"
#import "kernel_ios.h"
//using namespace aengine;
#import "kernel.h"

@interface ViewController () {
}
@property (assign, nonatomic) EAGLContext *context;
@end

@implementation ViewController

-(void)dealloc
{
	self.context = nil;
	[super dealloc];
}

-(void) setKernel:(void*) kernel_instance
{
	self->kernel = kernel_instance;
}

@synthesize context = _context;

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	//NSLog(@"touchesBegan");
	//NSLog( @"# Touches: %i", [[touches allObjects] count] );

	for( int i = 0; i < [touches count]; ++i )
	{
		UITouch * t = [[touches allObjects] objectAtIndex: i];
		CGPoint pt = [t locationInView: [self view]];
		kernel::TouchEvent ev;
		ev.subtype = kernel::TouchBegin;
		ev.id = i;
		ev.x = (int)pt.x;
		ev.y = (int)pt.y;
		kernel::event_dispatch( ev );
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
		kernel::TouchEvent ev;
		ev.subtype = kernel::TouchMoved;
		ev.id = i;
		ev.x = (int)pt.x;
		ev.y = (int)pt.y;
		kernel::event_dispatch( ev );
	}
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	//NSLog(@"touchesEnded");
	for( int i = 0; i < [touches count]; ++i )
	{
		UITouch * t = [[touches allObjects] objectAtIndex: i];
		CGPoint pt = [t locationInView: [self view]];
		kernel::TouchEvent ev;
		ev.subtype = kernel::TouchEnd;
		ev.id = i;
		ev.x = (int)pt.x;
		ev.y = (int)pt.y;
		kernel::event_dispatch( ev );
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

    GLKView *view = (GLKView *)self.view;

    view.context = self.context;
    view.drawableDepthFormat = GLKViewDrawableDepthFormat24;

	// allow multiple touches; "By default, a view ignores all but the first touch during a multitouch sequence."
	view.multipleTouchEnabled = YES;


//	view.exclusiveTouch = YES;

    [EAGLContext setCurrentContext:self.context];

	// ios6
	// this is set on a per-view basis.
//	[self setWantsBestResolutionOpenGLSurface: YES];

//	CGRect viewBounds = [view bounds];
//	NSLog( @"view dims: %g x %g", viewBounds.size.width, viewBounds.size.height );

//	NSRect pixelBounds = [[self view] convertRectToBacking:[ [self view ]bounds] ];
//	iOSKernel * mobile_kernel = (iOSKernel*)self->kernel;
//	if ( mobile_kernel )
//	{
//		mobile_kernel->set_backing_size( )
//	}
} // viewDidLoad

- (void)viewDidUnload
{
	NSLog( @"ViewController.m - viewDidUnload" );
    [super viewDidUnload];

    [EAGLContext setCurrentContext:self.context];

    if ([EAGLContext currentContext] == self.context) {
        [EAGLContext setCurrentContext:nil];
    }
	self.context = nil;
} // viewDidUnload

- (void)didReceiveMemoryWarning
{
	iOSKernel * mobile_kernel = (iOSKernel*)self->kernel;
	if ( mobile_kernel )
	{
		return mobile_kernel->did_receive_memory_warning();
	}
    [super didReceiveMemoryWarning];
} // didReceiveMemoryWarning

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
	iOSKernel * mobile_kernel = (iOSKernel*)self->kernel;
	if ( mobile_kernel )
	{
		return mobile_kernel->should_change_orientation( interfaceOrientation );
	}

	return YES;
} // shouldAutorotateToInterfaceOrientation


- (void)update
{
	//NSLog( @"ViewController.m - update" );
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
	kernel::tick();
}


@end
