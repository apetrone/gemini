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

bl_info = {
	"name": "gemini exporter",
	"author": "Adam Petrone",
	"version": (0,0,1),
	"blender": (2, 6, 3),
	"api": 39307,
	"location": "Location",
	"description": "Exporter for gemini engine",
	"warning": "",
	"wiki_url": "",
	"tracker_url": "",
	"category": "Import-Export"
}

#
# imports
# 

from io_export_gemini.models import (
	ExporterConfig,
	Geometry,
	Material,
	Vertex
)

# handle module reloads [mainly used for development]
if "bpy" in locals():
	import importlib
	if "io_export_gemini" in locals():
		importlib.reload(io_export_gemini)
		importlib.reload(io_export_gemini.models)

import bpy
from bpy.props import (
	StringProperty,
	BoolProperty,
	FloatProperty,
	EnumProperty
)
from time import time
from mathutils import (
	Matrix,
	Vector
)

import math
import operator
import json
import os
import platform

#
# utility functions
#
BMeshAware = False

def float_clamp(value):
       if abs(value) < 0.0001:
               return 0.0
       return value

def float_equal(value0, value1):
       if abs(value0-value1) < 0.0000001:
               return True
       return False

def float_cmp(value0, value1):
       if abs(value0-value1) < 0.0000001:
               return 0
       return -1

def checkBMeshAware():
	global BMeshAware

	print( "Blender r%s" % bpy.app.build_revision )
	BMeshAware = (int(bpy.app.build_revision) > 44136)

# def get_host_platform():
# 	"""
# 		Retrieves the host platform
# 	"""
# 	platform_name = platform.platform().lower()
# 	if "linux" in platform_name:
# 		return "linux"
# 	elif "darwin" in platform_name:
# 		return "macosx"
# 	elif "nt" or "windows" in platform_name:
# 		return "windows"
# 	else:
# 		return "unknown"

def getMeshFaces( mesh ):
	# for BMesh compatability
	# http://blenderartists.org/forum/showthread.php?251840-Bmesh-Where-did-the-UV-coords-go&p=2097099&viewfull=1#post2097099
	if hasattr(mesh, 'polygons'):
		mesh.calc_tessface()
		mesh_faces = mesh.tessfaces
	else:
		mesh_faces = mesh.faces

	return mesh_faces

def create_triangulated_mesh(config, object):

	# enter object mode in blender
	#bpy.ops.object.mode_set( mode='OBJECT' )

	# de-select all objects
	for i in bpy.context.scene.objects:
		i.select = False

	# de-seect object
	object.select = False

	# set active object to this one
	bpy.context.scene.objects.active = object

	# set object mode
	#bpy.ops.object.mode_set( mode='OBJECT' )

	mesh_faces = getMeshFaces( object.data )

	quads = [f for f in mesh_faces if len(f.vertices) == 4]

	
	if quads:
		#print( "This mesh will be converted to Triangles..." )
		# make a copy of the object and data
		data = object.data.copy()
		obj = object.copy()
		obj.data = data
		obj.name = object.name + '_triangulated'

		# link copied object into the scene
		bpy.context.scene.objects.link( obj )

		# de-select all objects
		for i in bpy.context.scene.objects:
			i.select = False

		# select the copied object and make it active
		obj.select = True
		bpy.context.scene.objects.active = obj

		# enter EDIT mode on this object
		bpy.ops.object.mode_set( mode='EDIT' )

		# select all components
		bpy.ops.mesh.select_all( action='SELECT' )
		
		# convert quads to tris using Blender's internal method
		bpy.ops.mesh.quads_convert_to_tris()

		# update scene
		bpy.context.scene.update()

		# go back to object mode
		bpy.ops.object.mode_set( mode='OBJECT' )

		#print( "Triangulated Mesh", obj.name )

		return obj
	else:	   
		#print( "No need to convert mesh to triangles." )
		return object

