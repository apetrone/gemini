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

# bind pose
# http://blenderartists.org/forum/showthread.php?323968-Exporting-armature-amp-actions-how-do-you-get-the-bind-pose-and-relative-transform&highlight=exporter

# blend indices and weights
# http://blenderartists.org/forum/showthread.php?337882-Vertex-blend-indices-and-blend-weights&highlight=exporter

# exporting keyframe animations
# https://38leinad.wordpress.com/2012/06/04/blender-exporting-keyframe-based-animations/
#
# Blender has no mapping of Actions -> Compatible Objects.
# The best way would be to have the user choose which actions to export; otherwise
# we have to export all of them.
# At this time, I'm indifferent about exporting actions and various objects.
# For now, I'll just choose to export all actions.


# This should export skeletons with the origin (0, 0, 0)
# corresponding to the root of the model.

# TODO:
# - export armatures with root at 0, 0, 0
# - calculate min/max bounds


#
# imports
#

# handle module reloads [mainly used for development]
if "bpy" in locals():
	import importlib
	if "io_export_gemini" in locals():
		importlib.reload(io_export_gemini)

import bpy
from bpy.props import (
	StringProperty,
	BoolProperty,
	FloatProperty,
	EnumProperty
)
from bpy_extras.io_utils import (
	axis_conversion
)

from time import time
from mathutils import (
	Matrix,
	Vector,
)

from math import (
	degrees
)

import math
import operator
import json
import os
import platform
import sys

#
# constants
#
COORDINATE_SYSTEM_ZUP = "ZUP"
COORDINATE_SYSTEM_YUP = "YUP"

#
# utility functions
#

def matrix_to_list(matrix):
	output = []

	for row in range(0, len(matrix.row)):
		for col in range(0, len(matrix.col)):
			output.append(matrix[col][row])
	return output

def create_triangulated_mesh(object):
	# de-select all objects
	for i in bpy.context.scene.objects:
		i.select = False

	# de-seect object
	object.select = False

	# set active object to this one
	bpy.context.scene.objects.active = object

	# for BMesh compatability
	# http://blenderartists.org/forum/showthread.php?251840-Bmesh-Where-did-the-UV-coords-go&p=2097099&viewfull=1#post2097099
	if hasattr(object.data, 'polygons'):
		object.data.calc_tessface()
		mesh_faces = object.data.tessfaces
	else:
		mesh_faces = object.data.faces

	quads = [f for f in mesh_faces if len(f.vertices) == 4]

	if quads:
		# make a copy of the object and triangulate this.
		data = object.data.copy()
		obj = object.copy()
		obj.data = data
		obj.name = object.name + '_triangulated'

		# link copied object into the scene
		bpy.context.scene.objects.link( obj )

		bpy.ops.object.select_all(action='DESELECT')

		# select the copied object and make it active
		bpy.ops.object.select_pattern(pattern=obj.name)
		bpy.context.scene.objects.active = obj

		# enter EDIT mode on this object
		bpy.ops.object.mode_set(mode='EDIT')

		# select all components
		bpy.ops.mesh.select_all(action='SELECT')

		# convert quads to tris using Blender's internal method
		bpy.ops.mesh.quads_convert_to_tris()

		# update scene
		bpy.context.scene.update()

		# go back to object mode
		bpy.ops.object.mode_set(mode='OBJECT')

		return obj, True
	else:
		# No need to convert mesh to triangles; already triangulated
		return object, False


#
# supporting classes
#


class Material(object):
	def __init__(self, index, path):
		self.index = index
		self.path = path

	def __eq__(self, other):
		return self.path == other.path

	def __lt__(self, other):
		return self.index < other.index or self.path < other.path

# class HashedVector(Vector):
# 	def _description(self):
# 		return (self.x, self.y, self.z)

# 	def __hash__(self):
# 		return hash( self._description() )

class Vector3(object):

	def __init__(self, x, y, z):
		self.x = x
		self.y = y
		self.z = z

	def description(self):
		return (self.x, self.y, self.z)

	def __lt__(self, other):
		return self.__hash__() < other.__hash__()

	def __hash__(self):
		return hash( self.description() )

	def __eq__(self, other):
		if not hasattr(other, 'description'):
			return False
		return self.description() == other.description()

	def __repr__(self):
		return str('Vector3(x=%2.2f, y=%2.2f, z=%2.2f)' % (self.x, self.y, self.z))

