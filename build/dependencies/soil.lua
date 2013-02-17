solution "soil"
configurations { "debug", "release" }

local target_folder = "lib"

function translate_platform( platform )
	if platform == "x32" then
		return "x86"
	elseif platform == "Native" then
		return "native"
	end
	
	return platform
end

project "soil"
	objdir "obj"
	platforms { "x32", "x64", "native" }
	kind "StaticLib"
	language ("C")
	
	files
	{
		"*.c",	
		"*.h"
	}
	
	includedirs 
	{ 
		"."
	}

	configuration { "linux" }
		flags { "ExtraWarnings" }

	configuration {"macosx"}
		buildoptions { "-fvisibility=hidden", "-fvisibility-inlines-hidden" }	

	configuration{ "windows" }
		defines { "WIN32", "UNICODE", baseDefines }

	for _,platform in ipairs(platforms()) do
		configuration { "debug", platform }
			targetdir (target_folder .. "/" .. translate_platform( platform ) .. "/debug" )
			flags { "Symbols" }
			defines { "DEBUG" }
			
		configuration { "release", platform }
			targetdir (target_folder .. "/" .. translate_platform( platform ) .. "/release" )
			flags { "OptimizeSpeed", "Symbols" }
	end		

