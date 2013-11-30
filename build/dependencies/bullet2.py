class bullet2Builder(Builder): 
	def setup(self, *args, **kwargs):
		builder = kwargs.get( "builder", None )
		host_platform = kwargs.get( "host_platform", None )
		target_platform = kwargs.get( "target_platform", host_platform )

		project_names = {
			LINUX : 'bullet',
			MACOSX : 'BULLET_PHYSICS',
			WINDOWS : 'BULLET_PHYSICS'
		}

		builder.root = "bullet2"
		
		libs = [
			"BulletCollision",		
			"BulletDynamics",
			"BulletSoftBody",
			"LinearMath"]

		builder.targets = [
			'BulletCollision',
			'BulletDynamics',
			'BulletSoftBody',
			'LinearMath']

		i = 0
		for lib in libs:
			p = Project( name=project_names[ target_platform ] )
			p.output = libs[ i ]
			i += 1
			builder.addProject( p )

		if target_platform is LINUX:
			builder.file_path = "./msvc/gmake"	

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

		builder.includes = ['src']
		builder.libs = [ project.output ]

		print( "%s -> %s" % ( project.name, project.output ) )
		builder.addOutput( path=libdir, name=project.output, type=Builder.StaticLibrary )

		#if target_platform is LINUX:
		#	driver.config = (params['configuration'].lower() + Premake4.archmap[ params['platform'] ][ params['build_architecture'] ])
		#	driver.makefile = project.name + '.make'
		#params['valid_archs'] = "armv7" #params['build_architecture']
		#params['sdkroot'] = 'iphoneos'


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

		if target_platform is LINUX:
			self.cd = ChangeDirectory(path=params['file_path'])
			self.cd.run()
		params['file'] = "./%s.make" % (params['targets'][ params['project_id'] ])

		params['config'] = params['configuration'].lower() + params['architecture']


	def postbuild(self, *args, **kwargs):
		params = kwargs.get( "args", None )
		if params['platform'] is LINUX:
			self.cd.pop()

	def postclean(self, *args, **kwargs):
		params = kwargs.get( "args", None )

		d = Delete(path=params['libpath'], directory=True)
		d.run()