# A custom vertex class allows us to place these into a dictonary
# and have them compare properly
class Vertex(object):
	def __init__(self,
		position = Vector3(0, 0, 0),
		normal = Vector3(0, 0, 0),
		u = 0.0,
		v = 0.0,
		color = (1, 1, 1, 1),
		material_id = 0,
		weights = []):
		self.position = position
		self.normal = normal
		self.u = u
		self.v = v
		self.color = color
		self.material_id = material_id
		self.weights = weights

	def show(self):
		print("xyz: %2.2f, %2.2f, %2.2f" % (self.position.x, self.position.y, self.position.z))
		print("norm: %2.2f, %2.2f, %2.2f" % (self.normal.x, self.normal.y, self.normal.z))
		print("uv: %2.2f, %2.2f" % (self.u, self.v))
		print("weights: %s" % self.weights)

	def description(self):
		return (self.position.x,
			self.position.y,
			self.position.z,
			self.normal.x,
			self.normal.y,
			self.normal.z,
			self.u,
			self.v,
			self.material_id)

	def __lt__(self, other):
		return self.__hash__() < other.__hash__()

	def __hash__(self):
		return hash( self.description() )

	def __eq__(self, other):
		if not hasattr(other, 'description'):
			return False
		return self.description() == other.description()


class Weight(object):
	def __init__(self, vertex_index, weight):
		self.vertex_index = vertex_index
		self.weight = weight

class VertexCache(object):
	def __init__(self):
		self.vertices = {}
		self.ordered_vertices = []
		self.indices = []

		# map old vertex index to new vertex index
		self.old_vertex_map = {}

	def find_vertex_index(self, vertex):
		if vertex not in self.vertices:
			self.vertices[vertex] = len(self.ordered_vertices)
			self.ordered_vertices.append(vertex)
		return self.vertices[vertex]

	def populate_with_geometry(self, vertices, normals, uvs, colors, loops,
		weights=[]):
		#
		# NOTE: Some Elements are accessed by LOOP INDEX -- NOT BY VERTEX INDEX!
		# UVs, Vertex Colors
		#
		for loop in loops:
			vertex_index = loop.vertex_index

			position = Vector3(vertices[vertex_index][0],
				vertices[vertex_index][1],
				vertices[vertex_index][2])

			normal = Vector3(normals[loop.index][0],
				normals[loop.index][1],
				normals[loop.index][2])

			# for now, this only handles one uv set
			uv = uvs[0][loop.index]

			# for now, this only handles one color set
			color = colors[0][loop.index]

			# assemble a vertex using the loop index
			vertex = Vertex(position = position,
				normal = normal,
				u = uv[0],
				v = uv[1],
				color = color,
				weights = weights[vertex_index])

			index = self.find_vertex_index(vertex)
			#vertex.show()

			self.indices.append(index)

