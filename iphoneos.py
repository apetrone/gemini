from gemini_common import BUILD_NAME, BUILD_ROOT, common_dependencies, construct_binpath, dependency_libpath, dependency_path

class gemini_iphoneos(Builder):
	def setup(self, *args, **kwargs):
		builder = kwargs.get( "builder", None )
		target_platform = kwargs.get( "target_platform", None )

		self.build_name = BUILD_NAME
		self.resource_path = "resources/ios/*"
		self.build_name = 'geminiios'

		project = Project( name=self.build_name )
		builder.addProject( project )
		builder.includes = []
		builder.libs = []
		builder.root = BUILD_ROOT

	def config(self, *args, **kwargs):
		driver = kwargs.get( "driver", None )
		builder = kwargs.get( "builder", None )
		host_platform = kwargs.get( "host_platform", None )
		target_platform = kwargs.get( "target_platform", host_platform )
		project = kwargs.get( "project", None )
		params = kwargs.get( "args", None )

		old_arch = params["build_architecture"]
		params["build_architecture"] = 'native'
		logging.info( "binpath: %s" % construct_binpath(params) )
		builder.addOutput( path=construct_binpath(params), name=self.build_name, type=Builder.Bundle )
		params["build_architecture"] = old_arch

	@staticmethod
	def depends( *args, **kwargs ):
		target_platform = kwargs.get( "target_platform", None )
		d = {}
		d['libpath'] = "lib/armv7/{configuration}"
		d['depends_file'] = 'build/deps_iphoneos.lua'
		d['depends_path'] = dependency_path()

		mobile_dependencies = []
		dependency_list = list( set(mobile_dependencies) | set(common_dependencies(target_platform)) )
		d['depends'] = dependency_list

		return d

	def generate(self, *args, **kwargs):
		builder = kwargs.get( "builder", None )
		host_platform = kwargs.get( "host_platform", None )
		target_platform = kwargs.get( "target_platform", host_platform )
		arch_list = builder.arch_list[:]

		if 'armv7' in arch_list:
			arch_list.remove( 'armv7' )

		if 'native' in arch_list:
			arch_list.remove( 'native' )

		if 'Native' not in arch_list:
			arch_list.append( 'Native' )


		premake = Premake4( action=builder.premake_action, file="premake4.lua", platform_list=",".join( arch_list ), indextype="ushort" )

		# this will pass the --ios option through to premake
		premake.ios = True
		premake.run()

	def prebuild(self, *args, **kwargs):
		driver = kwargs.get( "driver", None )
		params = kwargs.get( "args", None )

		params['DEBUG_INFORMATION_FORMAT'] = "dwarf-with-dsym"
		params['LD_DYLIB_INSTALL_NAME'] = "@executable_path/./"

	def postbuild(self, *args, **kwargs):
		driver = kwargs.get( "driver", None )
		params = kwargs.get( "args", None )
		builder = kwargs.get( "builder", None )
		target_platform = kwargs.get( "target_platform", None )

		# ios resources are placed starting at the root of the bundle
		appBundle = os.path.join( builder.destination_output, os.path.pardir )
		destination_resource_path = os.path.abspath( appBundle ) + os.path.sep

		self.resource_path = os.path.abspath( os.path.join( currentWorkingDirectory(), self.resource_path ) )
		#logging.info( "Now to copy build resources... (%s -> %s)" % (self.resource_path, destination_resource_path ) )
		gcp = GlobCopy( src=self.resource_path, dst=destination_resource_path )
		gcp.run()

		"""
		if target_platform == MACOSX:
			source_xib = os.path.abspath( os.path.join(destination_resource_path, "en.lproj", "MainMenu.xib") )
			output_nib = os.path.abspath( os.path.join(destination_resource_path, "en.lproj", "MainMenu.nib") )
			ibtool = IBTool( input=source_xib, output=output_nib )
			ibtool.run()
		"""