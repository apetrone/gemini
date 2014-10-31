//
//  main.m
//  test
//
//  Created by Adam Petrone on 1/26/12.
//  Copyright (c) 2012 Nemetschek Vectorworks. All rights reserved.
//

#import <UIKit/UIKit.h>

#import "AppDelegate.h"

#include <gemini/typedefs.h>
#include "kernel_desktop.h"

int main(int argc, char *argv[])
{
	memory::startup();
	kernel::parse_commandline(argc, argv);
		
	@autoreleasepool
	{
	    return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
	}
}
