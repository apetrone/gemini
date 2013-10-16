class Box2DBuilder(Builder):
	def setup(self, *args, **kwargs):
		builder = kwargs.get( "builder", None )
		host_platform = kwargs.get( "host_platform", None )
		target_platform = kwargs.get( "target_platform", host_platform )



		project_name = 'box2d'
		if host_platform is LINUX:
			project_name = 'Box2D'	

		box2d = Project( name=project_name )
		builder.addProject( box2d )

		builder.root = "box2d"
		builder.includes = ['.']
		builder.libs = [ project_name ]
		builder.targets = ["Box2D"]

	def config(self, *args, **kwargs):
		driver = kwargs.get( "driver", None )
		builder = kwargs.get( "builder", None )
		host_platform = kwargs.get( "host_platform", None )
		target_platform = kwargs.get( "target_platform", host_platform )
		project = kwargs.get( "project", None )
		params = kwargs.get( "args", None )

		libdir = "lib/%s/%s" % (params['architecture'], params['configuration'])

		if host_platform is LINUX:
			items = ['lib']
			items.append( params['build_architecture'] )
			items.append( params['configuration'] )
			libdir = '/'.join( items )
			libdir = "Box2D"
		#elif host_platform is MACOSX:
			#project.name = os.path.join( "Build/xcode4", project.name )

		#driver.config = (params['configuration'].lower() + Premake4.archmap[ params['platform'] ][ params['build_architecture'] ])
		builder.addOutput( path=libdir, name=project.name, type=Builder.StaticLibrary )

	def generate(self, *args, **kwargs):
		builder = kwargs.get( "builder", None )
		host_platform = kwargs.get( "host_platform", None )
		target_platform = kwargs.get( "target_platform", host_platform )
		params = kwargs.get( "args", None )

		generator_name = "Unix Makefiles"

		if host_platform is MACOSX:
			generator_name = "Xcode"
		elif host_platform is WINDOWS:
			generator_name = "Visual Studio 2010"
		
		cm = CMake( generator=generator_name )
		cm.run()

	def prebuild(self, *args, **kwargs):
		params = kwargs.get( "args", None )
		md = Makedirs(path=params['libpath'])
		md.run()

	def postclean(self, *args, **kwargs):
		params = kwargs.get( "args", None )
		d = Delete(path=os.path.join(params['libpath'], os.path.pardir), directory=True)
		d.run()