"""
class export_animation(bpy.types.Operator):
	bl_idname = "gemini_export.animation"
	bl_label = "Export gemini .animation"

	UseVertexAnimation = BoolProperty(
		name="Use Vertex Animation",
		description="Exports vertex animations",
		default=True)

	TimelineBegin = IntProperty(
		name="Animation Begin",
		description="First frame of animation to export",
		default=0)

	TimelineEnd = IntProperty(
		name="Animation End",
		description="Last frame of animation to export",
		default=1)

	def execute(self, context):
		return {'FINISHED'}

	def invoke(self, context, event):
		#print( 'Invoke' )
		wm = context.window_manager
		wm.fileselect_add(self)
		return {'RUNNING_MODAL'}
"""



#
# supporting classes
#

class Node(object):
	NODE = "node"
	ROOT = "root"
	MESH = "mesh"

	def __init__(self, **kwargs):
		self.type = kwargs.get("type", Node.NODE)
		self.children = kwargs.get("children", [])

	def add_child(self, child):
		self.children.append(child)

	def to_json(self):
		return json.dumps(self, default=lambda o: o.__dict__,
			sort_keys=True, indent='\t')

class RootNode(Node):
	def __init__(self, **kwargs):
		super(RootNode, self).__init__(**kwargs)

		self.type = Node.ROOT
		self.materials = kwargs.get("materials", [])
		self.skeleton = kwargs.get("skeleton", [])

	def reconcile_materials(self, config):
		material_id = 0
		for material in config.material_list:
			if material.path:
				material_name = os.path.splitext(
					os.path.basename(material.path))[0]
				data = {
					'id': material_id,
					'name': material_name
				}
				material_id += 1
				self.materials.append(data)

class Mesh(Node):
	def __init__(self, **kwargs):
		super(Mesh, self).__init__(**kwargs)

		self.name = kwargs.get("name", "UnnamedNode")
		self.type = Node.MESH
		self.mins = [0.0, 0.0, 0.0]
		self.maxs = [0.0, 0.0, 0.0]
		self.mass_center_offset = [0.0, 0.0, 0.0]

	@staticmethod
	def from_object(config, obj, root):
		node = Mesh(name=obj.name)

		# from Marmalade plugin; convert Z up to Y up.
		xrotation = Matrix.Rotation(-math.pi/2, 4, 'X')

		# convert object rotation to quaternion (this ignores scale, apparently)
		rotation = obj.matrix_world.to_quaternion()

		# convert this to a 3x3 rotation matrix
		rotation_matrix = rotation.to_matrix().to_3x3()	
		final_rotation = xrotation.to_3x3() * rotation_matrix
		#scale = obj.matrix_world.to_scale()
		#translation = obj.matrix_world.to_translation()

		triangulated_object = create_triangulated_mesh(config, obj)

		# apply rotation and scale transform to the object
		# this ensures that any modifications are baked to the vertices
		# before we output them to the file
		bpy.context.scene.objects.active = obj
		output = bpy.ops.object.transform_apply(location=True,
			scale=True)

		mesh = obj.to_mesh(bpy.context.scene, True, 'PREVIEW')
		mesh.transform(final_rotation.to_4x4())
		
		# collect all blender materials used by this mesh
		for bmaterial in mesh.materials:
			config.add_material(bmaterial)

		# remove the triangulated object we created
		bpy.ops.object.mode_set(mode='OBJECT')
		bpy.context.scene.objects.unlink(triangulated_object)

		return node

