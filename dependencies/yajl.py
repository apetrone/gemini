class yajl(Builder): 
	def setup(self, platform, builder):
		self.build_name = 'yajl'
		project = Project( name="YetAnotherJSONParser" )
		builder.addProject( project )
		builder.includes = [ "yajl-2.0.5/include" ]
		builder.libs = ["yajl"]
		builder.root = "yajl"
		builder.targets = ["yajl"]

	def config(self, builder, driver, project, args):
		outdir = "lib/%s/%s" % (args['architecture'], args['configuration'])
		builder.setOutput( path=outdir, name="yajl", type=Builder.DynamicLibrary )

		
		#driver.makefile = "%s.make" % (self.build_name)

	def postbuild(self, builder, driver, args,):
		if args["platform"] is MACOSX:
			# make the id relative (strip the full path from it)
			install_name_tool = InstallNameTool( command="id", name=builder.output_name, input=builder.destination_output )
			install_name_tool.run()

	def generate(self, builder):
		generator_name = None
		if builder.platform is LINUX:
			generator_name = "Unix Makefiles"
		elif builder.platform is MACOSX:
			generator_name = "Xcode"
		elif builder.platform is WINDOWS:
			generator_name = "vs2010"		
		cmake = CMake( generator=generator_name )
		cmake.run()
