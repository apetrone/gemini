class gemini(Builder):
	def setup(self, *args, **kwargs):
		builder = kwargs.get( "builder", None )
		target_platform = kwargs.get( "target_platform", None )

		self.build_name = 'gemini'

		if target_platform == MACOSX:
			self.resource_path = "resources/osx/icon.icns"
		elif target_platform == IPHONEOS:
			self.resource_path = "resources/ios/*"
			self.build_name = 'geminiios'

		project = Project( name=self.build_name )
		builder.addProject( project )
		builder.includes = []
		builder.libs = []
		builder.root = "build"

		self.builder_type = DefaultDict( Builder.Binary )
		self.builder_type[ MACOSX ] = Builder.Bundle
		self.builder_type[ IPHONEOS ] = Builder.Bundle

	def config(self, *args, **kwargs):
		driver = kwargs.get( "driver", None )
		builder = kwargs.get( "builder", None )
		host_platform = kwargs.get( "host_platform", None )
		target_platform = kwargs.get( "target_platform", host_platform )
		project = kwargs.get( "project", None )
		params = kwargs.get( "args", None )

		bindir = "latest/bin/%s/%s" % (params['build_architecture'], params['configuration'])

		builder.setOutput( path=bindir, name=self.build_name, type=self.builder_type[ target_platform ] )

		if host_platform is LINUX:
			driver.config = (params['configuration'].lower() + Premake4.archmap[ params['platform'] ][ params['build_architecture'] ])
			driver.makefile = "%s.make" % (self.build_name)

	@staticmethod
	def depends( *args, **kwargs ):
		target_platform = kwargs.get( "target_platform", None )
		d = {}
		d['libpath'] = "lib/{architecture}/{configuration}"
		d['depends_file'] = 'build/deps.lua'
		d['depends_path'] = 'build/dependencies'

		common_dependencies = [ "soil.py" ] #[ "squirrel3.py", "sqrat.py" ]
		mobile_dependencies = common_dependencies
		desktop_dependencies = list( set(common_dependencies) | set(["xwl/xwl.py"]) )

		if target_platform == IPHONEOS:
			d['depends'] = mobile_dependencies
		else:
			d['depends'] = desktop_dependencies
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


		premake = Premake4( action=builder.premake_action, file="premake4.lua", platform_list=",".join( arch_list ) )
		if target_platform == IPHONEOS:
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

		if target_platform == IPHONEOS or target_platform == MACOSX:
			
			if target_platform == IPHONEOS:
				# ios resources are placed starting at the root of the bundle
				appBundle = os.path.join( builder.destination_output, os.path.pardir )
				destination_resource_path = os.path.abspath( appBundle ) + os.path.sep
			elif target_platform == MACOSX:
				appBundle = os.path.join( builder.destination_output, os.path.pardir, os.path.pardir )
				destination_resource_path = os.path.abspath( os.path.join( appBundle, "Resources" ) )

			self.resource_path = os.path.abspath( os.path.join( currentWorkingDirectory(), self.resource_path ) )
			#logging.info( "Now to copy build resources... (%s -> %s)" % (self.resource_path, destination_resource_path ) )
			gcp = GlobCopy( src=self.resource_path, dst=destination_resource_path )
			gcp.run()
