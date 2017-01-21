// -------------------------------------------------------------
// Copyright (C) 2017- Adam Petrone
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

#include <renderer/renderer.h>

#include <runtime/configloader.h>
#include <runtime/material_library.h>

#include <core/logging.h>
#include <core/mem.h>


namespace gemini
{
	enum ParamFlags
	{
		PF_TYPE = 1,
		PF_VALUE
	};


	unsigned int texture_unit_for_map(const std::string& name);
	unsigned int material_type_to_parameter_type(const char* name);

	core::util::ConfigLoadStatus material_load_from_json(const Json::Value & root, void * data)
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

		if (!name.empty())
		{
			material->name = name.asString().c_str();
		}

		if (!type.empty())
		{
			if (type.asString() == "alpha")
			{
				material->flags |= Material::BLENDING;
			}
			else if (type.asString() == "cubemap")
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
			renderer::MaterialParameter * parameter;
			unsigned int param_id = 0;
			for (; piter != piter_end; ++piter, ++param_id)
			{
				parameter = &material->parameters[param_id];
				int param_flags = 0;
				Json::Value plist = (*piter);
				parameter->name = piter.key().asString().c_str();
				//				LOGV( "parameter-> %s\n", parameter->name() );

				Json::Value type = plist.get("type", "");
				if (!type.isNull())
				{
					param_flags |= PF_TYPE;
					std::string typestr = type.asString();
					//					LOGV( "type: %s\n", typestr.c_str() );

					// convert string to param type
					parameter->type = material_type_to_parameter_type(typestr.c_str());
				}
				else
				{
					LOGE("Couldn't find parameter field: \"type\"\n");
					return core::util::ConfigLoad_Failure;
				}

				if (parameter->type == renderer::MP_INT)
				{
					Json::Value value = plist.get("value", "");
					if (!value.isNull())
					{
						//						param_flags |= PF_VALUE;
						parameter->int_value = atoi(value.asString().c_str());
						//						LOGV( "param value: %i\n", parameter->intValue );
					}
				}
#if 0
				else if (parameter->type == renderer::MP_SAMPLER_2D)
				{
					param_flags |= PF_VALUE;
					Json::Value texture_param = plist.get("texture", Json::nullValue);
					if (texture_param.isNull())
					{
						LOGW("texture param missing for \"sampler\" type\n");
						param_flags &= ~PF_VALUE;
					}
					if (param_flags & PF_VALUE)
					{
						assets::Texture * tex = assets::load_texture(texture_param.asString().c_str());
						parameter->intValue = tex->Id();
						LOGV("param value: %i\n", parameter->intValue);
						parameter->texture_unit = texture_unit_for_map(parameter->name);
						LOGV("texture unit: %i\n", parameter->texture_unit);
					}
					else
					{
						return util::ConfigLoad_Failure;
					}
				}
#endif
				else if (parameter->type == renderer::MP_SAMPLER_2D)
				{
					param_flags |= PF_VALUE;
					Json::Value texture_unit = plist.get("texture_unit", Json::nullValue);
					Json::Value texture_param = plist.get("texture", Json::nullValue);
					Json::Value filter = plist.get("filter", Json::nullValue);



					if (texture_param.isNull())
					{
						LOGW("texture param missing for \"sampler\" type\n");
						param_flags &= ~PF_VALUE;
					}

					//					if ( texture_unit.isNull() )
					//					{
					//						LOGW( "texture_unit missing for \"sampler\" type\n" );
					//					}

					if (param_flags & PF_VALUE)
					{
						//assets::TextureParameters texparams;

						//if (!filter.isNull())
						//{
						//	const std::string& filter_type = filter.asString();
						//	if (filter_type == "none")
						//	{
						//		texparams.filter_type = image::FILTER_NONE;
						//	}
						//	else if (filter_type == "linear")
						//	{
						//		texparams.filter_type = image::FILTER_LINEAR_MIPMAP;
						//	}
						//}


						//assets::Texture * tex = assets::textures()->load_from_path(texture_param.asString().c_str(), texparams);
						////						parameter->int_value = tex->Id();

						//parameter->texture = tex->texture;
						////						LOGV( "param value: %i\n", parameter->intValue );

						//parameter->texture_unit = texture_unit_for_map(parameter->name);
						////						LOGV( "texture unit: %i\n", parameter->texture_unit );
						LOGV("TODO: support texture parameters when loading from materials\n");
						texture_load(texture_param.asString().c_str());
					}
					else
					{
						return core::util::ConfigLoad_Failure;
					}
				}
				else if (parameter->type == renderer::MP_SAMPLER_CUBE)
				{
					param_flags |= PF_VALUE;
					Json::Value texture_unit = plist.get("texture_unit", Json::nullValue);
					Json::Value texture_param = plist.get("texture", Json::nullValue);


					if (texture_param.isNull())
					{
						LOGW("texture param missing for \"samplerCube\" type\n");
						param_flags &= ~PF_VALUE;
					}

					if (texture_unit.isNull())
					{
						//						LOGW( "texture_unit missing for \"samplerCube\" type\n" );
					}

					if (param_flags & PF_VALUE)
					{
						LOGW("cubemap not implemented!\n");
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
				else if (parameter->type == renderer::MP_VEC4)
				{
					//					param_flags |= PF_VALUE;
					Json::Value value = plist.get("value", Json::nullValue);
					if (value.isNull())
					{
						//						param_flags &= ~PF_VALUE;
						LOGW("Unable to find value for \"vec4\" type\n");
					}
					else
					{
						int results = sscanf(value.asString().c_str(), "%g,%g,%g,%g", &parameter->vector_value[0], &parameter->vector_value[1], &parameter->vector_value[2], &parameter->vector_value[3]);
						if (results < 4)
						{
							LOGW("Unable to parse \"vec4\" type\n");
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
					LOGE("Couldn't find parameter field: \"value\"\n");
					return core::util::ConfigLoad_Failure;
				}
			} // read all shader parameters
		} //  total_parameters


//		material->calculate_requirements();

#if 0
		StackString< MAX_PATH_SIZE > path = "conf/";
		if (!shader.empty())
		{
			path.append(shader.asString().c_str());
		}
		else
		{
			//			path.append( ASSETS_DEFAULT_MATERIAL_SHADER_NAME );
		}
		//		material->shader = loadShader( path() );
		if (!material->shader)
		{
			LOGW("Couldn't find shader \"%s\" for material!\n", shader.asString().c_str());
		}
#endif
		return core::util::ConfigLoad_Success;
	}



	unsigned int material_type_to_parameter_type(const char* name)
	{
		if (core::str::case_insensitive_compare(name, "int", 0) == 0)
		{
			return renderer::MP_INT;
		}
		else if (core::str::case_insensitive_compare(name, "samplerCube", 0) == 0)
		{
			return renderer::MP_SAMPLER_CUBE;
		}
		else if (core::str::case_insensitive_compare(name, "sampler", 0) == 0)
		{
			return renderer::MP_SAMPLER_2D;
		}
		else if (core::str::case_insensitive_compare(name, "vec4", 0) == 0)
		{
			return renderer::MP_VEC4;
		}

		LOGW("Couldn't find material parameter with name: %s\n", name);
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










	MaterialLibrary::MaterialLibrary(Allocator& allocator, render2::Device* render_device)
		: AssetLibrary2(allocator)
		, device(render_device)
	{
	}

	void MaterialLibrary::create_asset(LoadState& state, void* parameters)
	{
		state.asset = MEMORY2_NEW(*state.allocator, Material)(*state.allocator);
	}

	AssetLoadStatus MaterialLibrary::load_asset(LoadState& state, platform::PathString& fullpath, void* parameters)
	{
		LOGV("loading material \"%s\"\n", fullpath());

		platform::PathString asset_uri = fullpath;
		//asset_uri.append(".material");

		if (core::util::json_load_with_callback(asset_uri(), material_load_from_json, &state, true) == core::util::ConfigLoad_Success)
		{
			return AssetLoad_Success;
		}
		return AssetLoad_Failure;
	}

	void MaterialLibrary::destroy_asset(LoadState& state)
	{
		MEMORY2_DELETE(*state.allocator, state.asset);
	}
} // namespace gemini
