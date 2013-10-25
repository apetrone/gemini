newoption {
	trigger = "platform_list",
	value = nil,
	description = "Comma separated list of platforms to build for"
}

newoption {
	trigger = "ios",
	value = nil,
	description = "Enables iOS target (requires Xcode4; experimental)"
}

newoption {
	trigger = "iphonesimulator",
	value = nil,
	description = "Enables iphone simulator target"
}

newoption {
	trigger = "rpi",
	value = nil,
	description = "Compile for Raspberry Pi"
}

newoption {
	trigger = "indextype",
	value = "uint",
	description = "Set the rendering index type. (Available types are: uint, ushort)"
}

newoption {
	trigger = "with_glesv2",
	value = nil,
	description = "Compile with OpenGL ES 2.0"
}


local platform_strings = "Native"
local build_name = "gemini"
local RASPBERRYPI = false
local IndexTypeDefine = "PLATFORM_INDEX_TYPE"
local INDEX_TYPE = nil
local WITH_GLESV2 = false

if _OPTIONS["with_glesv2"] ~= nil then
	WITH_GLESV2 = true
end

if _OPTIONS["platform_list"] ~= nil then
	platform_strings = string.explode( _OPTIONS["platform_list"], "," )
end

-- update build name for ios
if _OPTIONS["ios"] ~= nil then
	build_name = "geminiios"
	WITH_GLESV2 = true
end

if _OPTIONS["iphonesimulator"] ~= nil then
	build_name = "geminisimulator"
	WITH_GLESV2 = true
end

-- set the index type define
if _OPTIONS["indextype"] == nil or _OPTIONS["indextype"] == "uint" then
	INDEX_TYPE = IndexTypeDefine .. "=1"
elseif _OPTIONS["indextype"] == "ushort" then
	INDEX_TYPE = IndexTypeDefine .. "=2"
end

-- building for RaspberryPi
if _OPTIONS["rpi"] ~= nil then
	RASPBERRYPI = true
	WITH_GLESV2 = true
	-- override index type to use ushort
	INDEX_TYPE = IndexTypeDefine .. "=2"
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
		"src/core/gldrivers/opengl_common.*",
		"src/core/audio/openal.*",
		"src/core/assets/*.*",
		"src/contrib/*",
		"src/game/**.*",
	}

	excludes
	{
		"src/kernels/test_bullet2.cpp"
	}

	-- building for desktop
	if _OPTIONS["ios"] == nil then
		files
		{
			"src/core/desktop/kernel_desktop.cpp",
			"src/core/audio/openal_vorbis_decoder.*",
		}

		if not WITH_GLESV2 then
			files
			{
				"src/core/gldrivers/opengl_core32.*"
			}
		end

		includedirs
		{
			"src/core/audio/"
		}

		if not RASPBERRYPI then
			prebuildcommands
			{
				"python ../tools/blacksmith/blacksmith.py -c ../assets/desktop.conf -y"
			}
		else
			includedirs
			{
				"/opt/vc/include"
			}

			prebuildcommands
			{
				"python ../tools/blacksmith/blacksmith.py -c ../assets/raspberrypi.conf -y"
			}
		end
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


	local deps_name = "deps.lua"
	if _OPTIONS["ios"] ~= nil then
		deps_name = "deps_iphoneos.lua"
	end

	setup_platforms( solution(), deps_name )

	configuration { "windows" }
		

		defines { "WIN32", INDEX_TYPE }
		files
		{
			"resources/win32/resources.rc",
			"resources/win32/resource.h",
			"src/core/desktop/entry.cpp",
			common_file_list[ "windows" ],			
		}

		links
		{
			"OpenGL32"
		}

		includedirs
		{
			"resources/windows/",
			"src/core/win32" -- for glext.h, wglext.h
		}


	configuration { "linux" }
		defines { "LINUX=1", INDEX_TYPE }

		-- GCC 4.3+ supports "-std=c++0x"
		-- GCC 4.7+ supports "-std=c++11"
		-- buildoptions { "-std=c++0x" }

		if RASPBERRYPI then
			defines { "PLATFORM_IS_RASPBERRYPI=1" }
		end

		if WITH_GLESV2 then
			files
			{
				"src/core/gldrivers/opengl_glesv2.*",
			}

			defines { "PLATFORM_USE_GLES2=1" }
		else
			links
			{
				"GL"
			}			
		end

		links { "pthread", "dl", "openal" }
		if not RASPBERRYPI then
			-- need X11 on Linux, non RaspberryPi builds.
			links { "X11" }
		end


		files
		{
			"src/core/desktop/entry.cpp",
			common_file_list[ "linux" ],
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

				defines { INDEX_TYPE }

				files
				{
					"src/core/ios/*.m*",
					"src/core/ios/*.h*",
					"src/core/osx/osx_platform.*",
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

				defines { INDEX_TYPE }
				
				linkoptions
				{
					"-framework Cocoa",
					"-framework OpenGL",
					"-framework AudioToolbox",
					"-framework OpenAL"
				}
				xcodebuildsettings
				{
					'INFOPLIST_FILE = resources/osx/Info.plist',
					-- 'CLANG_CXX_LANGUAGE_STANDARD = "c++0x"',
					-- 'CLANG_CXX_LIBRARY = "libc++"'
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
