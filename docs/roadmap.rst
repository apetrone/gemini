Roadmap - 2017
---------------------------

Priorities for future developments. Listed in order of descending
priority.

- runtime: Hot loading of shaders (render system v2)
	- ability to load in-place
		* shaders
		* models
		* textures
	- ability to detect changes
	- perhaps pub/sub for asset loading between editor and engine?
- editor: Asset Pipeline / Processing framework
- editor/engine separation
	Pros of separation:
	- Clearly defined boundaries between code and responsibilities.
		For now, I'm going with a separate engine and editor.
		I'd rather have the clear separation. I'm sure I will come around
		and want an integrated one later, but that's the point. I want to
		run into all of these problems so I can intelligently reason
		one way or the other.


- runtime: Oriented Bounding Boxes
- engine: Joint-specific hit boxes


---------------------------
Brainstorming
---------------------------

Ideas or nice to haves I want to learn or integrate.

- live updates from Blender

- deployment system
	- Use itch.io? or Steam? Leverage precache code?
	- How are updates rolled out?



---------------------------
Editor
---------------------------


---------------------------
Asset Compiler
---------------------------
- bakes assets down to specific platforms
- should support platform-specific formats and configurations
- embedded in the editor
- multi-threaded work queue

---------------------------
Asset Server
---------------------------
- invokes asset compiler
- caches assets locally?
- serves over network to connected clients
- embedded in the editor


update render2 with missing features from old renderer.
---------------------------
.. image source formats: RGB, RGBA, RED
.. image internal formats: RGBA, RED
.. srgb image to internal: SRGB8_ALPHA8, GL_RED

.. vbo draw type (stream/static)
.. vertex descriptor: FLOAT2, FLOAT3, FLOAT4, INT4, UNSIGNED_INT, UBYTE3, UBYTE4

.. textures
.. - unpack alignment
.. - texture type
.. - tex2d
.. - texture_cube_map
.. - GL_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER
.. - linear filtering
.. - nearest filtering
.. - mipmap generation
.. - update textures (texsubimage)

.. render target
.. - framebuffer
.. - renderbuffer: depth_component24, depth_attachment
.. - color attachment 0

.. uniforms
.. mat4
.. 1i
.. 3f
.. 4f
sampler2d
texture units
.. arbitrary state change
.. blendfunc

.. framebuffer srgb

run_command + post_command

material -> uniform parameter conversion

.. shaders:
.. - bind attributes
.. - bind uniforms
- bind uniform block

gl function logger stack

-------------------------
missing from render2
- cube map support
- uniform blocks
- vertex descriptor types
- additional blend states

- multi-parameter constants (sampler2d and texture unit)