class Node(object):
	NODE = "node"
	ROOT = "root"
	MESH = "mesh"
	ANIM = "anim"

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

	def prepare_materials_for_export(self, model):
		material_id = 0
		for material in model.material_list:
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
		self.mins = [9999.0, 9999.0, 9999.0]
		self.maxs = [-9999.0, -9999.0, -9999.0]
		self.mass_center_offset = [0.0, 0.0, 0.0]
		self.vertices = []
		self.normals = []
		self.indices = []
		self.material_id = -1
		self.uv_sets = []
		self.bind_data = []
		self.blend_weights = []
		self.vertex_colors = []
		self.skeleton = []

	def populate_with_vertex_cache(self, cache):
		uvset = []
		colorset = []

		# traverse the cache over the ordered vertices
		# in order to build the list for the mesh node
		vertex_index = 0
		for vertex in cache.ordered_vertices:
			self.vertices.append(
				[vertex.position.x, vertex.position.y, vertex.position.z])

			self.normals.append(
				[vertex.normal.x, vertex.normal.y, vertex.normal.z])

			uvset.append([vertex.u, vertex.v])
			colorset.append(vertex.color)

			vertex_index += 1

			if vertex.weights:
				self.blend_weights.append(vertex.weights)

		self.uv_sets.append(uvset)

		# the format currently only supports a single vertex color set
		self.vertex_colors = colorset
		self.indices = cache.indices

		print("# vertices: %i" % len(self.vertices))
		print("# indices: %i" % len(self.indices))

	@staticmethod
	def process_armature(model, node, cache, obj):
		""" Returns a BoneCache instance """
		bone_data = BoneCache()

		# apply modifiers to the mesh node
		for modifier in obj.modifiers:
			print("Found modifier '%s'" % modifier.name)
			if modifier.type == "ARMATURE":

				if not modifier.object:
					# This can happen when the Armature was renamed or
					# otherwise disassociated with the modifier
					model.info("%s has an Armature modifier with an "
						"invalid object!" %	obj.name
					)

				armature = modifier.object

				# build the skeleton for this mesh

				if len(node.skeleton) > 0:
					model.info("Found multiple skeletons in the file. This "
						"exporter cannot handle that yet."
					)
				assert(node.skeleton == [])

				pose_bones = armature.pose.bones
				pose_bones_by_name = {}
				for pbone in pose_bones:
					pose_bones_by_name[pbone.name] = pbone

				# We also need to populate the bind pose offset list
				bind_data = []

				bone_data = collect_bone_data(model,
					armature,
					pose_bones_by_name)

				for bone in bone_data.ordered_items:
					node.skeleton.append({
						"name": bone.name,
						"parent": bone.parent_index
					})

					bind_data.append({
						"name": bone.name,
						"parent": bone.parent_index,
						"inverse_bind_pose": matrix_to_list(bone.inverse_bind_pose),
						"bind_pose": matrix_to_list(bone.bind_pose)
					})

				node.bind_data = bind_data

		return bone_data

	@staticmethod
	def from_object(model, obj, root):
		node = Mesh(name=obj.name)

		triangulated_object, created_temp_mesh = create_triangulated_mesh(obj)

		# apply rotation and scale transform to the object
		# this ensures that any modifications are baked to the vertices
		# before we output them to the file
		triangulated_object.select = True

		# This is used to transform the vertices and normals of the mesh.
		world_matrix = model.global_matrix * triangulated_object.matrix_world.copy()

		mesh = triangulated_object.to_mesh(bpy.context.scene, True, 'PREVIEW')

		# collect all blender materials used by this mesh
		for bmaterial in mesh.materials:
			mat = model.add_material(bmaterial)
			node.material_id = mat.index

		cache = VertexCache()

		bone_data = Mesh.process_armature(model, node, cache, obj)
		model.bone_data = bone_data

		weights = [[None]] * len(mesh.vertices)

		vertices = []

		mins = Vector((9999, 9999, 9999))
		maxs = Vector((-9999, -9999, -9999))

		# build a mapping from group index to bone index
		total_bones = len(bone_data.ordered_items) if bone_data else 0
		group_index_to_bone = [None] * total_bones
		for group in obj.vertex_groups:
			# convert group index to boneinfo.index
			boneinfo = bone_data.find_by_name(group.name)
			if not boneinfo:
				# It's possible to have vertex groups for bones which
				# may have been removed. Ignore these.
				continue

			group_index_to_bone[group.index] = boneinfo

		for index, mv in enumerate(mesh.vertices):
			position = world_matrix * mv.co
			vertices.extend([(
				position[0],
				position[1],
				position[2])])

			if position[0] < mins.x:
				mins.x = position[0]
			elif position[0] > maxs.x:
				maxs.x = position[0]

			if position[1] < mins.y:
				mins.y = position[1]
			elif position[1] > maxs.y:
				maxs.y = position[1]

			if position[2] < mins.z:
				mins.z = position[2]
			elif position[2] > maxs.z:
				maxs.z = position[2]

			# don't add out of range indices
			weights[index] = []
			for group_element in mv.groups:
				if group_element.group < total_bones:
					bone = group_index_to_bone[group_element.group]
					if bone and group_element.weight > 0.0:
						if len(weights[index]) < 4:
							weights[index].append({
								"bone": bone.name,
								"value": group_element.weight
								})
						#else:
						#	print("warning: dropping weight for bone '%s', %2.2f" % (bone.name, group_element.weight))


		node.mins = [mins.x, mins.y, mins.z]
		node.maxs = [maxs.x, maxs.y, maxs.z]

		mesh.calc_normals_split()

		normals = []
		for loop in mesh.loops:
			# transform normals
			normal = (world_matrix.to_3x3() * loop.normal).normalized()
			normals.append((normal[0], normal[1], normal[2]))
		mesh.free_normals_split()

		uvs = []
		if not mesh.uv_layers:
			uv_set = [(0.0, 0.0)] * len(mesh.loops)
			uvs.append(uv_set)
		else:
			for uvlayer in mesh.uv_layers:
				uv_set = []
				print("extracting uv layer: %s" % uvlayer.name)
				for uvloop in uvlayer.data:
					uv_set.append([uvloop.uv[0], 1.0-uvloop.uv[1]])
				uvs.append(uv_set)

		colors = []
		if not mesh.vertex_colors:
			# add a default color set
			color_set = ([(1.0, 1.0, 1.0, 1.0)] * len(mesh.loops))
			colors.append(color_set)
		else:
			for color_layer in mesh.vertex_colors:
				color_set = []
				print("extracting color layer: %s" % color_layer.name)
				for data in color_layer.data:
					color_set.append((data.color[0], data.color[1], data.color[2], 1.0))
				colors.append(color_set)

		if weights:
			print("total weights: %i, total vertices: %i" % (len(weights), len(vertices)))
			assert(len(weights) == len(vertices))

		# convert and copy geometry over to node
		cache.populate_with_geometry(vertices, normals, uvs, colors, mesh.loops, weights=weights)
		node.populate_with_vertex_cache(cache)

		# de-select the previously selected object
		triangulated_object.select = False

		if created_temp_mesh:
			# remove the triangulated object we created
			bpy.ops.object.mode_set(mode='OBJECT')
			bpy.context.scene.objects.unlink(triangulated_object)

		return node


