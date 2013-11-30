class bullet2Builder(Builder): 
	def setup(self, *args, **kwargs):
		builder = kwargs.get( "builder", None )
		host_platform = kwargs.get( "host_platform", None )
		target_platform = kwargs.get( "target_platform", host_platform )

		project_names = {
			LINUX : 'bullet',
			MACOSX : 'BULLET_PHYSICS',
			IPHONEOS : 'BULLET_PHYSICS',
			IPHONESIMULATOR : 'BULLET_PHYSICS',
			WINDOWS : 'BULLET_PHYSICS'
		}

		builder.root = "bullet2"
		
		# incorrect link ordering will cause errors on linux
		libs = [
			"BulletDynamics",		
			"BulletCollision",
			"BulletSoftBody",
			"LinearMath"]

		builder.targets = []

		i = 0
		for lib in libs:
			p = Project( name=project_names[ target_platform ] )
			p.output = libs[ i ]
			
			builder.addProject( p )
			builder.targets.append( libs[ i ] )
			i += 1

	def config(self, *args, **kwargs):
		driver = kwargs.get( "driver", None )
		builder = kwargs.get( "builder", None )
		host_platform = kwargs.get( "host_platform", None )
		target_platform = kwargs.get( "target_platform", host_platform )
		project = kwargs.get( "project", None )
		params = kwargs.get( "args", None )

		libdir = "lib/%s/%s" % (params['architecture'], params['configuration'])

		project_output = project.output

		if params['platform'] is LINUX:
			items = ['src']
			items.append( project.output )
			libdir = '/'.join( items )
		elif params['platform'] is WINDOWS:
			project_output = "%s_%s" % (project.output, params['configuration'].title())
			libdir = "lib/%s" % (params['configuration'])

		builder.includes = ['src']
		builder.libs = [ project_output ]

		#print( "%s -> %s" % ( project.name, project.output ) )
		builder.addOutput( path=libdir, name=project_output, type=Builder.StaticLibrary )



	def generate(self, *args, **kwargs):
		builder = kwargs.get( "builder", None )
		target_platform = kwargs.get( "target_platform", None )
		params = kwargs.get( "args", None )

		gen = "None"
		if target_platform is WINDOWS:
			gen = params["cmake_generator"]
		elif target_platform is LINUX:
			gen = "Unix Makefiles"
		elif target_platform is MACOSX:
			gen = "Xcode"
		elif target_platform is IPHONEOS:
			gen = "Xcode"

		cmake = CMake( generator = gen )
		cmake.run()

	def prebuild(self, *args, **kwargs):
		target_platform = kwargs.get( "target_platform", None )
		params = kwargs.get( "args", None )
		md = Makedirs(path=params['libpath'])
		md.run()

		params['file'] = "./Makefile"
		params['config'] = params['configuration'].lower() + params['architecture']

	def postclean(self, *args, **kwargs):
		params = kwargs.get( "args", None )

		d = Delete(path=params['libpath'], directory=True)
		d.run()