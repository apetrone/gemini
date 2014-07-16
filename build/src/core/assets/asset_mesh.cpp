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
#include <gemini/util/configloader.h>
#include <gemini/core/log.h>

#include "assets.h"
#include "assets/asset_mesh.h"
#include "renderer.h"

using namespace renderer;

namespace assets
{
	static glm::mat4 json_to_mat4(Json::Value& value)
	{
		glm::mat4 output;
		Json::ValueIterator it = value.begin();

		float m[16];
		size_t i = 0;
		for( ; it != value.end(); ++it, ++i )
		{
			Json::Value item = *it;
			m[i] = item.asFloat();
		}
		
		
		return glm::mat4(
			m[0], m[1], m[2], m[3],
			m[4], m[5], m[6], m[7],
			m[8], m[9], m[10], m[11],
			m[12], m[13], m[14], m[15]
		);
	}

	// -------------------------------------------------------------
	// Mesh
	
	util::ConfigLoadStatus mesh_load_from_json( const Json::Value & root, void * data )
	{
		Mesh * mesh = (Mesh*)data;
//		LOGV( "Loading mesh %s\n", mesh->name() );
		
		assets::Material * default_mat = assets::materials()->load_from_path("materials/default");
		if ( !default_mat )
		{
			LOGE( "Could not load the default material!\n" );
			return util::ConfigLoad_Failure;
		}
		
		Json::Value materials = root["materials"];
		LOGV( "Total Materials: %i\n", materials.size() );
		
		Json::ValueIterator mit = materials.begin();
		
		unsigned int * material_ids = CREATE_ARRAY( unsigned int, materials.size() );
		unsigned int current_material = 0;
		assets::Material * amat = 0;
		for( ; mit != materials.end(); ++mit )
		{
			Json::Value material = (*mit);
			std::string material_name = material["name"].asString();
			
			StackString<MAX_PATH_SIZE> material_path;
			xstr_sprintf( &material_path[0], material_path.max_size(), "materials/%s", material_name.c_str() );
			amat = 0;
			amat = assets::materials()->load_from_path( material_path() );
			if ( amat )
			{
				material_ids[ current_material ] = amat->Id();
			}
			else
			{
				amat = assets::materials()->get_default();
				material_ids[ current_material ] = amat->Id();
			}
			//			LOGV( "assigned material '%s' to (%i)\n", material_name.c_str(), material_ids[ current_material ] );
			++current_material;
		}
		
		Json::Value geometry_list = root["geometry"];
		//		LOGV( "Total Geometry: %i\n", geometry_list.size() );
		mesh->total_geometry = geometry_list.size();
		mesh->geometry = CREATE_ARRAY( Geometry, mesh->total_geometry );
		mesh->geometry_vn = CREATE_ARRAY( Geometry, mesh->total_geometry );
		
		Geometry * geometry;
		int gid = 0;
		//		int vnid = 0;
		Json::ValueIterator git = geometry_list.begin();
		for( ; git != geometry_list.end(); ++git )
		{
			geometry = &mesh->geometry[ gid++ ];
			geometry->draw_type = renderer::DRAW_INDEXED_TRIANGLES;
			
			Json::Value geometry_node = *git;
			Json::Value positions = geometry_node["positions"];
			Json::Value indices = geometry_node["indices"];
			Json::Value normals = geometry_node["normals"];
			Json::Value uvs = geometry_node["uvs"];
			int material_id = geometry_node["material_id"].asInt();
//			LOGV( "geometry: %i, material_id: %i\n", gid-1, material_id );
//			LOGV( "# vertices: %i\n", positions.size()/3 );
//			LOGV( "# indices: %i\n", indices.size() );
//			LOGV( "# triangles: %i\n", indices.size()/3 );
			geometry->index_count = indices.size();
			geometry->indices = CREATE_ARRAY(IndexType, geometry->index_count);
			for( int i = 0; i < geometry->index_count; ++i )
			{
				geometry->indices[i] = indices[i].asInt();
			}
			
			geometry->vertex_count = positions.size() / 3;
			geometry->vertices = CREATE_ARRAY( glm::vec3, geometry->vertex_count );
			geometry->normals = CREATE_ARRAY( glm::vec3, geometry->vertex_count );
			geometry->uvs = CREATE_ARRAY( UV, geometry->vertex_count );
			
			for( int v = 0; v < geometry->vertex_count; ++v )
			{
				geometry->vertices[v] = glm::vec3(positions[v*3].asFloat(), positions[v*3+1].asFloat(), positions[v*3+2].asFloat() );
			}
			
			for( int v = 0; v < geometry->vertex_count; ++v )
			{
				geometry->normals[v] = glm::vec3(normals[v*3].asFloat(), normals[v*3+1].asFloat(), normals[v*3+2].asFloat() );
			}
			
			if ( uvs.size() > 0 )
			{
				for( int v = 0; v < geometry->vertex_count; ++v )
				{
					geometry->uvs[v].u = uvs[v*2].asFloat();
					
					// Invert the UV coordinates on models when we import them.
					// TODO: should this be done in the exporter?
					geometry->uvs[v].v = 1.0f - uvs[v*2+1].asFloat();
				}
			}
			else
			{
				LOGW( "Mesh has no UV coordinates!\n" );
			}
			
#if 0
			geometry->colors = new aengine::Color[ geometry->vertex_count ];
			for( int v = 0; v < geometry->vertex_count; ++v )
			{
				geometry->colors[v] = aengine::Color( 255, 255, 255, 255 );
			}
#endif
			
			
			if ( material_id != -1 && current_material > 0 /*&& material_id < current_material*/ )
			{
				geometry->material_id = material_ids[ material_id ];
				//				LOGV( "using material %i %i\n", material_id, geometry->material_id );
			}
			else
			{
				Material * default_material = assets::materials()->get_default();
				geometry->material_id = default_material->Id();
			}
			
#if 0
			for( int i = 0; i < geometry->index_count; ++i )
			{
				int idx = geometry->indices[i];
				//			LOGV( "%i, (%g %g %g), (%g %g %g), (%g %g)\n",
				//				   idx,
				//				   geometry->vertices[ idx ][0], geometry->vertices[ idx ][1], geometry->vertices[ idx ][2],
				//				   geometry->normals[ idx ][0], geometry->normals[ idx ][1], geometry->normals[ idx ][2],
				//				   geometry->uvs[ idx ].u, geometry->uvs[ idx ].v );
				Vector3 v = geometry->vertices[ idx ];
				Vector3 n = geometry->normals[ idx ];
				UV uv = geometry->uvs[ idx ];
				LOGV( "i %i: %i = %g %g %g | %g %g %g | %g %g\n", i, idx, v[0], v[1], v[2], n[0], n[1], n[2], uv.u, uv.v );
			}
#endif
			
			//
			// setup debug normals now
#if 0
			if ( geometry->normals )
			{
				
				Geometry * vertex_normals = &mesh->geometry_vn[ vnid++ ];
				vertex_normals->draw_type = renderer::DRAW_LINES;
				if ( default_mat )
				{
					vertex_normals->material_id = default_mat->Id();
				}
				
				const float normal_scale = 0.25f;
				Color vertex_normal_color(255, 0, 255);
				vertex_normals->vertex_count = geometry->vertex_count * 2;
				vertex_normals->index_count = vertex_normals->vertex_count;
				vertex_normals->vertices = CREATE_ARRAY(glm::vec3, vertex_normals->vertex_count );
				vertex_normals->indices = CREATE_ARRAY(IndexType, vertex_normals->index_count );
				vertex_normals->colors = CREATE_ARRAY(Color, vertex_normals->vertex_count );
				vertex_normals->normals = 0;
				vertex_normals->uvs = 0;
				
				unsigned int vid;
				for( int v = 0; v < geometry->vertex_count; ++v )
				{
					vid = v*2;
					glm::vec3 start = geometry->vertices[v];
					glm::vec3 end = (start + geometry->normals[v] * normal_scale);
					vertex_normals->vertices[ vid ] = start;
					vertex_normals->vertices[ vid+1 ] = end;
					vertex_normals->indices[ vid ] = vid;
					vertex_normals->indices[ vid+1 ] = vid+1;
					vertex_normals->colors[ vid ] = vertex_normal_color;
					vertex_normals->colors[ vid+1 ] = vertex_normal_color;
				}
			}
#endif
		}
		

		// Process Bones
		Json::Value bone_list = root["bones"];
		mesh->total_bones = bone_list.size();
		mesh->bones = CREATE_ARRAY(Bone, mesh->total_bones);
		
		Bone* bone = &mesh->bones[0];
		Json::ValueIterator bone_it = bone_list.begin();
		for( ; bone_it != bone_list.end(); ++bone_it)
		{
			Json::Value bone_node = *bone_it;
			bone->name = bone_node["name"].asString();
			
			Json::Value inverse_bind_pose = bone_node["inverse_bind_pose"];
			bone->inverse_bind_matrix = json_to_mat4(inverse_bind_pose);

			LOGV("bone: %s\n", bone->name.c_str());
			
			bone++;
		}
		
		
		
		// Attempt to load animations?
		Json::Value animations = root["animations"];
		Json::ValueIterator animation_it = animations.begin();
		for( ; animation_it != animations.end(); ++animation_it)
		{
			Json::Value janimation = *animation_it;
			mesh->animation.duration_seconds = janimation["duration_seconds"].asFloat();
			mesh->animation.frames_per_second = janimation["frames_per_second"].asFloat();

			Json::Value node_list = janimation["nodes"];
			Json::ValueIterator node_it = node_list.begin();
			mesh->animation.transforms.allocate(node_list.size());
			LOGV("loading %i nodes...\n", node_list.size());
			
			size_t tr_id = 0;
			for( ; node_it != node_list.end(); ++node_it, ++tr_id)
			{
				AnimationData::BoneTransform* tr = &mesh->animation.transforms[tr_id];
				Json::Value jnode = *node_it;
				std::string bone_name = jnode["bone_name"].asString();
				Json::Value jkeys = jnode["keys"];
				tr->keys.allocate(jkeys.size());
				
				Json::ValueIterator jkey_it = jkeys.begin();
				size_t key_id = 0;
				LOGV("loading %i keys...\n", jkeys.size());
				for( ; jkey_it != jkeys.end(); ++jkey_it, ++key_id)
				{
					Json::Value matrix = *jkey_it;
					tr->keys[key_id] = json_to_mat4(matrix);
				}
				
			}
		}
		
		
		
		
		DEALLOC( material_ids );
		
		return util::ConfigLoad_Success;
	} // mesh_load_from_json
	
