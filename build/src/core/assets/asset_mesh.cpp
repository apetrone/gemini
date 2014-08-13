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
#include "renderer/renderer.h"

using namespace renderer;

namespace assets
{
	static glm::mat4 json_to_mat4(Json::Value& value)
	{
		glm::mat4 output;
		Json::ValueIterator it = value.begin();
		assert(value.size() > 0);
		float m[16] = {0};
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
		//LOGV( "Total Materials: %i\n", materials.size() );
		
		Json::ValueIterator mit = materials.begin();
		
		FixedArray<unsigned int> material_ids;
		material_ids.allocate(materials.size());
		
		unsigned int total_materials_read = 0;
		assets::Material * amat = 0;
		for( ; mit != materials.end(); ++mit )
		{
			Json::Value material = (*mit);
			std::string material_name = material["name"].asString();
			unsigned int material_id = material["material_id"].asUInt();
			
			StackString<MAX_PATH_SIZE> material_path;
			xstr_sprintf( &material_path[0], material_path.max_size(), "materials/%s", material_name.c_str() );
			amat = 0;
			amat = assets::materials()->load_from_path( material_path() );
			if ( !amat )
			{
				amat = assets::materials()->get_default();
			}
			
			assert(amat != 0);
//			amat->print_parameters();

			material_ids[ material_id ] = amat->Id();
			LOGV( "assigned material '%s' to (%i)\n", material_name.c_str(), material_ids[ material_id ] );
			
			++total_materials_read;
		}
		
		Json::Value geometry_list = root["geometry"];
//		LOGV( "Total Geometry: %i\n", geometry_list.size() );
		mesh->geometry.allocate(geometry_list.size());
		
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
			Json::Value colors = geometry_node["colors"];
			Json::Value blend_weights = geometry_node["blend_weights"];
			
			int material_id = -1;
			Json::Value material_id_node = geometry_node["material_id"];
			if (!material_id_node.isNull())
			{
				material_id = material_id_node.asInt();
			}
			
			const std::string& name = geometry_node["name"].asString();
			geometry->name = name.c_str();
			
			
			if ( material_id != -1 && total_materials_read > 0 /*&& material_id < current_material*/ )
			{
				geometry->material_id = material_ids[ material_id ];
//				LOGV( "using material %i %i\n", material_id, geometry->material_id );
			}
			else
			{
//				LOGV("using default material!\n");
				Material * default_material = assets::materials()->get_default();
				geometry->material_id = default_material->Id();
			}
			
//			LOGV( "geometry: %i, material_id: %i, name: %s\n", gid-1, material_id, geometry->name() );
//			LOGV( "# vertices: %i\n", positions.size()/3 );
//			LOGV( "# indices: %i\n", indices.size() );
//			LOGV( "# triangles: %i\n", indices.size()/3 );
			geometry->index_count = indices.size();
			geometry->indices.allocate(geometry->index_count);
			for( int i = 0; i < geometry->index_count; ++i )
			{
				geometry->indices[i] = indices[i].asInt();
			}
			
			geometry->vertex_count = positions.size() / 3;
			geometry->vertices.allocate(geometry->vertex_count);
			geometry->normals.allocate(geometry->vertex_count);
//			geometry->untransformed_vertices.allocate(geometry->vertex_count);

		
			
			if (!blend_weights.isNull() && !blend_weights.empty())
			{
				geometry->blend_indices.allocate(geometry->vertex_count);
				geometry->blend_weights.allocate(geometry->vertex_count);
				
				Json::ValueIterator blend_weight_iter = blend_weights.begin();
				int v = 0;
				for( ; blend_weight_iter != blend_weights.end(); ++blend_weight_iter, ++v)
				{
					Json::Value blend_weight = *blend_weight_iter;

					Json::Value jbi = blend_weight["indices"];
					Json::Value jbw = blend_weight["weights"];
					
					assert(jbi.size() > 0);
					int bone_indices[ MAX_VERTEX_WEIGHTS ] = {0};
					float bone_weights[ MAX_VERTEX_WEIGHTS ] = {0};
					
					glm::vec4 bw;
					for (int wid = 0; wid < jbi.size(); ++wid)
					{
						if (wid >= MAX_VERTEX_WEIGHTS)
							break;
						bone_indices[wid] = jbi[wid].asInt();
						bone_weights[wid] = jbw[wid].asFloat();
					}
					
					geometry->blend_indices[v] = glm::vec4(bone_indices[0], bone_indices[1], bone_indices[2], bone_indices[3]);
//					LOGV("blend_indices: %g %g %g %g\n", geometry->blend_indices[v].x, geometry->blend_indices[v].y, geometry->blend_indices[v].z, geometry->blend_indices[v].w);
					
					geometry->blend_weights[v] = glm::vec4(bone_weights[0], bone_weights[1], bone_weights[2], bone_weights[3]);
//					LOGV("blend_weights: %g %g %g %g\n", geometry->blend_weights[v].x, geometry->blend_weights[v].y, geometry->blend_weights[v].z, geometry->blend_weights[v].w);
				}

			}
			
			for( int v = 0; v < geometry->vertex_count; ++v )
			{
				geometry->vertices[v] = glm::vec3(positions[v*3].asFloat(), positions[v*3+1].asFloat(), positions[v*3+2].asFloat() );
//				geometry->untransformed_vertices[v] = geometry->vertices[v];
			}
			
			for( int v = 0; v < geometry->vertex_count; ++v )
			{
				geometry->normals[v] = glm::vec3(normals[v*3].asFloat(), normals[v*3+1].asFloat(), normals[v*3+2].asFloat() );
			}
			
			if (!uvs.isNull() && uvs.size() > 0 )
			{
				geometry->uvs.allocate(uvs.size());
				for( int i = 0; i < uvs.size(); ++i)
				{
					geometry->uvs[i].allocate(geometry->vertex_count);
					
					Json::Value& uv = uvs[i];
					for( int v = 0; v < geometry->vertex_count; ++v )
					{
						geometry->uvs[i][v].u = uv[v*2].asFloat();
						// Invert the UV coordinates on models when we import them.
						// TODO: should this be done in the exporter?
						geometry->uvs[i][v].v = 1.0f - uv[v*2+1].asFloat();
					}
				}
			}
			else
			{

				LOGW( "Mesh \"%s\" has no UV coordinates.\n", geometry->name() );
			}
			
			if (!colors.isNull() && colors.size() > 0)
			{
				geometry->colors.allocate(geometry->vertex_count);
				for (int v = 0; v < geometry->vertex_count; ++v)
				{
					Color& color = geometry->colors[v];
					int idx = v*4;
					color.r = (colors[idx].asFloat() * 255.0f);
					color.g = (colors[idx+1].asFloat() * 255.0f);
					color.b = (colors[idx+2].asFloat() * 255.0f);
					color.a = (colors[idx+3].asFloat() * 255.0f);
				}
			}



			//
			// setup debug normals now
#if 0
			mesh->geometry_vn.allocate(geometry_list.size());
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
//		LOGV("total bones: %i\n", mesh->total_bones);
		
		if (mesh->total_bones > 0)
		{
			mesh->bones.allocate(mesh->total_bones);
			
			Bone* bone = &mesh->bones[0];
			Json::ValueIterator bone_it = bone_list.begin();
			

			for( ; bone_it != bone_list.end(); ++bone_it)
			{
				Json::Value bone_node = *bone_it;
				bone->name = bone_node["name"].asString();
				
				Json::Value inverse_bind_pose = bone_node["inverse_bind_pose"];
				bone->inverse_bind_matrix = json_to_mat4(inverse_bind_pose);
				bone->bind_matrix = glm::inverse(bone->inverse_bind_matrix);
//				Json::Value local_transform = bone_node["transform"];
//				bone->local_transform = json_to_mat4(local_transform);

				LOGV("bone: %s\n", bone->name.c_str());
				
				bone++;
			}
		}
		
		// allocate animation bones
		AnimationData& anim = mesh->animation;
		
		// allocate enough data for all bones
		anim.scale.allocate(mesh->total_bones);
		anim.rotation.allocate(mesh->total_bones);
		anim.translation.allocate(mesh->total_bones);
		
		
		
		
		// Attempt to load animations?
		Json::Value animations = root["animations"];
		Json::ValueIterator animation_it = animations.begin();
		for( ; animation_it != animations.end(); ++animation_it)
		{
			Json::Value janimation = *animation_it;
			mesh->animation.duration_seconds = janimation["duration_seconds"].asFloat();
			float ticks_per_second = janimation["ticks_per_second"].asFloat();
			mesh->animation.frames_per_second = 30;
			mesh->animation.frame_delay_seconds = (1.0f/mesh->animation.frames_per_second);

			Json::Value node_list = janimation["nodes"];
			Json::ValueIterator node_it = node_list.begin();
//			mesh->animation.frames.allocate(node_list.size());
			LOGV("loading %i nodes...\n", node_list.size());

			for( ; node_it != node_list.end(); ++node_it)
			{
				Json::Value jnode = *node_it;
				std::string bone_name = jnode["name"].asString();


				if (!jnode["bone_index"].isNull() && !jnode["bone_parent"].isNull())
				{
					int32_t bone_index = jnode["bone_index"].asInt();
					int32_t bone_parent = jnode["bone_parent"].asInt();
					
					//assert(bone_index >= 0);
					
					if (bone_index >= 0)
					{
						Bone* bone = &mesh->bones[bone_index];
						bone->index = bone_index;
						
						LOGV("match %s to %s\n", bone_name.c_str(), bone->name.c_str());
						if (bone_parent != -1)
						{
							Bone* parent = &mesh->bones[bone_parent];
							assert(parent != 0);
							
							bone->parent_index = bone_parent;
						}
						
						
						Json::Value jkeys = jnode["keys"];
						read_keys_object(anim, bone, jkeys);
					}
				}
				

			}
		}
	
		Json::Value transform = root["transform"];
		if (!transform.isNull())
		{
			mesh->node_transform = json_to_mat4(transform);
		}
	
		return util::ConfigLoad_Success;
	} // mesh_load_from_json
	
	
	void read_vector_keys(KeyframeData<glm::vec3>& keydata, const Json::Value& root)
	{
		const Json::Value& times = root["time"];
		const Json::Value& values = root["value"];
	
		size_t total_times = times.size();
		assert(total_times > 0);
			
		size_t total_values = values.size();
		assert(total_values > 0);
				
		size_t total_vector_keys = total_values / 3;
		assert(total_vector_keys > 0);
		
		// read key times
		keydata.time.allocate(total_times);
		for(int i = 0; i < total_times; ++i)
		{
			keydata.time[i] = times[i].asFloat();
		}
		
		// read key values
		keydata.keys.allocate(total_vector_keys);
		for(int i = 0; i < total_vector_keys; ++i)
		{
			const Json::Value& x = values[i*3];
			const Json::Value& y = values[i*3+1];
			const Json::Value& z = values[i*3+2];
			
			glm::vec3& s = keydata.keys[i];
			s.x = x.asFloat();
			s.y = y.asFloat();
			s.z = z.asFloat();
		}
	}
	
