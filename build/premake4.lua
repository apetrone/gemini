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

if _OPTIONS["ios"] ~= nil then
	build_name = "geminiios"
end

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
		"src/kernels/**.c*"
	}

	-- building for desktop
	if _OPTIONS["ios"] == nil then
		files {
			"src/desktop/*.c*"
		}
	end

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
		links { "X11", "GL", "pthread", "dl" }
		files { common_file_list[ "linux" ] }

	configuration { "macosx" }
		defines { "__MACH__" }
		files
		{ 
			common_file_list[ "macosx" ],
			"src/*.m*"
		}



		if xcodebuildsettings ~= nil then

			if _OPTIONS["ios"] ~= nil then
				-- ios needs an application bundle
				kind "WindowedApp"

				files
				{
					"src/ios/*.m*",
					"src/ios/*.h*",
					"src/osx/osx_platform.*"
				}

				linkoptions
				{
					"-framework UIKit",
					"-framework OpenAL",
					"-framework OpenGLES",
					"-framework AudioToolbox",
					"-framework AVFoundation",
					"-framework Foundation",
					"-framework CoreGraphics",
					"-framework GLKit"
				}
				xcodebuildsettings
				{
					'INFOPLIST_FILE = "resources/ios/Info.plist"',
					'CODE_SIGN_IDENTITY = "iPhone Developer"',
					'SDKROOT = "iphoneos"',
					'ARCHS = "armv7 armv7s"',
					'TARGETED_DEVICE_FAMILY = "1,2"',
					'VALID_ARCHS = "armv7 armv7s"',
					'SUPPORTED_PLATFORMS = "iphonesimulator iphoneos"',
				}

			else
				linkoptions
				{
					"-framework Cocoa",
					"-framework OpenGL",
					"-framework AudioToolbox"
				}
				xcodebuildsettings
				{
					"INFOPLIST_FILE = resources/osx/Info.plist"
				}

				files
				{
					"src/osx/*.m*",
					"src/osx/*.h*"
				}
			end
		else
			print( "Your version of premake does NOT support xcodebuildsettings!" )
		end
