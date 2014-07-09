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
#include "assets.h"
#include "assets/asset_material.h"
#include "configloader.h" // for ConfigLoadStatus, Json::
#include "renderer.h"

using namespace renderer;

namespace assets
{
	// -------------------------------------------------------------
	// Material
	
	void Material::release()
	{
		if ( num_parameters )
		{
			DESTROY_ARRAY(Parameter, parameters, num_parameters);
		}
	} // release
	
	void Material::calculate_requirements()
	{
		// calculate material requirements
		this->requirements = 0;
		
		for( int id = 0; id < this->num_parameters; ++id )
		{
			Material::Parameter * param = &this->parameters[ id ];
			unsigned int mask = find_parameter_mask( param->name );
			//			LOGV( "param \"%s\" -> %i\n", param->name(), mask );
			this->requirements |= mask;
		}
		
		//		LOGV( "Material requirements: %i\n", this->requirements );
	} // calculate_requirements
	
	Material::Parameter * Material::parameter_by_name( const char * name )
	{
		Material::Parameter * parameter = 0;
		
		// this should have a hash table... oh well
		for( int i = 0; i < this->num_parameters; ++i )
		{
			if ( xstr_nicmp( this->parameters[i].name.c_str(), name, xstr_len(name) ) == 0 )
			{
				parameter = &this->parameters[i];
				break;
			}
		}
		
		return parameter;
	} // parameter_by_name
	
	void Material::allocate_parameters( unsigned int max_parameters )
	{
		if ( this->parameters )
		{
			DESTROY_ARRAY(Parameter, parameters, this->num_parameters);
		}
		
		this->num_parameters = max_parameters;
		this->parameters = CREATE_ARRAY(Parameter, max_parameters);
	} // allocate_parameters
	
	void Material::set_parameter_name( unsigned int id, const char * name )
	{
		this->parameters[id].name = name;
	} // set_parameter_name
	
	void Material::set_parameter_vec4( unsigned int id, const glm::vec4 & vec )
	{
		this->parameters[id].vecValue = vec;
		this->parameters[id].type = MP_VEC4;
	} // set_parameter_vec4
	
	
	enum ParamFlags
	{
		PF_TYPE = 1,
		PF_VALUE
	};

	
	util::ConfigLoadStatus material_load_from_json( const Json::Value & root, void * data )
	{
		Material * material = (Material*)data;
		Json::Value name = root["name"];
		Json::Value texture = root["texture"];
		Json::Value type = root["type"];
		Json::Value shader = root["shader"];
		
		material->flags = 0;
		material->num_parameters = 0;
		material->parameters = 0;
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
		
		material->num_parameters = param_list.size();
		if ( material->num_parameters )
		{
			material->parameters = CREATE_ARRAY( Material::Parameter, material->num_parameters );
			Material::Parameter * parameter;
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
					return util::ConfigLoad_Failure;
				}
				
				if ( parameter->type == MP_INT )
				{
					Json::Value value = plist.get( "value", "" );
					if ( !value.isNull() )
					{
						//						param_flags |= PF_VALUE;
						parameter->intValue = atoi( value.asString().c_str() );
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
						assets::Texture * tex = assets::textures()->load_from_path( texture_param.asString().c_str() );
						parameter->intValue = tex->Id();
						//						LOGV( "param value: %i\n", parameter->intValue );
						
						parameter->texture_unit = texture_unit_for_map( parameter->name );
						//						LOGV( "texture unit: %i\n", parameter->texture_unit );
					}
					else
					{
						return util::ConfigLoad_Failure;
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
						return util::ConfigLoad_Failure;
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
						int results = sscanf( value.asString().c_str(), "%g,%g,%g,%g", &parameter->vecValue[0], &parameter->vecValue[1], &parameter->vecValue[2], &parameter->vecValue[3] );
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
					return util::ConfigLoad_Failure;
				}
			} // read all shader parameters
		} //  num_parameters
		
		
		material->calculate_requirements();
		
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
		return util::ConfigLoad_Success;
	}
	

	
	unsigned int material_type_to_parameter_type( const char * name )
	{
		if ( xstr_nicmp( name, "int", 0 ) == 0 )
		{
			return MP_INT;
		}
		else if ( xstr_nicmp( name, "samplerCube", 0 ) == 0 )
		{
			return MP_SAMPLER_CUBE;
		}
		else if ( xstr_nicmp( name, "sampler", 0 ) == 0 )
		{
			return MP_SAMPLER_2D;
		}
		else if ( xstr_nicmp( name, "vec4", 0 ) == 0 )
		{
			return MP_VEC4;
		}
		
		LOGW( "Couldn't find material parameter with name: %s\n", name );
		return 0;
	} // material_type_to_parameter_type
	
	int material_parameter_type_to_render_state( unsigned int type )
	{
		int params[] =
		{
			DC_UNIFORM1i,
			DC_UNIFORM_SAMPLER_2D,
			DC_UNIFORM_SAMPLER_CUBE,
			DC_UNIFORM4f
		};
		
		return params[ type ];
	} // material_parameter_type_to_render_state
	

	
	unsigned int texture_unit_for_map( ShaderString & name )
	{
		if ( xstr_nicmp( name.c_str(), "diffusemap", 0 ) == 0 )
		{
			return 0;
		}
		else if ( xstr_nicmp( name.c_str(), "normalmap", 0 ) == 0 )
		{
			return 1;
		}
		else if ( xstr_nicmp( name.c_str(), "specularmap", 0 ) == 0 )
		{
			return 2;
		}
		else if ( xstr_nicmp( name.c_str(), "cubemap", 0 ) == 0 )
		{
			return 0;
		}
		
		return 0;
	} // texture_unit_for_map
	
	AssetLoadStatus material_load_callback( const char * path, Material * material, const AssetParameters & parameters )
	{
		if (util::json_load_with_callback( path, material_load_from_json, material, true ) == util::ConfigLoad_Success )
		{
			return AssetLoad_Success;
		}
		
		return AssetLoad_Failure;
	} // material_load_callback
	
	void material_construct_extension( StackString<MAX_PATH_SIZE> & extension )
	{
		extension = ".material";
	} // material_construct_extension
}; // namespace assets
