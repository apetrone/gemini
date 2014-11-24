// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone

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
#include "material.h"

#include <slim/xlog.h>

namespace renderer
{
	
//	unsigned int texture_unit_for_map( renderer::ShaderString & name );
//	
//	unsigned int material_type_to_parameter_type( const char * name );
//	int material_parameter_type_to_render_state( unsigned int type );

	MaterialParameter * Material::parameter_by_name(const std::string& name)
	{
		for (auto& parameter : parameters)
		{
			if (parameter.name == name)
			{
				return &parameter;
			}
		}
		
		return 0;
	} // parameter_by_name
	
	
	void Material::set_parameter_name( unsigned int id, const char * name )
	{
		this->parameters[id].name = name;
	} // set_parameter_name
	
	void Material::set_parameter_vec4( unsigned int id, const glm::vec4 & vec )
	{
		this->parameters[id].vector_value = vec;
		this->parameters[id].type = MP_VEC4;
	} // set_parameter_vec4
	
	void Material::add_parameter(const MaterialParameter& param)
	{
		parameters.push_back(param);
	}
	
	void Material::print_parameters()
	{
		LOGV("material parameters for %s\n", name.c_str());
		int i = 0;
		for (auto& parameter : parameters)
		{
			LOGV("param %i, %s, %i\n", i, parameter.name.c_str(), parameter.int_value);
			++i;
		}
	}

}; // namespace renderer