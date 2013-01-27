class gemini(Builder):
	def setup(self, *args, **kwargs):
		builder = kwargs.get( "builder", None )

		self.build_name = 'gemini'
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

		driver.config = (params['configuration'].lower() + Premake4.archmap[ params['platform'] ][ params['build_architecture'] ])
		driver.makefile = "%s.make" % (self.build_name)

	@staticmethod
	def depends():
		d = {}
		d['libpath'] = "lib/{architecture}/{configuration}"
		d['depends_file'] = 'build/deps.lua'
		d['depends_path'] = 'build/dependencies'
		d['depends'] = [ "xwl/xwl.py" ]
		return d

	def generate(self, *args, **kwargs):
		builder = kwargs.get( "builder", None )
		host_platform = kwargs.get( "host_platform", None )
		target_platform = kwargs.get( "target_platform", host_platform )

		arch_list = builder.arch_list[:]
		if 'native' in arch_list:
			arch_list.remove( 'native' )
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