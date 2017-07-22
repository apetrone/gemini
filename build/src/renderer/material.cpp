// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone
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
#include "material.h"

#include <core/logging.h>


namespace renderer
{
	MaterialParameter* Material::parameter_by_name(const std::string& search_name)
	{
		for (MaterialParameter& parameter : parameters)
		{
			if (parameter.name == search_name)
			{
				return &parameter;
			}
		}

		return 0;
	} // parameter_by_name

	void Material::set_parameter_name(unsigned int id, const char* parameter_name)
	{
		this->parameters[id].name = parameter_name;
	} // set_parameter_name

	void Material::set_parameter_vec4(unsigned int id, const glm::vec4& vec)
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
} // namespace renderer
