class openALBuilder(Builder): 
	def setup(self, *args, **kwargs):
		builder = kwargs.get( "builder", None )
		host_platform = kwargs.get( "host_platform", None )
		target_platform = kwargs.get( "target_platform", host_platform )

		p = Project( name="openal" )
		builder.addProject( p )
		builder.root = "openal-1.1"

	def config(self, *args, **kwargs):
		driver = kwargs.get( "driver", None )
		builder = kwargs.get( "builder", None )
		host_platform = kwargs.get( "host_platform", None )
		target_platform = kwargs.get( "target_platform", host_platform )
		project = kwargs.get( "project", None )
		params = kwargs.get( "args", None )

		libdir = None
		if target_platform is LINUX:
			builder.libs = ['openal']
		elif target_platform is MACOSX:
			builder.linkoptions = ['-framework OpenAL']
		elif target_platform is WINDOWS:
			builder.libs = ['OpenAL32']
			builder.includes = ['include']
			libmap = {
				'x86' : 'Win32',
				'x64' : 'Win64',
				'native' : 'Win32'
			}
			libdir = "libs/%s" % libmap[ params['build_architecture'] ]

			builder.addOutput( path=libdir, name=builder.libs[0], type=Builder.StaticLibrary )