class AnimatedSequence(Node):
	def __init__(self, **kwargs):
		super(AnimatedSequence, self).__init__(**kwargs)

		self.type = Node.ANIM
		self.duration_seconds = 0
		self.frames_per_second = -1
		self.name = kwargs.get("name", None)
#
# main classes
#

class BoneData(object):
	def __init__(self, bone, pose_bone, index, parent_index = -1):
		self.name = bone.name
		self.index = index
		self.parent_index = parent_index
		self.pose_bone = pose_bone
		self.bone = bone

		# inverse bind pose (object space to joint space)
		self.inverse_bind_pose = None

		# bone's local offset from parent in object space
		self.bind_pose = None

class BoneCache(object):
	def __init__(self):
		self.bone_data_by_name = {}
		self.bone_data_by_index = {}
		self.ordered_items = []

	def set(self, bone_name, bone_index, bone_data):
		self.bone_data_by_name[bone_name] = bone_data
		self.bone_data_by_index[bone_index] = bone_data
		self.ordered_items.append(bone_data)

	def find_by_name(self, bone_name):
		if bone_name in self.bone_data_by_name:
			return self.bone_data_by_name[bone_name]
		return None

def collect_bone_data(model, armature, pose_bones_by_name):
	cache = BoneCache()
	bone_index = 0
	bones = armature.data.bones

	parent_matrix = Matrix()
	armature_offset = armature.matrix_world.to_translation()
	# Only assume this will be offset from Blender's Z-axis.
	# This is to allow character's pelvis bones to be aligned at the
	# origin, but offset in Z due to character height.
	armature_offset.x = 0
	armature_offset.y = 0
	transformed_parent_offset = Matrix.Translation(model.global_matrix * armature_offset)

	for index, bone in enumerate(bones):
		parent_index = -1

		if not bone.use_deform:
			# skip bones non-deforming bones.
			continue

		if bone.parent:
			parent_index = cache.find_by_name(bone.parent.name).index

		bone_data = BoneData(bone,
			pose_bones_by_name[bone.name],
			bone_index,
			parent_index)

		# If you hit this, you aren't using the associated pose bone
		assert(bone_data.name == bone_data.pose_bone.name)

		bind_matrix = Matrix()
		inverse_bind_pose = Matrix()

		parent = bone_data.pose_bone.parent.bone if bone_data.pose_bone.parent else None
		if parent:
			parent_matrix = (parent.matrix_local)
		else:
			# No parent, so this bone is the root bone.
			# It should be treated as the identity, for now.
			parent_matrix = bone_data.bone.matrix_local

		# inverse_bind_pose needs to include the transform from the armature
		# since it needs the correct position of its world position in order to transform to joint space.
		inverse_bind_pose = (model.global_matrix * armature.matrix_world * bone_data.bone.matrix_local).inverted()

		# the bind pose needs to be relative to the parent
		# At the moment, these poses don't track the armature offset.
		# I'm not quite sure the best way to handle this -- so for now:
		# The artist must create the rig with the root bone at the origin (0, 0, 0).
		# That way, we can treat all model roots as the origin.
		bone_data.bind_pose = (model.global_matrix * bone_data.bone.matrix_local) * ((model.global_matrix * parent_matrix).inverted())

		if not parent:
			bone_data.bind_pose = transformed_parent_offset * bone_data.bind_pose

		# converts the object space vertices to joint space
		bone_data.inverse_bind_pose = inverse_bind_pose

		cache.set(bone.name, bone_index, bone_data)

		bone_index += 1
	return cache


