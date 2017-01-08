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
Assets
---------------------------
- have assets in a centralized system
- cache assets to avoid loading resources multiple times
- update assets in place
- different assets per platform as needed