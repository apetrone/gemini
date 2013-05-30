class soilBuilder(Builder): 
	def setup(self, *args, **kwargs):
		builder = kwargs.get( "builder", None )
		host_platform = kwargs.get( "host_platform", None )
		target_platform = kwargs.get( "target_platform", host_platform )

		p = Project( name="soil" )
		builder.addProject( p )

		builder.root = "soil"
		builder.libs = [ p.name ]
		builder.includes = ['.']

	def config(self, *args, **kwargs):
		driver = kwargs.get( "driver", None )
		builder = kwargs.get( "builder", None )
		host_platform = kwargs.get( "host_platform", None )
		target_platform = kwargs.get( "target_platform", host_platform )
		project = kwargs.get( "project", None )
		params = kwargs.get( "args", None )

		libdir = "lib/native/%s" % (params['configuration'])

		if params['platform'] is LINUX:
			items = ['lib']
			items.append( params['build_architecture'] )
			items.append( params['configuration'] )
			libdir = '/'.join( items )

		builder.addOutput( path=libdir, name=project.name, type=Builder.StaticLibrary )
		driver.config = (params['configuration'].lower() + Premake4.archmap[ params['platform'] ][ params['build_architecture'] ])

	def generate(self, *args, **kwargs):
		# Generated project files are not consistent even after setting location attribute in premake.
		# In order to get all project files to show up in the same directory, I let it be default (current working directory).
		# So, we copy the premake4 file into the dependency folder and run premake4 on it from there.
		builder = kwargs.get( "builder", None )

		cp = Copy( src="../soil.lua", dst="soil.lua" )
		cp.run()
		premake = Premake4( action=builder.premake_action, file="soil.lua" )
		premake.run()

	def postclean(self, *args, **kwargs):
		params = kwargs.get( "args", None )

		d = Delete(path=os.path.join(params['libpath'], os.path.pardir), directory=True)
		d.run()