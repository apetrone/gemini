-- where binaries will be placed
binary_folder = "latest/bin"
target_folder = binary_folder

function translate_platform( platform )
	if platform == "x32" then
		return "x86"
	elseif platform == "Native" then
		return "native"
	end

	return platform
end

common_file_list =
{
	-- We don't want all subfolders; each platform is defined for additional files below

	"src/*.c*",
	"src/*.h*",

	--[[
	"src/thirdparty/*.c",
	"src/thirdparty/*.h*",
	--"dependencies/murmur3/murmur3.c",
	--"dependencies/jsoncpp/jsoncpp.cpp",
	"dependencies/simplefilewatcher/source/FileWatcher.cpp",

	windows=
	{
		"src/windows/*.c*",
		"src/windows/*.h*",
		"dependencies/simplefilewatcher/source/FileWatcherWin32.cpp"
	},

	linux=
	{
		"src/linux/*.c*",
		"src/linux/*.h*",
		"dependencies/simplefilewatcher/source/FileWatcherLinux.cpp"
	},
	macosx=
	{
		"src/osx/*.c*",
		"src/osx/*.h*",
		"dependencies/simplefilewatcher/source/FileWatcherOSX.cpp"
	}
	--]]
}

common_include_dirs=
{
	"src",
	--[[
	--"src/prototypes",
	"src/thirdparty",
	--"dependencies/murmur3",
	--"dependencies/jsoncpp",
	--"dependencies/miniz/",
	"resources",
	"dependencies/glm",
	"dependencies/simplefilewatcher/include"
	00]]
}

tool_excludes =
{
	"src/entry.cpp"
}

common_flags =
{
	debug =
	{
		"Symbols"
	},
	release=
	{
		"Symbols",
		"Optimize"
	}	
}

common_defines=
{
	debug=
	{
		"DEBUG",
	},
	release=
	{
	}
}



function setup_platforms( solution )
	deps = loadfile( "deps.lua" )
	deps()

	platform_table = solution.platforms
	config_table = solution.configurations
	for _,platform in ipairs( platform_table ) do
		for _,config in ipairs( config_table ) do
			-- make sure these are placed in the appropriate config/platform combo
			configuration { platform, config }
				local trplat = translate_platform( platform )
				local target_dir = target_folder .. "/" .. trplat .. "/" .. config
				targetdir( target_dir )
				flags { common_flags[ config ] }
				defines { common_defines[ config ] }

				includedirs { common_include_dirs, dependency_includes[ trplat ][ config ] }
				libdirs { dependency_libdirs[ trplat ][ config ] }
				links { dependency_libs[ trplat ][ config ] }
				linkoptions { dependency_linkoptions[ trplat ][ config ] }
		end
	end
end
