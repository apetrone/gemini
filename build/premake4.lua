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

if _OPTIONS["rpi"] ~= nil then
	print( "Building for the Raspberry Pi" )
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
		"src/kernels/**.c*",
		"src/core/*.*",
		"src/core/audio/openal.*",
		"src/contrib/*",

		"src/game/**.*",
	}

	-- building for desktop
	if _OPTIONS["ios"] == nil then
		files
		{
			"src/core/desktop/kernel_desktop.cpp",
			"src/core/gldrivers/opengl_core32.*",
			"src/core/audio/openal_vorbis_decoder.*",
		}

		includedirs
		{
			"src/core/audio/"
		}
		prebuildcommands
		{
			"python ../tools/blacksmith/blacksmith.py -c ../assets/desktop.conf -y"
		}

	else
		prebuildcommands
		{
			"python ../tools/blacksmith/blacksmith.py -c ../assets/ios.conf -y"
		}

		files
		{
			"src/core/audio/audio_extaudio_decoder.*"
		}
	end

	includedirs 
	{ 
		common_include_dirs,
		"src",
		"src/core/",
		"src/core/audio",
		"src/contrib",
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
			"resources/windows/resource.h",
			"src/core/desktop/entry.cpp",
		}

		links
		{
			"OpenGL32",
			"OpenAL"
		}

		includedirs
		{
			"resources/windows/"
		}


	configuration { "linux" }
		defines { "LINUX=1" }
		links { "X11", "GL", "pthread", "dl", "openal" }
		files
		{
			common_file_list[ "linux" ],
			"src/core/desktop/entry.cpp",
		}

		-- set rpath to be the same directory as the binary
		linkoptions
		{
			"-Wl,-rpath,."
		}

	configuration { "macosx" }
		defines { "__MACH__" }
		files
		{ 
			common_file_list[ "macosx" ],
			"src/core/osx/osx_gemgl.*",			
		}

		if xcodebuildsettings ~= nil then

			if _OPTIONS["ios"] ~= nil then
				-- ios needs an application bundle
				kind "WindowedApp"

				files
				{
					"src/core/ios/*.m*",
					"src/core/ios/*.h*",
					"src/core/osx/osx_platform.*",
					"src/core/osx/osx_entry.mm",
					"src/core/gldrivers/opengl_glesv2.*",
					"src/core/osx/osx_gemgl.*",
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
					'VALIDATE_PRODUCT = NO',
					'CODE_SIGN_ENTITLEMENTS = "resources/ios/Entitlements.plist"',
				}

			else
				linkoptions
				{
					"-framework Cocoa",
					"-framework OpenGL",
					"-framework AudioToolbox",
					"-framework OpenAL"
				}
				xcodebuildsettings
				{
					"INFOPLIST_FILE = resources/osx/Info.plist"
				}

				files
				{
					"src/core/osx/*.m*",
					"src/core/osx/*.h*",
				}
			end
		else
			print( "Your version of premake does NOT support xcodebuildsettings!" )
		end