class GeminiModel(object):
	"""This serves as the container instance for a model which is to be
	written to disk. This holds intermediate data used to collect bones,
	animations, and other internal data required for export."""

	def __init__(self, plugin_instance, **kwargs):
		self.instance = plugin_instance
		assert(self.instance)

		# exporter settings
		self.coordinate_system = kwargs.get("coordinate_system", None)

		# used for coordinate conversion
		# Assume -Z forward with Y-Up.
		self.global_matrix = axis_conversion(to_forward='-Z', to_up='Y').to_4x4()

		self.filepath = kwargs.get("filepath", None)

		# animation export range
		self.frame_start = kwargs.get("frame_start", 0)
		self.frame_end = kwargs.get("frame_end", 0)

		# Export selected meshes only
		self.selected_only = kwargs.get("selected_only", False)

		self.armature = None

		# store the pose position of the armature so we can restore it after
		self.armature_pose_position = None

		self.file_handle = None

		print("export range: %i -> %i" % (self.frame_start, self.frame_end))

		# internal state for handling materials
		self.material_index = 0
		self.materials = {}
		self.material_list = []

		# Bone cache for this model if attached to an Armature
		self.bone_data = None

		self.selected_meshes = []
		self.collision_meshes = []

	#
	# Materials
	#
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

	#
	# File test -- make sure we can open it before attempting to export
	#
	def try_open_file(self):
		""" A test to ensure we can open the file requested by the user"""
		try:
			self.file_handle = open(self.filepath, "w")
			return True
		except IOError:
			return False

	# Plugin logging facilities
	def info(self, message):
		self.instance.report({'INFO'}, message)

	def populate_animations(self, actions):
		""" Populate Animations given the armatures and actions from the scene"""

		# bail out early if we're missing data
		if not (self.armature and self.bone_data):
			return []

		bpy.ops.object.select_all(action='DESELECT')

		animations = []

		scene_fps = bpy.context.scene.render.fps

		sequence_time_begin = self.frame_start / float(scene_fps)

		# place the armature in POSE position
		self.armature.data.pose_position = 'POSE'

		# If this assert is hit, armature has no keyframes.
		if self.armature.animation_data is not None:
			old_action = self.armature.animation_data.action

		for action in actions:
			sequence = AnimatedSequence()
			sequence.name = action.name
			sequence.frames_per_second = scene_fps
			sequence.duration_seconds = sequence_duration = (self.frame_end - self.frame_start) / scene_fps

			if self.armature.animation_data is not None:
				print("Exporting action '%s'" % action.name)
				bpy.context.scene.objects.active = self.armature
				bpy.ops.object.select_pattern(pattern=self.armature.name)
				bpy.ops.object.mode_set(mode='POSE')
				bpy.ops.pose.select_all(action='SELECT')
				self.armature.animation_data.action = action
				bpy.ops.object.mode_set(mode='OBJECT')

			for bone_data in self.bone_data.ordered_items:
				obj = {
					"name": bone_data.name,
					"rotation": {},
					"translation": {},
					"scale": {}
				}

				time_values = []
				#
				# SCALE
				#
				# scale = []
				# obj["scale"] = {
				# 	"time": time_values,
				# 	"value": scale
				# }

				#
				# ROTATION
				#
				rotation = []
				#euler = [] # for debugging
				obj["rotation"] = {
					"time": time_values,
					"value": rotation
				#	"euler": euler,
				}

				#
				# TRANSLATION
				#
				translation = []
				obj["translation"] = {
					"time": time_values,
					"value": translation
				}

				# set the current frame
				prev_rot = None
				for frame in range(self.frame_start, self.frame_end):
					bpy.context.scene.frame_set(frame)

					# populate the time
					current_time_seconds = (frame / float(scene_fps)) - sequence_time_begin
					time_values.append(current_time_seconds)

					global_tx = Matrix()

					inverse_parent_matrix = Matrix()

					if bone_data.pose_bone.parent:
						inverse_parent_matrix = (bone_data.pose_bone.parent.matrix).inverted()
					else:
						global_tx = self.global_matrix

					inverted_bind_pose = bone_data.bind_pose.copy().inverted()

					# Compute the delta from the bind pose to this frame.
					# We need the inverted bind pose to remove any transforms from
					# the initial bind pose.
					# Inverse parent matrix ensures this only records local transforms.
					matrix_delta = global_tx * (inverted_bind_pose * inverse_parent_matrix * bone_data.pose_bone.matrix)

					tx, rx, sx = matrix_delta.decompose()
					#euler.append([degrees(value) for value in rx.to_euler()])
					if prev_rot:
						# This helps ensure we dump the quaternion
						# that chooses the shortest-angle between
						# it and the last one.
						if prev_rot.dot(rx) < 0.0:
							rx = rx.inverted()

					#scale.append([sx[0], sx[1], sx[2]])
					rotation.append([rx.x, rx.y, rx.z, rx.w])
					translation.append([tx[0], tx[1], tx[2]])

					prev_rot = rx

				sequence.children.append(obj)

			animations.append(sequence)

			if self.armature.animation_data is not None:
				self.armature.animation_data.action = old_action
		return animations


	def add_object(self, obj):
		type_filter = ['MESH', 'ARMATURE']
		num_armatures = 0

		# For now, we only care about meshes and armatures
		if obj.type in type_filter:
			if obj.type == 'ARMATURE':
				# setup armature
				self.armature = obj
				self.armature_pose_position = obj.data.pose_position
				self.armature.data.pose_position = 'REST'

				num_armatures += 1
				if num_armatures > 1:
					print("*** Found more than one armature! Danger Will Robinson!")

			elif obj.type == 'MESH':
				if self.selected_only and obj.select or not self.selected_only:
					self.selected_meshes.append(obj)

	def generate_collision_geometry(self, collision_meshes):
		vertices = []
		normals = []
		indices = []

		if not collision_meshes:
			print('No collision meshes. Skipping.')
			return

		if len(collision_meshes) > 1:
			print("Found more than one collision mesh. Ignoring all but the first, {}".format(collision_meshes[0].name))

		mesh = Mesh.from_object(self, collision_meshes[0], None)

		return {'vertices': mesh.vertices, 'normals': mesh.normals, 'indices': mesh.indices}


	#
	# Export model + animations
	#
	def export(self):
		# make sure we're in object mode
		if bpy.ops.object.mode_set.poll():
			bpy.ops.object.mode_set(mode="OBJECT")

		original_scrubber_position = bpy.context.scene.frame_current

		# in order to retrieve pose data; we need to explicitly
		# set the frame before we retrieve data.
		bpy.context.scene.frame_set(1)

		# scan through scene objects
		for index, obj in enumerate(bpy.context.scene.objects):
			#print("index: %i, object: %s, type: %s" % (index, obj.name, obj.type))
			if '_collision' in obj.name:
				print('Found collision mesh {}'.format(obj.name))
				self.collision_meshes.append(obj)
			else:
				self.add_object(obj)

		# clear selection
		bpy.ops.object.select_all(action='DESELECT')
		#original_selection = bpy.context.scene.objects.selected_objects
		bpy.context.scene.objects.active = None

		current_mesh = 0
		total_meshes = len(self.selected_meshes)

		print("Total selected meshes: %i" % len(self.selected_meshes))

		animations = []

		root_node = RootNode(materials=[])
		root_node.export_info = {
			'blender_version': ("%i.%i.%i" % (bpy.app.version[0], bpy.app.version[1], bpy.app.version[2])),
			'host_platform': platform.platform().lower(),
			'source_file': bpy.data.filepath
		}

		# iterate over selected objects
		for obj in self.selected_meshes:
			# NOTE: This assaumes that each object has a single distinct texture.
			# This will have to be updated at some point!

			# TODO: Add an explicit EDGE SPLIT modifier?
			meshnode = Mesh.from_object(self, obj, root_node)
			root_node.children.append(meshnode)

			current_mesh += 1
			self.info('Export Progress -> %g%%' % ((current_mesh/total_meshes) * 100.0))

		root_node.prepare_materials_for_export(self)

		if self.armature:
			# write animations to the same directory where the .model goes
			target_directory = os.path.dirname(self.filepath)

			# populate animations using actions and the armatures
			animations = self.populate_animations(bpy.data.actions)

			for animation in animations:
				target_animation_path = os.path.join(target_directory, ("%s.animation" % animation.name))
				print("target_animation_path: %s" % target_animation_path)
				handle = open(target_animation_path, "w")

				handle.write(animation.to_json())
				handle.close()

		# Populate the animations so we know which to load.
		root_node.animations = [anim.name for anim in animations]

		root_node.collision_geometry = self.generate_collision_geometry(self.collision_meshes)

		# Write out the mesh file
		self.file_handle.write(root_node.to_json())
		self.file_handle.close()

		# restore the original frame
		bpy.context.scene.frame_set(original_scrubber_position)

		if self.armature:
			# restore armature's pose position
			self.armature.data.pose_position = self.armature_pose_position

