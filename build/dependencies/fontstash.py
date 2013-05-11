class font_stash(Builder):
	def setup(self, *args, **kwargs):
		builder = kwargs.get( "builder", None )
		target_platform = kwargs.get( "target_platform", None )

		p = Project( name="Font-Stash" )
		builder.addProject( p )
		builder.root = "Font-Stash"
		builder.includes = ['.']
		builder.libs = []

	def config(self, *args, **kwargs):
		driver = kwargs.get( "driver", None )
		builder = kwargs.get( "builder", None )
		host_platform = kwargs.get( "host_platform", None )
		target_platform = kwargs.get( "target_platform", host_platform )
		project = kwargs.get( "project", None )
		params = kwargs.get( "args", None )

		#builder.setOutput( path=construct_binpath( params ), name=self.build_name, type=self.builder_type[ target_platform ] )

	def generate(self, *args, **kwargs):
		builder = kwargs.get( "builder", None )
		host_platform = kwargs.get( "host_platform", None )
		target_platform = kwargs.get( "target_platform", host_platform )
		arch_list = builder.arch_list[:]

	def prebuild(self, *args, **kwargs):
		driver = kwargs.get( "driver", None )
		params = kwargs.get( "args", None )

	def postbuild(self, *args, **kwargs):
		driver = kwargs.get( "driver", None )
		params = kwargs.get( "args", None )
		builder = kwargs.get( "builder", None )
		target_platform = kwargs.get( "target_platform", None )