class MeshContainer(object):
	
	def __init__(self, config):
		self.geometry = {}
		self.materials = {}
		self.material_list = []
		self.name = ""
		self.translate = []
		self.rotate = []
		self.config = config

		self.geometry_index = 0
		self.material_index = 0

	def getFilepathForMaterial(self, material):
		filepath = None
		if material and material.active_texture:
			if hasattr(material.active_texture, "image") and hasattr(material.active_texture.image, "filepath"):
				filepath = material.active_texture.image.filepath
		return filepath

	def addMaterial(self, material):
		filepath = self.getFilepathForMaterial( material )
		if filepath:
			if filepath not in self.materials:
				aematerial = Material( self.material_index )
				aematerial.setMaterial( filepath )
				self.materials[ filepath ] = aematerial
				self.material_list.append( aematerial )
				self.material_index += 1

				print( "Added new material (%i, %s)" % (aematerial.index, aematerial.path) )
				return aematerial
			else:
				return self.materials[ filepath ]
		else:
			return None

	def findMaterial(self, material):
		filepath = self.getFilepathForMaterial( material )
		if filepath and filepath in self.materials:
			return self.materials[ filepath ]

		return None
		'''
		if material in self.materials:
			return self.materials[ material ]
		else:
			raise Exception( 'Material not found!' )
		'''		
	def findGeometry(self, material_index):
		#print( "findGeometry for material %i" % material_index )

		if material_index in self.geometry:
			return self.geometry[ material_index ]
		else:
			geometry = self.geometry[ material_index ] = Geometry()
			geometry.id = self.geometry_index
			self.geometry_index += 1
			return geometry

	'''
	def recalculateIndices(self):
		for matid, geometry in self.geometry.items():
			geometry.recalculateHighestIndex()
	'''
	def writeFile(self, file, scene_nodes, skeleton):

		# geometry_id = 0
		# for material_id, geometry in self.geometry.items():

		# 	print( "Writing geometry: %i" % geometry_id )
		# 	vertices = []
		# 	normals = []
		# 	uv_sets = []
		# 	shape_keys = []

		# 	getval = operator.itemgetter(0)
		# 	getkey = operator.itemgetter(1)
		# 	vert_sorted = map(getval, sorted(geometry.verts.items(), key=getkey))

		# 	uvs = []

		# 	mins = []
		# 	maxs = []

		# 	vertex_colors = []
		# 	blend_weights = []
		# 	bind_pose = []

		# 	for v in vert_sorted:
		# 		vertices.extend( [float_clamp(v.position.x), float_clamp(v.position.y), float_clamp(v.position.z)] )
		# 		normals.extend( [v.normal.x, v.normal.y, v.normal.z] )
		# 		uvs.extend( [v.u, v.v] )

		# 	for key in geometry.shape_keys:
		# 		shape_data = {}
		# 		shape_data["name"] = key.name
		# 		shape_data["data"] = key.vertices
		# 		shape_keys.append(shape_data)

		# 	# only setup one uv_set for now.
		# 	uv_sets.append(uvs)

		# 	geometry_data = {
		# 		'material_id': material_id, 
		# 		'indices': geometry.indices, 
		# 		'vertices': vertices, 
		# 		'normals' : normals, 
		# 		'uv_sets' : uv_sets,
		# 		# 'shape_keys': shape_keys,
		# 		'mins' : mins,
		# 		'maxs' : maxs,
		# 		'vertex_colors' : vertex_colors,
		# 		'blend_weights' : blend_weights,
		# 		'bind_pose' : bind_pose
		# 	}
		# 	root['geometry'].append( geometry_data )
		# 	geometry_id += 1

		material_list = []
		material_id = 0
		for material in self.material_list:
			if material.path:
				material_name = os.path.splitext( os.path.basename( material.path ) )[0]
				material_list.append( {'name': material_name } )
			material_id += 1

		# setup root node

		# root['materials'] = material_list
		# root['nodes'] = node_list
		# root['skeleton'] = skeleton

		# root_node = RootNode(materials=material_list, skeleton=skeleton)
		# root_node.export_info = {
		# 	'blender_version': ("%i.%i.%i" % (bpy.app.version[0], bpy.app.version[1], bpy.app.version[2])),
		# 	'host_platform': platform.platform().lower(),
		# 	'source_file': bpy.data.filepath
		# }

		# root_node.children = scene_nodes

		# file.write(root_node.to_json())


#
# main classes
#

