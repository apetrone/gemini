import argparse
import os
from glob import iglob
import json

# material definition specific constants
PARAMS_BLOCK_KEY	= "params"
MATERIAL_GLOB		= "*.material"

# stolen from my blacksmith tool
class AttributeStore(object):
	def __init__(self, *initial_data, **kwargs):
		for dictionary in initial_data:
			for key in dictionary:
				setattr(self, key, dictionary[key])
		for key in kwargs:
			setattr(self, key, kwargs[key])

	def __iter__(self):
		for i in self.__dict__.items():
			yield i

	def __str__(self):
		return str(self.__dict__)


def json_to_dict( path ):
	with open(path, "rb") as file:
		json_dict = json.load( file )
		return json_dict

def collect_materials( *args, **kwargs ):
	config = kwargs.get( "config", None )
	search_path = os.path.join( config.abs_target_path, MATERIAL_GLOB )
	materials = []

	for src_file_path in iglob( search_path ):
		material_def = json_to_dict( src_file_path )
		materials.append( material_def )

	return materials

if __name__ == "__main__":
	p = argparse.ArgumentParser()
	p.add_argument('-t', '--target-path', dest='target_path', help="Path to assets/materials/ directory", required=True)
	p.add_argument('-p', '--permutations', dest='permutations_file', help="Path to shader_permutations.conf", required=True)
	args = p.parse_args()

	config = AttributeStore()
	config.target_path = args.target_path
	config.abs_target_path = os.path.abspath( args.target_path )
	config.abs_permutations_file = os.path.abspath( args.permutations_file )


	# collect all materials from the asset folder
	materials = collect_materials(config=config)

	uniforms = set()
	for material in materials:
		if PARAMS_BLOCK_KEY in material:
			for k,v in material[PARAMS_BLOCK_KEY].iteritems():
				uniforms.add( k )



	shader_permutations = json_to_dict( config.abs_permutations_file )
	
	# cross uniforms set with 

	uniform_permutations = shader_permutations['uniforms']
	#print( uniform_permutations )

	print( "unique uniform parameters" )
	print( uniforms )
	# for uniform in uniforms:
	# 	print( "uniform: %s" % uniform )
	# 	permutation = uniform_permutations[ uniform ]
	# 	print( "\t%s" % permutation )

	unused_uniforms = []
	used_attributes = []
	for uniform_name, data in shader_permutations[ "uniforms" ].iteritems():
		print( uniform_name )
		if not uniform_name in uniforms:
			unused_uniforms.append( uniform_name )
		elif "requires" in data:
			used_attributes.extend( data["requires"] )


	print( used_attributes )

	for uniform_name in unused_uniforms:
		#print( "removing: %s" % uniform_name )
		del shader_permutations["uniforms"][ uniform_name ]


	#print( shader_permutations )

