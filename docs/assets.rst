Asset Creation
---------------

-----------------
World Dimensions
-----------------
The game has 1 unit == 1 meter.

- Stair steps = 18.75 cm tall
- Character = 1.82m tall, .5m wide/deep

-----------------
Models
-----------------
A model consists of multiple files.  Below is a breakdown and description of each.

A typical model might look like the following:

	.. code-block:: bash

		- character.model/
		  * mesh
		  * collision
		  * skeleton
		  * idle.animation
		  * jump.animation

mesh:
	A mesh file describes the geometry for a model. Models may have an arbitrary
	number of geometry descriptions -- as these are split by material in the file.

	Each geometry description is comprised of vertex indices -- which describe the triangles of the geometry.
	Vertex positions and associated data can include:

		* normals
		* uv layer sets
		* blend weights
		* blend indices
		* tangent-space normals
		* per-vertex colors

	A material block is also included which lists the material names and associated
	id referenced by a mesh node.

	Optionally, an "export_info" block is included to hold meta-data about the exported model
	and the application/machine used to export it.

collision:
	The collision mesh used for this model

skeleton:
	A skeleton defines a hierarchy of joints.
	It's a list of dictionaries; one per joint.

	.. code-block:: c

		{
			"name": The name of this joint
			"parent": An index into this skeletal list referencing this bone's parent
		}

animation:
	An animation intended to be used with the skeleton included in this mesh.
	Each file defines a different 'sequence', which can be played or blended.

	.. code-block:: c

		{
			"duration_seconds": the duration of this sequence expressed in seconds
			"frames_per_second": the playback rate of sequence expressed in frames per second
			"name": the name of this sequence
		}

	A "nodes" block holds a list of references to the bones found in skeleton.
	scale, rotation, and translation blocks are specified for each bone with a list of
	time and values for each frame of the animation.
	In all three of those blocks, the "time" is the defined as:

	.. code-block:: c

		"time": [] # a list of timestamps from 0 to duration_seconds at 1000/frames_per_second intervals.

	Each node in the list is defined as:

	.. code-block:: c

		{
			"name": The referenced bone name in the skeleton

			"rotation":
			{
				"time": [],
				"value": []
			},
			"scale":
			{
				"time": [],
				"value": []
			},
			"translation":
			{
				"time": [],
				"value": []
			}
		}

----------------------
Exporting Models
----------------------

The model conversion tool (muse) has direct support for Autodesk's FBX file
format. Any DCC tool which can export to this, is in theory, supported.

----------------------
Exporting from Blender
----------------------
Exporting from Blender is most reliable with the following settings.

[Skeletal Animations]

Version: FBX 6.1 ASCII
+ Selected Objects
+ Animation
+ All Actions
- Uncheck Default Take

NOTES:

	* FBX 7.4 binary mangles the action names

----------------------
Exporting from Maya LT
----------------------
Maya LT hasn't been tested yet, but I'm sure full FBX support is there.