class export_gemini(bpy.types.Operator):
	'''Export Skeleton Mesh / Animation Data file(s)'''
	bl_idname = "gemini_export.test" # this is important since its how bpy.ops.export.udk_anim_data is constructed
	bl_label = "Export gemini (.model)"
	# List of operator properties, the attributes will be assigned
	# to the class instance from the operator settings before calling.

	supported_coordinate_systems = (
		(COORDINATE_SYSTEM_YUP, "Y-Up", ""),
		(COORDINATE_SYSTEM_ZUP, "Z-Up", "")
	)

	coordinate_system = EnumProperty(
		name="Coordinate System",
		description="Select a coordinate system to export to",
		items=supported_coordinate_systems,
		default=COORDINATE_SYSTEM_YUP)

	filepath = StringProperty(
			name="File Path",
			description="Filepath used for exporting the file",
			maxlen= 1024,
			subtype='FILE_PATH')

	selected_only = BoolProperty(
		name="Selected Geometry Only",
		description="Export only the selected geometry",
		default=True)

	# triangulate_mesh = BoolProperty(
	# 	name="Triangulate Mesh",
	# 	description="Convert Quad to Tri Mesh Boolean...",
	# 	default=False)

	def execute(self, context):

		# TODO: assert if no bones have keyframes! This can be very confusing
		# if the armature itself has keyframes, but the bones do not.

		model = GeminiModel(
			self,
			coordinate_system = self.coordinate_system,
			filepath = self.filepath,
			frame_start = bpy.context.scene.frame_start,
			frame_end = bpy.context.scene.frame_end + 1,
			selected_only = self.selected_only
		)

		try:
			# Sanity check that we can open the destination file.
			model.try_open_file()
		except:
			self.report("ERROR", str(sys.exc_info()[1]))
			return {"FINISHED"}

		start_time = time()

		model.export()

		delta_time = time() - start_time
		result_message = 'Write file "%s" in %2.2f seconds' % (self.filepath, delta_time)
		self.report({'INFO'}, result_message)
		return {'FINISHED'}

	def invoke(self, context, event):
		wm = context.window_manager
		wm.fileselect_add(self)
		return {'RUNNING_MODAL'}

#
# startup / blender specific interface functions
#
def menu_func(self, context):
	self.layout.operator(export_gemini.bl_idname, text="gemini (.model)")
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
