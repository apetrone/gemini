class glmBuilder(Builder): 
	def setup(self, *args, **kwargs):
		builder = kwargs.get( "builder", None )

		builder.root = "glm"
		builder.addProject( Project(name="glm") )
		builder.includes = ['.']

	def config(self, *args, **kwargs):
		pass	

	def generate(self, *args, **kwargs):
		pass