class export_gemini(bpy.types.Operator):
	'''Export Skeleton Mesh / Animation Data file(s)'''
	bl_idname = "gemini_export.test" # this is important since its how bpy.ops.export.udk_anim_data is constructed
	bl_label = "Export gemini .model"
	# List of operator properties, the attributes will be assigned
	# to the class instance from the operator settings before calling.

	CoordinateSystems = (
		("2", "Right-Handed", ""),
		#("1", "Left-Handed", "")
	)

	CoordinateSystem = EnumProperty(
		name="System",
		description="Select a coordinate system to export to",
		items=CoordinateSystems,
		default="2")

	ExportRaw = BoolProperty(
		name="Export raw geometry",
		description="This is a description",
		default=True)

	filepath = StringProperty(
			name="File Path",
			description="Filepath used for exporting the file",
			maxlen= 1024,
			subtype='FILE_PATH',
			)	
	
	TransformNormals = BoolProperty(
		name="Transform Normals",
		description="Transform Normals along with Mesh",
		default=True)

	triangulate_mesh = BoolProperty(
		name="Triangulate Mesh",
		description="Convert Quad to Tri Mesh Boolean...",
		default=False)

	def execute(self, context):
		global unique_vertices
		global last_vertex
		#print( 'Execute!' )

		self.config = ExporterConfig()
		self.config.ExportRaw = self.ExportRaw
		self.config.TransformNormals = self.TransformNormals
		self.config.triangulate_mesh = self.triangulate_mesh

		if self.config.TransformNormals:
			print( "TransformNormals is enabled." )

		start_time = time()
		
		#print( self.filepath )
		file_failure = False
		try:
			file = open( self.filepath, 'w' )
		except IOError:
			file_failure = True
			import sys
			self.report('ERROR', str(sys.exc_info()[1]) )
			
		selected_meshes = []
		if not file_failure:
			mesh_list = []

			for obj in bpy.context.scene.objects:
				if obj.type == 'MESH':
					mesh_list.append( obj )
					if obj.select:
						selected_meshes.append( obj )
				elif obj.type == 'ARMATURE':
					print( 'TODO: Export Armatures' )
			print( "Total Meshes: ", len(mesh_list)," Selected: ", len(selected_meshes) )


			# clear selection
			bpy.context.scene.objects.active = None

			current_mesh = 0
			total_meshes = len(selected_meshes)


			exportedMesh = MeshContainer(self.config)
			print("Total selected meshes: %i" % len(selected_meshes))




			# We can also generate a skeleton. At the present,
			# there can only be one skeleton.

			root_node = RootNode(materials=[], skeleton=[])
			root_node.export_info = {
				'blender_version': ("%i.%i.%i" % (bpy.app.version[0], bpy.app.version[1], bpy.app.version[2])),
				'host_platform': platform.platform().lower(),
				'source_file': bpy.data.filepath
			}

			# iterate over selected objects
			for obj in selected_meshes:
				meshnode = Mesh.from_object(self.config, obj, root_node)
				root_node.children.append(meshnode)

				current_mesh += 1
				self.report({'INFO'}, 'Export Progress -> %g%%' % ((current_mesh/total_meshes) * 100.0))

				#exportedMesh.recalculateIndices()

			#exportedMesh.writeFile(file, scene_nodes, skeleton)

			root_node.reconcile_materials(self.config)

			file.write(root_node.to_json())

			file.close()

			delta_time = time() - start_time

			result_message = 'Write file "%s" in %2.2f milliseconds' % (self.filepath, delta_time)
			self.report({'INFO'}, result_message)
			
		return {'FINISHED'}
		
	def invoke(self, context, event):
		#print( 'Invoke' )
		wm = context.window_manager
		wm.fileselect_add(self)
		return {'RUNNING_MODAL'}


#
# startup / blender specific interface functions
#
def menu_func(self, context):
	self.layout.operator(export_gemini.bl_idname, text="gemini .model")
	#self.layout.operator(export_animation.bl_idname, text="gemini .animation")

def register():   
	bpy.utils.register_module(__name__)	
	bpy.types.INFO_MT_file_export.append(menu_func)
	#bpy.types.INFO_MT_mesh_add.append(menu_func)
		
def unregister():
	bpy.utils.unregister_module(__name__)
	bpy.types.INFO_MT_file_export.remove(menu_func)
	#bpy.types.INFO_MT_mesh_add.remove(menu_func)

if __name__ == "__main__":
	register()
