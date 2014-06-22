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
	"name": "gemini Exporter",
	"author": "Adam Petrone",
	"version": (0,0,1),
	"blender": (2, 6, 3),
	"api": 39307,
	"location": "Location",
	"description": "Exporter for Project Gemini",
	"warning": "",
	"wiki_url": "",
	"tracker_url": "",
	"category": "Import-Export"}

import os

import json
import bpy
from bpy.props import *
from datetime import datetime
bpy.types.Scene.triangulated_mesh = BoolProperty(
	name="Triangulate Mesh",
	description="Convert Quad to Tri Mesh Boolean...",
	default=False)

from mathutils import Matrix, Vector
import math
import operator

# References: http://en.wikibooks.org/wiki/Blender_3D:_Blending_Into_Python/Cookbook#Triangulate_NMesh
# io_export_unreal_psk_psa.py


def float_clamp( a ):
	if abs(a) < 0.0001:
		return 0.0
	return a

def float_equal( a, b ):
	if abs(a-b) < 0.0000001:
		return True
	return False

def float_cmp( a, b ):
	if abs(a-b) < 0.0000001:
		return 0
	return -1

class ArcEngineConfig(object):
	pass

class HVector(Vector):
	def _description(self):
		return (self.x, self.y, self.z)

	def __hash__(self):
		return hash( self._description() )

class AEVec3(object):
	
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
class AEVertex(object):
	def __init__(self):
		self.position = AEVec3()
		self.normal = AEVec3()
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

class Material(object):
	def __init__(self, index):
		self.index = index
		self.path = ""

	def setMaterial(self, path):
		self.path = path

	def __eq__(self, other):
		print( 'equality for materials' )
		return self.path == other.path

	def __lt__(self, other):
		return self.index < other.index or self.path < other.path

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

	def addVertex(self, vertex):
		if vertex not in self.verts:
			self.verts[ vertex ] = self.vert_index
			self.vert_index += 1
		
		return self.verts[ vertex ]

	def addTriangle( self, a, b, c ):
		#print( "addTriangle: [%i, %i, %i]" % (a,b,c) )
		#indices = [self.highest_index+a,self.highest_index+b,self.highest_index+c]
		indices = [a,b,c]
		#print( indices )
		self.indices.extend( indices )
'''
	def recalculateHighestIndex(self):
		print( "-> highestIndex is: %i" % self.highest_index )
		self.highest_index = max(self.indices)
		self.highest_index += 1
		print( "<- highestIndex is: %i" % self.highest_index )
'''
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
			geometry = self.geometry[ material_index ] = Geometry();
			geometry.id = self.geometry_index
			self.geometry_index += 1
			return geometry

	'''
	def recalculateIndices(self):
		for matid, geometry in self.geometry.items():
			geometry.recalculateHighestIndex()
	'''
	def writeFile(self, file):
		root = {}
		root['name'] = self.name
		root['info'] = {
			'blender_version': ("%i.%i.%i" % (bpy.app.version[0], bpy.app.version[1], bpy.app.version[2]))
		}
		root['transform'] = {'translate' : self.translate }
		root['geometry'] = []
		root['materials'] = []

		geometry_id = 0
		for material_id,geometry in self.geometry.items():

			print( "Writing geometry: %i" % geometry_id )
			positions = []
			normals = []
			uvs = []

			getval = operator.itemgetter(0)
			getkey = operator.itemgetter(1)
			vert_sorted = map(getval, sorted(geometry.verts.items(), key=getkey))

			for v in vert_sorted:
				positions.extend( [float_clamp(v.position.x), float_clamp(v.position.y), float_clamp(v.position.z)] )
				normals.extend( [v.normal.x, v.normal.y, v.normal.z] )
				uvs.extend( [v.u, v.v] )

			geometry_data = {
				'info': { 
					'num_indices' : len(geometry.indices),
					'num_positions': len(positions)
				},
				'material_id': material_id, 
				'indices': geometry.indices, 
				'positions': positions, 
				'normals' : normals, 
				'uvs' : uvs
			}
			root['geometry'].append( geometry_data )
			geometry_id += 1

		material_id = 0
		for material in self.material_list:
			if material.path:
				material_name = os.path.splitext( os.path.basename( material.path ) )[0]
				root['materials'].append( {'name': material_name } )
			material_id += 1
		file.write( json.dumps( root, indent='\t' ) )
