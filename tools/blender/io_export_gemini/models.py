# -------------------------------------------------------------
# Copyright (C) 2011- Adam Petrone

# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM,OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
# -------------------------------------------------------------

from mathutils import Matrix, Vector



class Material(object):
	def __init__(self, index, path):
		self.index = index
		self.path = path

	def __eq__(self, other):
		return self.path == other.path

	def __lt__(self, other):
		return self.index < other.index or self.path < other.path

class ExporterConfig(object):
	def __init__(self):
		self.material_index = 0
		self.materials = {}
		self.material_list = []

	def get_filepath_for_material(self, bmaterial):
		""" determine an image path from a blender material """
		filepath = None
		if bmaterial and bmaterial.active_texture:
			if hasattr(bmaterial.active_texture, "image") and hasattr(bmaterial.active_texture.image, "filepath"):
				filepath = bmaterial.active_texture.image.filepath
		return filepath

	def add_material(self, bmaterial):
		filepath = self.get_filepath_for_material(bmaterial)
		if filepath:
			if filepath not in self.materials:
				material = Material(self.material_index, filepath)
				self.materials[ filepath ] = material
				self.material_list.append(material)
				self.material_index += 1

				#print( "Added new material (%i, %s)" % (material.index, material.path) )
				return material
			else:
				return self.materials[ filepath ]
		else:
			return None

	def find_material(self, bmaterial):
		filepath = self.get_filepath_for_material(bmaterial)
		if filepath and filepath in self.materials:
			return self.materials[filepath]

		return None
		'''
		if material in self.materials:
			return self.materials[ material ]
		else:
			raise Exception( 'Material not found!' )
		'''				

# class HashedVector(Vector):
# 	def _description(self):
# 		return (self.x, self.y, self.z)

# 	def __hash__(self):
# 		return hash( self._description() )

class Vector3(object):
	
	def __init__(self):
		self.x = 0
		self.y = 0
		self.z = 0

	def __cmp__(self, other):
		return float_cmp(self.x, other.x) or float_cmp(self.y, other.y) or float_cmp(self.z, other.z)

	def description(self):
		return (self.x, self.y, self.z)

	def __hash__(self):
		return hash( self.description() )

	def set(self, x, y, z):
		self.x = x
		self.y = y
		self.z = z

	def __eq__(self, other):
		if not hasattr(other, 'description'):
			return False
		return self.description() == other.description()

# A custom vertex class allows us to place these into a dictonary
# and have them compare properly
class Vertex(object):
	def __init__(self):
		self.position = Vector3()
		self.normal = Vector3()
		self.u = 0.0
		self.v = 0.0
		self.material_id = 0

	def description(self):
		return (self.position.x, self.position.y, self.position.z, self.normal.x, self.normal.y, self.normal.z, self.u, self.v, self.material_id)

	def __hash__(self):
		return hash( self.description() )

	def __cmp__(self, other):
		return cmp(self.position, other.position) or cmp(self.normal, other.normal) or float_cmp(self.u, other.u) or float_cmp(self.v, other.v) or cmp(self.material_id, other.material_id)

	def __eq__(self, other):
		if not hasattr(other, 'description'):
			return False
		return self.description() == other.description()

# class ShapeKey(object):
# 	def __init__(self, name):
# 		self.name = name
# 		self.vertices = []

# 	def add_vector(self, vector):
# 		self.vertices.extend([vector.x, vector.y, vector.z])

class Geometry(object):
	def __init__(self):
		self.id = -1
		self.indices = []

		self.positions = []
		self.normals = []
		self.uvs = []

		self.vert_index = 0
		self.verts = {}

		self.highest_index = 0

		self.shape_keys = []

	def add_vertex(self, vertex):
		if vertex not in self.verts:
			self.verts[ vertex ] = self.vert_index
			self.vert_index += 1
		
		return self.verts[ vertex ]

	def add_triangle( self, a, b, c ):
		#print( "addTriangle: [%i, %i, %i]" % (a,b,c) )
		#indices = [self.highest_index+a,self.highest_index+b,self.highest_index+c]
		indices = [a,b,c]
		#print( indices )
		self.indices.extend( indices )


	# def add_shape_key(self, name):
	# 	key = ShapeKey(name=name)
	# 	self.shape_keys.append(key)
	# 	return key

'''
	def recalculateHighestIndex(self):
		print( "-> highestIndex is: %i" % self.highest_index )
		self.highest_index = max(self.indices)
		self.highest_index += 1
		print( "<- highestIndex is: %i" % self.highest_index )
'''
