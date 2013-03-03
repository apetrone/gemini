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

#include "platform.hpp"
#include "stackstring.hpp"
#include "assets.hpp"
#include "mathlib.h" // for glm
#include "color.hpp"
#include "renderer.hpp"


namespace assets
{
	
	
	struct UV
	{
		float u, v;
	};
	
	struct Geometry
	{
		enum
		{
			DRAW_LINES = 0,
			DRAW_TRIANGLES = 1,
			DRAW_PARTICLES = 2,
		};
		
		unsigned int vertex_count;
		unsigned int index_count;
		unsigned short draw_type;
		
		glm::vec3 * vertices;
		glm::vec3 * normals;
		Color * colors;
		UV * uvs;
		renderer::IndexType * indices;
		
		StackString<128> name;
		
		unsigned int material_id;
		
		Geometry();
		void alloc_vertices( unsigned int num_vertices );
		void alloc_indices( unsigned int num_indices );
		
		
		
//		RenderData * render_data;
		
		
		
		
	}; // Geometry
	
	
	struct Mesh : public virtual Asset
	{
		unsigned short total_geometry;
		Geometry * geometry;
		Geometry * geometry_vn;
		Matrix4 world_matrix;
		StackString<MAX_PATH_SIZE> path;
		
		Mesh();
		void alloc( unsigned int num_geometry );
		void init();
		void purge();
		virtual void release()
		{
		}
	}; // Mesh
	
	
	typedef void (*MeshIterator)( Mesh * mesh, void * userdata );
	unsigned int get_total_meshes();
	
	void for_each_mesh( MeshIterator fn, void * userdata = 0 );
	
	Mesh * load_mesh( const char * filename, unsigned int flags = 0, bool ignore_cache = false );
	Mesh * mesh_by_name( const char * filename );
	
	// inserts a mesh from an object and associate it with a filename
	void insert_mesh( const char * filename, Mesh * ptr );
}; // namespace assets