BMeshAware = False

def checkBMeshAware():
	global BMeshAware

	print( "Blender r%s" % bpy.app.build_revision )
	BMeshAware = (int(bpy.app.build_revision) > 44136)

def getMeshFaces( mesh ):
	# for BMesh compatability
	# http://blenderartists.org/forum/showthread.php?251840-Bmesh-Where-did-the-UV-coords-go&p=2097099&viewfull=1#post2097099
	if hasattr(mesh, 'polygons'):
		mesh.calc_tessface()
		mesh_faces = mesh.tessfaces
	else:
		mesh_faces = mesh.faces

	return mesh_faces

def triangulate_mesh( object ):

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

	#print( "Checking if object", object.name, "needs conversion from Quads to Triangles..." )
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

		bpy.context.scene.triangulated_mesh = True
		return obj
	else:	   
		#print( "No need to convert mesh to triangles." )
		bpy.context.scene.triangulated_mesh = False
		return object
		

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

class my_export_test(bpy.types.Operator):
	'''Export Skeleton Mesh / Animation Data file(s)'''
	bl_idname = "gemini_export.test" # this is important since its how bpy.ops.export.udk_anim_data is constructed
	bl_label = "Export gemini .model"
	# List of operator properties, the attributes will be assigned
	# to the class instance from the operator settings before calling.

	CoordinateSystems = (
		("1", "Left-Handed", ""),
		("2", "Right-Handed", ""))
	CoordinateSystem = EnumProperty(
		name="System",
		description="Select a coordinate system to export to",
		items=CoordinateSystems,
		default="1")

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

	def execute(self, context):
		global unique_vertices
		global last_vertex
		#print( 'Execute!' )

		self.cfg = ArcEngineConfig()
		self.cfg.ExportRaw = self.ExportRaw
		self.cfg.TransformNormals = self.TransformNormals
		
		if self.cfg.TransformNormals:
			print( "TransformNormals is enabled." )

		start_time = datetime.now()
		
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


			material_list = []

			bpy.context.scene.objects.active = None

			current_mesh = 0
			total_meshes = len(selected_meshes)


			exportedMesh = MeshContainer( self.cfg )


			print( "Total selected meshes: %i" % len(selected_meshes) )
			for obj in selected_meshes:
			#for obj in bpy.context.selected_objects:
				#if obj.type not in {'MESH'}:
				#	continue


				# from Marmalade plugin; convert Z up to Y up.
				xrotation = Matrix.Rotation( -math.pi/2, 4, 'X' )

				# convert object rotation to quaternion (this ignores scale, apparently)
				meshRotation = obj.matrix_world.to_quaternion()

				# convert this to a 3x3 rotation matrix
				meshRotationMatrix = meshRotation.to_matrix().to_3x3()

				
				final_rotation = xrotation.to_3x3() * meshRotationMatrix
				#scale = obj.matrix_world.to_scale()
				#translation = obj.matrix_world.to_translation()
				
				'''
				mesh_faces = getMeshFaces(obj.data)
				transformed_normals = {}
				for face in mesh_faces:
					for i in range(3):
						vert_index = face.vertices[i]
						normal = obj.data.vertices[ vert_index ].normal
						transformed_normals[ vert_index ] = tn = final_rotation * normal

						print( "face.index: %i, vert_index: %i" % (face.index, vert_index))
						print( "Original Normal: %s" % normal )
						print( "Transformed: %s" % tn )
				'''

				mesh_name = obj.name
				obj = triangulate_mesh( obj )
				print( "Processing object: %s" % obj.name )


				# make a copy of this value
				#translation_offset = obj.matrix_world.translation[:]

				# we apply rotation and scale transform to the object
				# this ensures that any modifications are baked to the vertices before we output them to the file
				bpy.context.scene.objects.active = obj
				output = bpy.ops.object.transform_apply( location=True, scale=True )
				#print( "Transform: %s" % output )

				mesh = obj.to_mesh( bpy.context.scene, True, 'PREVIEW' )
				mesh.name = obj.name

				exportedMesh.name = mesh_name
				#exportedMesh.translate = [translation_offset[0], translation_offset[1], translation_offset[2]]
				exportedMesh.translate = [0,0,0]
				
				

				

				mesh.transform( final_rotation.to_4x4() )


				material_id = 0
				for material in mesh.materials:
					#aeMaterial = exportedMesh.findMaterial( material_id )
					exportedMesh.addMaterial( material )
					material_id += 1			

				mesh_faces = getMeshFaces(mesh)
				print( "Total: %i faces." % len(mesh_faces))
				for face in mesh_faces:
					tris = []
					vlist = []
					material_id = 0
					if len(mesh.materials) > 0:
						material = exportedMesh.findMaterial( mesh.materials[ face.material_index ] )
						if material:
							material_id = material.index

					#print( "Finding geometry with material_id=%i" % material_id )
					geometry = exportedMesh.findGeometry( material_id )
					for i in range(3):
						
						vertex = {}
						vert_index = face.vertices[i]

						has_uvs = False
						if len(mesh.uv_textures) > 0:
							has_uvs = True
							uv_layer = mesh.tessface_uv_textures.active
							face_uv = uv_layer.data[ face.index ]

						if has_uvs:
							if len(face_uv.uv) == 3:
								uvs = (float_clamp(face_uv.uv[i][0]), float_clamp(face_uv.uv[i][1]))
							else:
								print( "ERROR: invalid uv face found: %i" % face.index )
								uvs = (0.0, 0.0)
						else:
							uvs = (0.0, 0.0)
							
						#vertex['position'] = mesh.vertices[ vert_index ].co
						#vertex['normal'] = mesh.vertices[ vert_index].normal
						#vertex['material_id'] = face.material_index
						#print( vertex )
						
						position = mesh.vertices[ vert_index ].co
						vertex_normal = final_rotation * mesh.vertices[vert_index].normal

						# transform the vertex normal
						#if self.cfg.TransformNormals:
						#	vertex_normal = -(final_rotation * vertex_normal)

						v = AEVertex()
						v.position.set(position[0], position[1], position[2])
						v.normal.set(vertex_normal[0], vertex_normal[1], vertex_normal[2])
						v.u = uvs[0]
						v.v = uvs[1]

						#print( "F%i P%s N%s -> N%s" % (vert_index, mesh.vertices[vert_index].co, mesh.vertices[vert_index].normal, vertex_normal) )
						#print( v.description() )
						vid = geometry.addVertex( v )
						
						tris.append( vid )
						#vlist.append( Vector(mesh.vertices[ vert_index ].co) )

						#print( "Geometry: %i, VertexID %i -> vid: %i" % (geometry.id, vert_index, vid ) )
						#print( "Vertex ID: %i, hash:%i" % (vid, hash(v)) )
						

					geometry.addTriangle( tris[0], tris[1], tris[2] )
					#print( "Face Normal: %s" % face.normal )
					'''
					edge1 = (vlist[1] - vlist[0])
					edge2 = (vlist[2] - vlist[1])
					n = edge1.cross(edge2)

					d = n.dot(face.normal)
					#print( "dot: %f" % d )
					#print( "Triangle: %i %i %i" % (tris[0], tris[1], tris[2]) )
					
					if d < 0:
						geometry.addTriangle( tris[2], tris[1], tris[0] )
					elif d > 0:
						geometry.addTriangle( tris[0], tris[1], tris[2] )
					else:
						print( "Face normal is coplanar!!" )
					'''
				if bpy.context.scene.triangulated_mesh:	
					bpy.ops.object.mode_set( mode='OBJECT' )
					#print( "Removing temporary mesh: ", obj.name )
					bpy.context.scene.objects.unlink( obj )

				current_mesh += 1
				print( 'Export Progress -> %g%%' % ((current_mesh/total_meshes) * 100.0) )

				#exportedMesh.recalculateIndices()

			exportedMesh.writeFile( file )
			file.close()

			

			delta_time = datetime.now() - start_time

			result_message = 'Exported %i meshes in %s seconds.' % (total_meshes, delta_time.seconds)
			print( result_message )
			self.report({'INFO'}, result_message)
			
		return {'FINISHED'}
		
	def invoke(self, context, event):
		#print( 'Invoke' )
		wm = context.window_manager
		wm.fileselect_add(self)
		return {'RUNNING_MODAL'}

def menu_func(self, context):
	self.layout.operator(my_export_test.bl_idname, text="gemini .model")
	self.layout.operator(export_animation.bl_idname, text="gemini .animation")

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
