// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//		* Redistributions of source code must retain the above copyright notice,
//		this list of conditions and the following disclaimer.

//		* Redistributions in binary form must reproduce the above copyright notice,
//		this list of conditions and the following disclaimer in the documentation
//		and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//		 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------
#include <core/configloader.h>
#include <core/str.h>

#include "assets.h"
#include "assets/asset_material.h"
#include "renderer/renderer.h"

using namespace gemini::renderer;

namespace gemini
{
	namespace assets
	{
		// -------------------------------------------------------------
		// Material
		
		void Material::release()
		{
		} // release
		

		
		enum ParamFlags
		{
			PF_TYPE = 1,
			PF_VALUE
		};

		
		core::util::ConfigLoadStatus material_load_from_json( const Json::Value & root, void * data )
		{
			Material * material = (Material*)data;
			Json::Value name = root["name"];
			Json::Value texture = root["texture"];
			Json::Value type = root["type"];
			Json::Value shader = root["shader"];
			
			material->flags = 0;
			if (!material->parameters.empty())
			{
				material->parameters.clear();
			}
			//		material->requirements = 0;
			//		int required_params = PF_TYPE | PF_VALUE;
			
			if ( !name.empty() )
			{
				material->name = name.asString().c_str();
			}
			
			if ( !type.empty() )
			{
				if ( type.asString() == "alpha" )
				{
					material->flags |= Material::BLENDING;
				}
				else if ( type.asString() == "cubemap" )
				{
					material->flags |= Material::CUBEMAP;
				}
			}
			
			// parse and load shader params
			Json::Value param_list = root["params"];
			//		LOGV( "Total Parameters: %i\n", param_list.size() );
			Json::ValueIterator piter = param_list.begin();
			Json::ValueIterator piter_end = param_list.end();
			

			if (!param_list.empty())
			{
				material->parameters.resize(param_list.size());
				MaterialParameter * parameter;
				unsigned int param_id = 0;
				for( ; piter != piter_end; ++piter, ++param_id )
				{
					parameter = &material->parameters[ param_id ];
					int param_flags = 0;
					Json::Value plist = (*piter);
					parameter->name = piter.key().asString().c_str();
					//				LOGV( "parameter-> %s\n", parameter->name() );
					
					Json::Value type = plist.get( "type", "" );
					if ( !type.isNull() )
					{
						param_flags |= PF_TYPE;
						std::string typestr = type.asString();
						//					LOGV( "type: %s\n", typestr.c_str() );
						
						// convert string to param type
						parameter->type = material_type_to_parameter_type( typestr.c_str() );
					}
					else
					{
						LOGE( "Couldn't find parameter field: \"type\"\n" );
						return core::util::ConfigLoad_Failure;
					}
					
					if ( parameter->type == MP_INT )
					{
						Json::Value value = plist.get( "value", "" );
						if ( !value.isNull() )
						{
							//						param_flags |= PF_VALUE;
							parameter->int_value = atoi( value.asString().c_str() );
							//						LOGV( "param value: %i\n", parameter->intValue );
						}
					}
	#if 0
					else if ( parameter->type == MP_SAMPLER_2D )
					{
						param_flags |= PF_VALUE;
						Json::Value texture_param = plist.get( "texture", Json::nullValue );
						
						if ( texture_param.isNull() )
						{
							LOGW( "texture param missing for \"sampler\" type\n" );
							param_flags &= ~PF_VALUE;
						}
						
						
						if ( param_flags & PF_VALUE )
						{
							assets::Texture * tex = assets::load_texture( texture_param.asString().c_str() );
							parameter->intValue = tex->Id();
							LOGV( "param value: %i\n", parameter->intValue );
							
							parameter->texture_unit = texture_unit_for_map( parameter->name );
							LOGV( "texture unit: %i\n", parameter->texture_unit );
						}
						else
						{
							return util::ConfigLoad_Failure;
						}
					}
	#endif
					else if ( parameter->type == MP_SAMPLER_2D )
					{
						param_flags |= PF_VALUE;
						Json::Value texture_unit = plist.get( "texture_unit", Json::nullValue );
						Json::Value texture_param = plist.get( "texture", Json::nullValue );
						Json::Value filter = plist.get("filter", Json::nullValue);
						

						
						if ( texture_param.isNull() )
						{
							LOGW( "texture param missing for \"sampler\" type\n" );
							param_flags &= ~PF_VALUE;
						}
						
						//					if ( texture_unit.isNull() )
						//					{
						//						LOGW( "texture_unit missing for \"sampler\" type\n" );
						//					}
						
						if ( param_flags & PF_VALUE )
						{
							assets::TextureParameters texparams;
							
							if (!filter.isNull())
							{
								const std::string& filter_type = filter.asString();
								LOGV("filter_type: %s\n", filter_type.c_str());
								if (filter_type == "none")
								{
									texparams.filter_type = image::FILTER_NONE;
								}
								else if (filter_type == "linear")
								{
									texparams.filter_type = image::FILTER_LINEAR_MIPMAP;
								}
							}
							
							
							assets::Texture * tex = assets::textures()->load_from_path(texture_param.asString().c_str(), texparams);
	//						parameter->int_value = tex->Id();

							parameter->texture = tex->texture;
							//						LOGV( "param value: %i\n", parameter->intValue );
							
							parameter->texture_unit = texture_unit_for_map( parameter->name );
							//						LOGV( "texture unit: %i\n", parameter->texture_unit );
						}
						else
						{
							return core::util::ConfigLoad_Failure;
						}
					}
					else if ( parameter->type == MP_SAMPLER_CUBE )
					{
						param_flags |= PF_VALUE;
						Json::Value texture_unit = plist.get( "texture_unit", Json::nullValue );
						Json::Value texture_param = plist.get( "texture", Json::nullValue );
						
						
						if ( texture_param.isNull() )
						{
							LOGW( "texture param missing for \"samplerCube\" type\n" );
							param_flags &= ~PF_VALUE;
						}
						
						if ( texture_unit.isNull() )
						{
							//						LOGW( "texture_unit missing for \"samplerCube\" type\n" );
						}
						
						if ( param_flags & PF_VALUE )
						{
							LOGW( "cubemap not implemented!\n" );
							//						assets::Texture * tex = 0; //assets::loadCubemap( texture_param.asString().c_str(), texture_flags );
							//						parameter->intValue = tex->Id();
							//						LOGV( "param value: %i\n", parameter->intValue );
							
							//						parameter->texture_unit = texture_unit_for_map( parameter->name );
							//						LOGV( "texture unit: %i\n", parameter->texture_unit );
						}
						else
						{
							return core::util::ConfigLoad_Failure;
						}
					}
					else if ( parameter->type == MP_VEC4 )
					{
						//					param_flags |= PF_VALUE;
						Json::Value value = plist.get( "value", Json::nullValue );
						if ( value.isNull() )
						{
							//						param_flags &= ~PF_VALUE;
							LOGW( "Unable to find value for \"vec4\" type\n" );
						}
						else
						{
							int results = sscanf( value.asString().c_str(), "%g,%g,%g,%g", &parameter->vector_value[0], &parameter->vector_value[1], &parameter->vector_value[2], &parameter->vector_value[3] );
							if ( results < 4 )
							{
								LOGW( "Unable to parse \"vec4\" type\n" );
								//							param_flags &= ~PF_VALUE;
							}
							else
							{
								//							LOGV( "parsed vec4: %g, %g, %g, %g\n", parameter->vecValue[0], parameter->vecValue[1], parameter->vecValue[2], parameter->vecValue[3] );
							}
						}
					}
					else
					{
						LOGE( "Couldn't find parameter field: \"value\"\n" );
						return core::util::ConfigLoad_Failure;
					}
				} // read all shader parameters
			} //  total_parameters
			
			
	//		material->calculate_requirements();
			
	#if 0
			StackString< MAX_PATH_SIZE > path = "conf/";
			if ( !shader.empty() )
			{
				path.append( shader.asString().c_str() );
			}
			else
			{
				//			path.append( ASSETS_DEFAULT_MATERIAL_SHADER_NAME );
			}
			
			//		material->shader = loadShader( path() );
			if ( !material->shader )
			{
				LOGW( "Couldn't find shader \"%s\" for material!\n", shader.asString().c_str() );
			}
	#endif
			return core::util::ConfigLoad_Success;
		}
		

		
		unsigned int material_type_to_parameter_type(const char* name)
		{
			if (core::str::case_insensitive_compare(name, "int", 0) == 0)
			{
				return MP_INT;
			}
			else if (core::str::case_insensitive_compare(name, "samplerCube", 0) == 0)
			{
				return MP_SAMPLER_CUBE;
			}
			else if (core::str::case_insensitive_compare(name, "sampler", 0) == 0)
			{
				return MP_SAMPLER_2D;
			}
			else if (core::str::case_insensitive_compare(name, "vec4", 0) == 0)
			{
				return MP_VEC4;
			}
			
			LOGW( "Couldn't find material parameter with name: %s\n", name );
			return 0;
		} // material_type_to_parameter_type

		

		
		unsigned int texture_unit_for_map(const std::string& name)
		{
			if (core::str::case_insensitive_compare(name.c_str(), "diffusemap", 0) == 0)
			{
				return 0;
			}
			else if (core::str::case_insensitive_compare(name.c_str(), "normalmap", 0) == 0)
			{
				return 1;
			}
			else if (core::str::case_insensitive_compare(name.c_str(), "specularmap", 0) == 0)
			{
				return 2;
			}
			else if (core::str::case_insensitive_compare(name.c_str(), "cubemap", 0) == 0)
			{
				return 0;
			}
			else if (core::str::case_insensitive_compare(name.c_str(), "lightmap", 0) == 0)
			{
				return 3;
			}
			
			return 0;
		} // texture_unit_for_map
		
		AssetLoadStatus material_load_callback(const char* path, Material* material, const AssetParameters& parameters )
		{
			if (core::util::json_load_with_callback(path, material_load_from_json, material, true) == core::util::ConfigLoad_Success)
			{
				return AssetLoad_Success;
			}
			
			return AssetLoad_Failure;
		} // material_load_callback
		
		void material_construct_extension( core::StackString<MAX_PATH_SIZE> & extension )
		{
			extension = ".material";
		} // material_construct_extension
	} // namespace assets
} // namespace gemini