	void mesh_construct_extension( StackString<MAX_PATH_SIZE> & extension )
	{
		extension = ".model";
	} // mesh_construct_extension
	
	Geometry::Geometry()
	{
		vertices = 0;
		normals = 0;
		uvs = 0;
		indices = 0;
		material_id = 0;
		colors = 0;
		
		vertex_count = 0;
		index_count = 0;
		draw_type = renderer::DRAW_TRIANGLES;
		
		attributes = 0;
		is_animated = 0;
		vertexbuffer = 0;
	}
	
	Geometry::~Geometry()
	{
		using namespace glm;
		if ( vertices )
		{
			DESTROY_ARRAY( vec3, vertices, vertex_count );
			vertices = 0;
		}
		
		if ( normals )
		{
			DESTROY_ARRAY( vec3, normals, vertex_count );
			normals = 0;
		}
		
		if ( colors )
		{
			DESTROY_ARRAY( Color, colors, vertex_count );
			colors = 0;
		}
		
		if ( uvs )
		{
			DESTROY_ARRAY( UV, uvs, vertex_count );
			uvs = 0;
		}
		
		if ( indices )
		{
			DEALLOC( indices );
			indices = 0;
		}
		
		if ( this->vertexbuffer )
		{
			renderer::driver()->vertexbuffer_destroy( this->vertexbuffer );
		}
	}
	
#if 0
	void Geometry::alloc_vertices( unsigned int num_vertices )
	{
		vertex_count = num_vertices;
		vertices = CREATE_ARRAY( glm::vec3, num_vertices );
	} // alloc_vertices
	
