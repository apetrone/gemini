class squirrel3Builder(Builder): 
	def setup(self, *args, **kwargs):
		builder = kwargs.get( "builder", None )
		host_platform = kwargs.get( "host_platform", None )
		target_platform = kwargs.get( "target_platform", host_platform )

		builder.root = "squirrel3"
		
		squirrel = Project( name="squirrel" )
		# is this still needed?
		if host_platform is WINDOWS:
			squirrel.name = "squirrel3"


		builder.addProject( squirrel )
		
		sqstdlib = Project( name="sqstdlib" )
		builder.addProject( sqstdlib )

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

		builder.includes = ['include', 'sqrat']
		builder.libs = ['squirrel', 'sqstdlib']
		builder.setOutput( path=libdir, name=project.name, type=Builder.StaticLibrary )

		driver.config = (params['configuration'].lower() + Premake4.archmap[ params['platform'] ][ params['build_architecture'] ])
		driver.makefile = project.name + '.make'

	def generate(self, *args, **kwargs):
		builder = kwargs.get( "builder", None )
		premake = Premake4( action=builder.premake_action )
		premake.run()

	def postclean(self, *args, **kwargs):
		params = kwargs.get( "args", None )
		d = Delete(path=os.path.join(params['libpath'], os.path.pardir), directory=True)
		d.run()