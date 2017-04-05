# -------------------------------------------------------------
# Copyright (C) 2017- Adam Petrone

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

# http://blenderscripting.blogspot.com/

bl_info = {
	"name": "imocap client",
	"author": "Adam Petrone",
	"version": (0,0,1),
	"blender": (2, 7, 6),
	"api": 39307,
	"location": "Location",
	"description": "Client for imocap",
	"warning": "",
	"wiki_url": "",
	"tracker_url": "",
	"category": "Import-Export"
}

#
# imports
#

# handle module reloads [mainly used for development]
if "bpy" in locals():
	import importlib
	if "io_import_imocap" in locals():
		importlib.reload(io_import_imocap)

import bpy
from bpy.props import (
	StringProperty,
	BoolProperty,
	IntProperty,
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

import select
import socket
import struct

from datetime import datetime
from mathutils import Quaternion
from threading import Thread

#
# constants
#
IMOCAP_DATA_SIZE = 20
QUANTIZE_VALUE = (1.0 / 16384.0)

# TODO: Need to flip Z and Y into Blender's coordinate frame.

#
# utility functions
#

def get_milliseconds():
	# http://stackoverflow.com/questions/5998245/get-current-time-in-milliseconds-in-python/21858377#21858377
	return int((datetime.utcnow() - datetime(1970, 1, 1)).total_seconds() * 1000.0)

class imocap_packet(object):
	def __init__(self, data):
		self.data = data
		self.header = struct.unpack_from('B', data, 0)[0]
		self.footer = struct.unpack_from('B', data, 61)[0]

	def is_valid(self):
		return self.header == 0xba and self.footer == 0xff

	def quaternion(self, index):
		offset = (1 + index * IMOCAP_DATA_SIZE)
		values = struct.unpack_from('<hhhh', self.data, offset)
		return [v * QUANTIZE_VALUE for v in values]

class imocap_client(object):
	# These constants are taken directly from the imocap
	# headers. They must match!
	IMOCAP_TOTAL_SENSORS = 3
	STATE_WAITING = 1
	STATE_HANDSHAKE = 2
	STATE_STREAMING = 3
	CONNECTION_REQUEST_VALUE = 1983
	HANDSHAKE_VALUE = 1985
	HANDSHAKE_RESPONSE = 65535
	CLIENT_PING_VALUE = 2000
	CLIENT_PING_DELAY_MSEC = 1000
	CLIENT_TIMEOUT_MSEC = 3000
	DISCONNECT_VALUE = 2005
	PACKET_SIZE = 62

	def __init__(self):
		self.is_connected = False
		self.thread = None
		self.listen_port = 27015
		self.last_client_ping_msec = 0
		self.last_client_contact_msec = 0
		self.device_rotations = [Quaternion((1.0, 0.0, 0.0, 0.0))] * self.IMOCAP_TOTAL_SENSORS
		self.zeroed_rotations = [Quaternion((1.0, 0.0, 0.0, 0.0))] * self.IMOCAP_TOTAL_SENSORS

	def connect(self):
		# The client responds to UDP broadcasts transmitted
		# by the device. This just needs to open the correct
		# port and wait.
		self.thread = Thread(target=self.wait_for_data, args=(1,0))
		self.is_connected = True

		self.client_address = None
		self.data_socket = socket.socket(socket.AF_INET,
											socket.SOCK_DGRAM)
		self.data_socket.bind(("0.0.0.0", self.listen_port))

		self.thread.start()

	def disconnect(self):
		self.is_connected = False
		self.thread = None
		self.data_socket = None
		self.client_address = None

	def get_local_rotation(self, index):
		return self.zeroed_rotations[index].inverted() * self.device_rotations[index]

	def on_received_new_data(self):
		if 'Cube' in bpy.context.scene.objects:
			obj = bpy.context.scene.objects['Cube']
			obj.rotation_mode = 'QUATERNION'
			obj.rotation_quaternion = self.get_local_rotation(0)
			obj.rotation_mode = 'XYZ'
		else:
			print('No Cube found in scene')

	def freeze_rotations(self):
		for index in range(0, self.IMOCAP_TOTAL_SENSORS):
			self.zeroed_rotations[index] = self.device_rotations[index]

	def wait_for_data(self, *args):
		current_state = self.STATE_WAITING

		while self.is_connected:
			current_milliseconds = get_milliseconds()

			if current_state == self.STATE_STREAMING:
				# If the client has timed out; reset the state.
				delta = current_milliseconds - self.last_client_contact_msec
				if delta > self.CLIENT_TIMEOUT_MSEC:
					print('Client has timed out Exceeded {}ms!'.format(
						self.CLIENT_TIMEOUT_MSEC))
					current_state = self.STATE_WAITING
					continue

				# In order to maintain communication with the streaming
				# device, this must dispatch heartbeats to the connected
				# device.
				delta = current_milliseconds - self.last_client_ping_msec
				if delta > self.CLIENT_PING_DELAY_MSEC:
					self.last_client_ping_msec = get_milliseconds()
					packet = struct.pack('<i', self.CLIENT_PING_VALUE)
					self.data_socket.sendto(packet, self.client_address)

			read_fds = [self.data_socket]
			write_fds = [self.data_socket]

			read_socks, _, _ = select.select(read_fds, [], [], 0.001)
			for sock in read_socks:
				data, address = sock.recvfrom(self.PACKET_SIZE)
				bytes_read = len(data)
				self.last_client_contact_msec = get_milliseconds()
				if current_state == self.STATE_STREAMING:
					packet = imocap_packet(data)
					if not packet.is_valid():
						print('Received an invalid packet!')
						continue
					else:
						# get just the quaternion data out for now.
						for index in range(0, self.IMOCAP_TOTAL_SENSORS):
							values = packet.quaternion(index)
							rotation = Quaternion((values[0],
												   values[1],
												   values[2],
												   values[3]))
							self.device_rotations[index] = rotation

						self.on_received_new_data()
				elif current_state == self.STATE_WAITING:
					request_value = struct.unpack('<i', data)[0]
					if request_value == self.CONNECTION_REQUEST_VALUE:
						# Respond to the device with the handshake value
						# This will initiate a handshake and establish
						# connection.
						print('Mocap client at {}; initiating handshake'. \
								format(address))
						packet = struct.pack('<i', self.HANDSHAKE_VALUE)
						sock.sendto(packet, address)
						current_state = self.STATE_HANDSHAKE
					else:
						print('Request value is invalid. '
							'(received: {}, expected: {})'.format(
								request_value,
								self.CONNECTION_REQUEST_VALUE))
				elif current_state == self.STATE_HANDSHAKE:
					# Data received while handshaking...
					request_value = struct.unpack('<i', data)[0]
					if request_value == self.HANDSHAKE_VALUE:
						# Stream has been opened with the device.
						# Reply with the response
						packet = struct.pack('<i', self.HANDSHAKE_RESPONSE)
						sock.sendto(packet, address)
						self.client_address = address
						self.last_client_contact_msec = get_milliseconds()
						current_state = self.STATE_STREAMING
						print('Connected to imocap device. {}'.format(address))
					else:
						print('Handshake did not match! '
							'(received: {}, expected: {})'.format(
								request_value, self.HANDSHAKE_VALUE))


global_client = imocap_client()

class imocap_connect(bpy.types.Operator):
	bl_idname = "io_import_imocap.connect"
	bl_label = "connect"

	def execute(self, context):
		if not global_client.is_connected:
			global_client.connect()
			self.report({'INFO'}, 'Connected to host.')
		else:
			print('disconnect')
			global_client.disconnect()
		return {'FINISHED'}

class imocap_freeze(bpy.types.Operator):
	bl_idname = 'io_import_imocap.freeze'
	bl_label = 'Freeze'

	def execute(self, context):
		print('Freeze Transforms')
		global_client.freeze_rotations()
		return {'FINISHED'}

class UIPanel(bpy.types.Panel):
	bl_label = 'imocap control'
	bl_space_type = 'VIEW_3D'
	bl_region_type = 'UI'

	def draw(self, context):
		layout = self.layout
		scene = context.scene

		action_text = 'Connect' if not global_client.is_connected else 'Disconnect'
		self.layout.operator('io_import_imocap.connect', text=action_text)

		self.layout.operator('io_import_imocap.freeze', text='Freeze Transforms')

#
# startup / blender specific interface functions
#
def register():
	bpy.utils.register_module(__name__)

def unregister():
	bpy.utils.unregister_module(__name__)

if __name__ == "__main__":
	register()