	void Geometry::alloc_indices( unsigned int num_indices )
	{
		index_count = num_indices;
		indices = CREATE_ARRAY( renderer::IndexType, num_indices );
	} // alloc_indices
#endif
	
	void Geometry::render_setup()
	{
		VertexDescriptor descriptor;
		// if we already setup this geometry; skip
		if ( attributes > 0 )
		{
			return;
		}
		
		// always has at least a position
		descriptor.add( VD_FLOAT3 );
		
		if ( normals )
		{
			ShaderString normals = "normals";
			attributes |= find_parameter_mask( normals );
			descriptor.add( VD_FLOAT3 );
		}
		
		if ( colors )
		{
			ShaderString colors = "colors";
			attributes |= find_parameter_mask( colors );
			descriptor.add( VD_UNSIGNED_BYTE4 );
		}
		
		if ( uvs )
		{
			ShaderString uv0 = "uv0";
			attributes |= find_parameter_mask( uv0 );
			descriptor.add( VD_FLOAT2 );
		}
		
		this->vertexbuffer = renderer::driver()->vertexbuffer_from_geometry( descriptor, this );
		
		if ( !this->is_animated )
		{
			renderer::driver()->vertexbuffer_upload_geometry( this->vertexbuffer, this );
		}
		//		vertexstream.create( this->vertex_count, this->index_count, renderer::BUFFER_STATIC );
		//		this->vertexbuffer = renderer::driver()->vertexbuffer_create( descriptor, this->draw_type, renderer::BUFFER_STATIC, descriptor.calculate_vertex_stride(), this->vertex_count, this->index_count );
	}
	

