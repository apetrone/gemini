//
//  ViewController.h
//  test
//
//  Created by Adam Petrone on 1/26/12.

#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>

@interface ViewController : GLKViewController
{
	void * kernel;
}

-(void) setKernel:(void*) kernel_instance;

@end