	void read_quat_keys(KeyframeData<glm::quat>& keydata, const Json::Value& root)
	{
		const Json::Value& times = root["time"];
		const Json::Value& values = root["value"];
		
		size_t total_times = times.size();
		assert(total_times > 0);
		
		size_t total_values = values.size();
		assert(total_values > 0);
		
		size_t total_vector_keys = total_values / 4;
		assert(total_vector_keys > 0);
		
		keydata.keys.allocate(total_vector_keys);

		// read key times
		keydata.time.allocate(total_times);
		for(int i = 0; i < total_times; ++i)
		{
			keydata.time[i] = times[i].asFloat();
		}

		// read key values
		for(int i = 0; i < total_vector_keys; ++i)
		{
			const Json::Value& x = values[i*4];
			const Json::Value& y = values[i*4+1];
			const Json::Value& z = values[i*4+2];
			const Json::Value& w = values[i*4+3];
			
			glm::quat& s = keydata.keys[i];
			s.x = x.asFloat();
			s.y = y.asFloat();
			s.z = z.asFloat();
			s.w = w.asFloat();
		}
	}
	
	void read_keys_object(AnimationData& anim, Bone* bone, Json::Value& jkeys)
	{
		const Json::Value& scale = jkeys["scale"];
		const Json::Value& rotation = jkeys["rotation"];
		const Json::Value& translation = jkeys["translation"];

		KeyframeData<glm::vec3>& scale_channel = anim.scale[bone->index];
		KeyframeData<glm::quat>& rotation_channel = anim.rotation[bone->index];
		KeyframeData<glm::vec3>& translation_channel = anim.translation[bone->index];

		// read keys
		read_vector_keys(scale_channel, scale);
		read_quat_keys(rotation_channel, rotation);
		read_vector_keys(translation_channel, translation);
	}