	Mesh::Mesh()
	{
		init();
	} // Mesh
	
	void Mesh::init()
	{
		total_geometry = 0;
		geometry = 0;
		geometry_vn = 0;
		
		total_bones = 0;
		bones = 0;
	} // init
	
	void Mesh::alloc( unsigned int num_geometry )
	{
		total_geometry = num_geometry;
		//		geometry = CREATE_ARRAY( Geometry,  num_geometry );
	} // alloc
	
	void Mesh::purge()
	{
		DESTROY_ARRAY( Geometry, geometry, total_geometry );
		geometry = 0;
		
		DESTROY_ARRAY( Geometry, geometry_vn, total_geometry );
		geometry_vn = 0;

		DESTROY_ARRAY(Bone, bones, total_bones);
		bones = 0;
		
		init();
	} // purge
	
	void Mesh::release()
	{
		purge();
	} // release
	
	void Mesh::prepare_geometry()
	{
		for( unsigned int geo_id = 0; geo_id < total_geometry; ++geo_id )
		{
			assets::Geometry * g = &geometry[ geo_id ];
			g->render_setup();
		}
	} // prepare_geometry
	
	//	unsigned int get_total_meshes()
	//	{
	//		return mesh_lib->total_asset_count();
	//	} // get_total_meshes
	//
	//	Mesh * load_mesh( const char * filename, unsigned int flags, bool ignore_cache )
	//	{
	//		return mesh_lib->load_from_path( filename, 0, ignore_cache );
	//	} // load_mesh
	
	//	void for_each_mesh( MeshIterator fn, void * userdata )
	//	{
	//		if ( !fn )
	//		{
	//			LOGE( "Invalid mesh iterator function!\n" );
	//			return;
	//		}
	//
	//		mesh_lib->for_each( fn, userdata );
	//	} // for_each_mesh
	
	
	//	void insert_mesh( const char * filename, Mesh * ptr )
	//	{
	//		if ( !mesh_lib->find_with_path( filename ) )
	//		{
	//			mesh_lib->take_ownership( filename, ptr );
	//		}
	//	} // insert_mesh
	
	//	Mesh * mesh_by_name( const char * filename )
	//	{
	//		return mesh_lib->find_with_path( filename );
	//	} // mesh_by_name
	//
	
	
	AssetLoadStatus mesh_load_callback( const char * path, Mesh * mesh, const AssetParameters & parameters )
	{
		if ( util::json_load_with_callback(path, mesh_load_from_json, mesh, true ) == util::ConfigLoad_Success )
		{
			return AssetLoad_Success;
		}
		
		return AssetLoad_Failure;
	} // mesh_load_callback
	
}; // namespace assets
