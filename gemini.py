from gemini_common import BUILD_NAME, BUILD_ROOT, common_dependencies, construct_binpath, dependency_libpath, dependency_path

class gemini(Builder):
	def setup(self, *args, **kwargs):
		builder = kwargs.get( "builder", None )
		target_platform = kwargs.get( "target_platform", None )

		self.build_name = BUILD_NAME
		self.resource_path = "resources/osx/*"

		project = Project( name=self.build_name )
		builder.addProject( project )
		builder.includes = []
		builder.libs = []
		builder.root = BUILD_ROOT

		self.builder_type = DefaultDict( Builder.Binary )
		self.builder_type[ MACOSX ] = Builder.Bundle

	def config(self, *args, **kwargs):
		driver = kwargs.get( "driver", None )
		builder = kwargs.get( "builder", None )
		host_platform = kwargs.get( "host_platform", None )
		target_platform = kwargs.get( "target_platform", host_platform )
		project = kwargs.get( "project", None )
		params = kwargs.get( "args", None )

		builder.addOutput( path=construct_binpath( params ), name=self.build_name, type=self.builder_type[ target_platform ] )

		if host_platform is LINUX:
			driver.config = (params['configuration'].lower() + Premake4.archmap[ params['platform'] ][ params['build_architecture'] ])
			driver.makefile = "%s.make" % (self.build_name)

	@staticmethod
	def depends( *args, **kwargs ):
		target_platform = kwargs.get( "target_platform", None )
		d = {}
		d['libpath'] = dependency_libpath()
		d['depends_file'] = 'build/deps.lua'
		d['depends_path'] = dependency_path()

		target_platform = kwargs.get( "target_platform", None )

		desktop_dependencies = list( set(common_dependencies(target_platform)) | set(["xwl/xwl.py", "assimp.py"]) )
		d['depends'] = desktop_dependencies
		return d

	def generate(self, *args, **kwargs):
		builder = kwargs.get( "builder", None )
		host_platform = kwargs.get( "host_platform", None )
		target_platform = kwargs.get( "target_platform", host_platform )
		arch_list = builder.arch_list[:]

		if 'native' in arch_list:
			arch_list.remove( 'native' )

		if 'Native' not in arch_list:
			arch_list.append( 'Native' )


		premake = Premake4( action=builder.premake_action, file="premake4.lua", platform_list=",".join( arch_list ) )
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

		if target_platform == MACOSX:
			if target_platform == MACOSX:
				appBundle = os.path.join( builder.destination_output, os.path.pardir, os.path.pardir )
				destination_resource_path = os.path.abspath( os.path.join( appBundle, "Resources" ) )	

			self.resource_path = os.path.abspath( os.path.join( currentWorkingDirectory(), self.resource_path ) )
			#logging.info( "Now to copy build resources... (%s -> %s)" % (self.resource_path, destination_resource_path ) )
			gcp = GlobCopy( src=self.resource_path, dst=destination_resource_path )
			gcp.run()

			if target_platform == MACOSX:
				source_xib = os.path.abspath( os.path.join(destination_resource_path, "en.lproj", "MainMenu.xib") )
				output_nib = os.path.abspath( os.path.join(destination_resource_path, "en.lproj", "MainMenu.nib") )
				ibtool = IBTool( input=source_xib, output=output_nib )
				ibtool.run()

		elif target_platform == LINUX:
			target_symlink = os.path.join(params["output_path"], "libassimp.so.3")

			# delete the symlink first
			d = Delete(path=target_symlink)
			d.run()

			ln = LNTool(source='libassimp.so', target=target_symlink)
			ln.run()
