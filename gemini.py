class gemini(Builder): 
	def setup(self, platform, builder):
		self.build_name = 'gemini'
		project = Project( name=self.build_name )
		builder.addProject( project )
		builder.includes = []
		builder.libs = []
		builder.root = "build"

	def config(self, builder, driver, project, args):
		bindir = "latest/bin/%s/%s" % (args['build_architecture'], args['configuration'])
		builder_type = Builder.Binary

		# create a bundle in osx
		#if builder.platform is MACOSX:
		#	builder_type = Builder.Bundle

		builder.setOutput( path=bindir, name=self.build_name, type=builder_type )

		driver.config = (args['configuration'].lower() + Premake4.archmap[ args['platform'] ][ args['build_architecture'] ])
		driver.makefile = "%s.make" % (self.build_name)

	@staticmethod
	def depends():
		d = {}
		d['libpath'] = "lib/{architecture}/{configuration}"
		d['depends_file'] = 'build/deps.lua'
		d['depends_path'] = 'dependencies'
		d['depends'] = [ "yajl.py" ]
		return d

	def generate(self, builder):
		arch_list = builder.arch_list[:]
		if 'native' in arch_list:
			arch_list.remove( 'native' )
			arch_list.append( 'Native' )

		premake = Premake4( action=builder.premake_action, file="premake4.lua", platform_list=",".join( arch_list ) )
		premake.run()

	def prebuild(self, driver, args):
		args['DEBUG_INFORMATION_FORMAT'] = "dwarf-with-dsym"
		args['LD_DYLIB_INSTALL_NAME'] = "@executable_path/./"