class assimpBuilder(Builder): 
	def setup(self, *args, **kwargs):
		builder = kwargs.get( "builder", None )
		host_platform = kwargs.get( "host_platform", None )
		target_platform = kwargs.get( "target_platform", host_platform )

		builder.root = "assimp"

		assimp = Project( name="assimpD" )
		builder.addProject( assimp )


	def config(self, *args, **kwargs):
		driver = kwargs.get( "driver", None )
		builder = kwargs.get( "builder", None )
		host_platform = kwargs.get( "host_platform", None )
		target_platform = kwargs.get( "target_platform", host_platform )
		project = kwargs.get( "project", None )
		params = kwargs.get( "args", None )


		libdir = "lib/%s/%s" % (params['architecture'], params['configuration'])

		if params['platform'] is LINUX:
			items = ['lib']
			items.append( params['build_architecture'] )
			items.append( params['configuration'] )
			libdir = '/'.join( items )

		builder.includes = ['include']
		builder.libs = ['assimpD']
		
		output_type = Builder.StaticLibrary
		output_name = project.name

		if target_platform is LINUX:
			pass
		elif target_platform is MACOSX:
			output_type = Builder.DynamicLibrary
			#if params['configuration'] == "debug":
			#driver.config = (params['configuration'].lower() + Premake4.archmap[ params['platform'] ][ params['build_architecture'] ])
			#driver.makefile = project.name + '.make'
		#params['valid_archs'] = "armv7" #params['build_architecture']
		#params['sdkroot'] = 'iphoneos'

		builder.addOutput( path=libdir, name=output_name, type=output_type )

	def generate(self, *args, **kwargs):
		builder = kwargs.get( "builder", None )
		target_platform = kwargs.get( "target_platform", None )
		#premake = Premake4( action=builder.premake_action )
		#premake.run()

		gen = "None"
		if target_platform is WINDOWS:
			gen = "vs2010"
		elif target_platform is LINUX:
			gen = "Unix Makefiles"
		elif target_platform is MACOSX:
			gen = "Xcode"
		elif target_platform is IPHONEOS:
			gen = "Xcode"

		# build without boost
		cmake = CMake( generator = gen, defines = "-DASSIMP_ENABLE_BOOST_WORKAROUND=ON" )
		cmake.run()

	def postclean(self, *args, **kwargs):
		params = kwargs.get( "args", None )
		d = Delete(path=os.path.join(params['libpath'], os.path.pardir), directory=True)
		d.run()