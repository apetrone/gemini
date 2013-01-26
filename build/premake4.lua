newoption {
	trigger = "platform_list",
	value = nil,
	description = "Comma separated list of platforms to build for"
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
			"src/osx/*.m*",
		}

		linkoptions
		{
			"-framework Cocoa",
			"-framework OpenGL",
			"-framework AudioToolbox"
		}

		if xcodebuildsettings ~= nil then
			xcodebuildsettings {
				"INFOPLIST_FILE = resources/osx/Info.plist"
			}
		else
			print( "Your version of premake does NOT support xcodebuildsettings!" )
		end
