// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM,OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
// -------------------------------------------------------------
#pragma once

#include <vector>

#include <gemini/mem.h>
#include <gemini/mem_stl_allocator.h>
#include <gemini/util/stackstring.h>

#include "assets.h"
#include "renderer.h"

#include <gemini/util/fixedarray.h>
#include "keyframechannel.h"

#include <json/json.h>

namespace assets
{
	// -------------------------------------------------------------
	// Mesh
	
	struct Geometry : public renderer::Geometry
	{
		StackString<128> name;
		unsigned int material_id;
		
		Geometry();
		~Geometry();
		
		// set this geometry up for rendering
		void render_setup();
		
	}; // Geometry
	
	// TEMP struct to test the whole shebang.
	struct AnimationData
	{
		KeyframeData<glm::vec3> scale;
		KeyframeData<glm::quat> rotation;
		KeyframeData<glm::vec3> translation;
				
		// duration of the full animation, in seconds
		float duration_seconds;
		
		float frames_per_second;
		float frame_delay_seconds; // 1.0f / frames_per_second
		
		// total frames in this animation
		uint32_t total_frames;
	};
	
	
	struct Bone
	{
		Bone()
		{
			parent_index = -1;
		}
		~Bone() {}
		
		// name of this bone
		std::string name;
		
		// local to bone space (vertex to bone)
		glm::mat4 inverse_bind_matrix;
		
		// from bone space to local space
		glm::mat4 bind_matrix;

		glm::mat4 local_transform;

		glm::mat4 world_transform;
		
		// -1: No parent
		int32_t parent_index;
	};
	
	struct Mesh : public Asset
	{
		unsigned short total_geometry;
		Geometry * geometry;
		Geometry * geometry_vn;
		glm::mat4 world_matrix;
		StackString<MAX_PATH_SIZE> path;
		
		unsigned short total_bones;
		Bone* bones;
		
		Mesh();
		void init();
		void alloc( unsigned int num_geometry );
		void purge();
		virtual void release();
		
		// prepare all geometry
		void prepare_geometry();
		
		// upload all geometry
		//		void upload_geometry();
		
		// For now, we only have room for a single animation -- so make it worthwhile.
		AnimationData animation;
	}; // Mesh
	
	// EXPERIMENTAL
	void read_keys_object(assets::Mesh* mesh, Json::Value& jkeys);
	void read_keys_array(assets::Mesh* mesh, Json::Value& jkeys);
	
	AssetLoadStatus mesh_load_callback( const char * path, Mesh * mesh, const AssetParameters & parameters );
	void mesh_construct_extension( StackString<MAX_PATH_SIZE> & extension );
	
	DECLARE_ASSET_LIBRARY_ACCESSOR(Mesh, AssetParameters, meshes);
}; // namespace assets