	void mesh_construct_extension( StackString<MAX_PATH_SIZE> & extension )
	{
		extension = ".model";
	} // mesh_construct_extension
	
	Geometry::Geometry()
	{
		material_id = 0;
		vertex_count = 0;
		index_count = 0;
		draw_type = renderer::DRAW_TRIANGLES;
		
		attributes = 0;
		vertexbuffer = 0;
	}
	
	Geometry::~Geometry()
	{
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

		// setup attributes
		if ( attributes == 0 )
		{
			// always has at least a position
			descriptor.add( VD_FLOAT3 );
			
			if ( !normals.empty() )
			{
				ShaderString normals = "normals";
				attributes |= find_parameter_mask( normals );
				descriptor.add( VD_FLOAT3 );
			}
			
			if ( !colors.empty() )
			{
				ShaderString colors = "colors";
				attributes |= find_parameter_mask( colors );
				descriptor.add( VD_UNSIGNED_BYTE4 );
			}
			
			if ( uvs.size() > 0 )
			{
				ShaderString uv0 = "uv0";
				attributes |= find_parameter_mask( uv0 );
				descriptor.add( VD_FLOAT2 );
			}
			
			if ( uvs.size() > 1 )
			{
				ShaderString uv1 = "uv1";
				attributes |= find_parameter_mask( uv1 );
				descriptor.add( VD_FLOAT2 );
			}
			
			if (!blend_indices.empty() && !blend_weights.empty())
			{
				ShaderString hardware_skinning = "hardware_skinning";
				attributes |= find_parameter_mask(hardware_skinning);
				
				ShaderString node_transforms = "node_transforms";
				attributes |= find_parameter_mask(node_transforms);
				
				descriptor.add(VD_FLOAT4);
				descriptor.add(VD_FLOAT4);
			}
		}
		
		if (!this->vertexbuffer)
		{
			this->vertexbuffer = renderer::driver()->vertexbuffer_from_geometry( descriptor, this );
		}
		
		if ( !this->is_animated() )
		{
			renderer::driver()->vertexbuffer_upload_geometry( this->vertexbuffer, /*descriptor, */ this );
		}
		//		vertexstream.create( this->vertex_count, this->index_count, renderer::BUFFER_STATIC );
		//		this->vertexbuffer = renderer::driver()->vertexbuffer_create( descriptor, this->draw_type, renderer::BUFFER_STATIC, descriptor.calculate_vertex_stride(), this->vertex_count, this->index_count );
	}
	
	
	//AnimationData::Frame::Frame()
//		rotation{{rotation_value.x}, {rotation_value.y}, {rotation_value.z}, {rotation_value.w}},
//		translation{{position_value.x}, {position_value.y}, {position_value.z}}
	//{
	
	//}

	Mesh::Mesh()
	{
	} // Mesh

	
//	void Mesh::alloc( unsigned int num_geometry )
//	{
//		total_geometry = num_geometry;
//		//		geometry = CREATE_ARRAY( Geometry,  num_geometry );
//	} // alloc
	
	void Mesh::release()
	{
	} // release
	
	void Mesh::prepare_geometry()
	{
		assert(geometry.size() != 0);
		
		for( unsigned int geo_id = 0; geo_id < geometry.size(); ++geo_id )
		{
			assets::Geometry * g = &geometry[ geo_id ];
			g->render_setup();
		}
	} // prepare_geometry

	AssetLoadStatus mesh_load_callback( const char * path, Mesh * mesh, const AssetParameters & parameters )
	{
		if ( util::json_load_with_callback(path, mesh_load_from_json, mesh, true ) == util::ConfigLoad_Success )
		{
			return AssetLoad_Success;
		}
		
		return AssetLoad_Failure;
	} // mesh_load_callback
	
}; // namespace assets
