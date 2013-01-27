newoption {
	trigger = "platform_list",
	value = nil,
	description = "Comma separated list of platforms to build for"
}

newoption {
	trigger = "ios",
	value = nil,
	description = "Enables iOS target (requires Xcode4)"
}

local platform_strings = "Native"
if _OPTIONS["platform_list"] ~= nil then
	platform_strings = string.explode( _OPTIONS["platform_list"], "," )
end

local build_name = "gemini"
solution ( build_name )
	configurations { "debug", "release" }
	platforms { platform_strings  }
common = loadfile( "common.lua" )
common()

project ( build_name )
	objdir "obj"
	uuid( "883b1310-0ec3-11e1-be50-0800200c9a66" )	
	kind "WindowedApp"
	--kind "ConsoleApp"
	language ("C++")

	files
	{
		common_file_list,
		--"src/samples/**.c*",
		--"src/buildinfo.c",
		--"src/kernels/**.c*"
	}

	includedirs 
	{ 
		common_include_dirs,
		"src"
	}

	prebuildcommands
	{
		--"python tools/buildinfo.py generate -g -o src/buildinfo.c"
	}

	setup_platforms( solution() )

	configuration { "windows" }
		defines { "WIN32" }
		files
		{
			common_file_list[ "windows" ],
			"resources/windows/resources.rc",
			"resources/windows/resource.h"
		}

		includedirs
		{
			"resources/windows/"
		}


	configuration { "linux" }
		defines { "LINUX=1" }
		links { "X11", "GL" }
		files { common_file_list[ "linux" ] }

	configuration { "macosx" }
		defines { "__MACH__" }
		files
		{ 
			common_file_list[ "macosx" ],
			"src/*.m*",
			"src/osx/*.m*"
		}



		if xcodebuildsettings ~= nil then

			if _OPTIONS["ios"] ~= nil then
				-- ios needs an application bundle
				kind "WindowedApp"

				linkoptions
				{
					"-framework UIKit",
					"-framework OpenGLES",
					"-framework AudioToolbox",
					"-framework Foundation",
					"-framework CoreFoundation",
					"-framework QuartzCore"
				}
				xcodebuildsettings {
					'INFOPLIST_FILE = "resources/ios/Info.plist"',
					'CODE_SIGN_IDENTITY = "iPhone Developer"',
					'SDKROOT = iphoneos',
					'ARCHS = "$(ARCHS_STANDARD_32_BIT)"',
					'TARGETED_DEVICE_FAMILY = "1,2"',
					'VALID_ARCHS = "armv7 armv7s"',
					'SUPPORTED_PLATFORMS = "iphoneos iphonesimulator"',
					'STANDARD_C_PLUS_PLUS_LIBRARY_TYPE = dynamic',
				}

				defines { "ARM_NEON_GCC_COMPATIBILITY" }
			else
				linkoptions
				{
					"-framework Cocoa",
					"-framework OpenGL",
					"-framework AudioToolbox"
				}
				xcodebuildsettings {
					"INFOPLIST_FILE = resources/osx/Info.plist"
				}
			end
		else
			print( "Your version of premake does NOT support xcodebuildsettings!" )
		end
