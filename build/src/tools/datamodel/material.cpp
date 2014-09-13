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

#include "datamodel/material.h"

namespace datamodel
{
	static Material _default_material;

	MaterialMap::MaterialMap()
	{
		_default_material.id = 0;
		_default_material.name = "default";
		next_id = 1;
	}

	const Material& MaterialMap::find_with_id(MaterialId id)
	{
		// no materials; use the default
		if (materials.empty() || id == 0)
		{
			return _default_material;
		}
		
		// catch out of range ids
		assert(materials.size() >= id);
		
		return materials[ (next_id-id-1) ];
	}
	
	const Material& MaterialMap::find_with_name(const std::string& name)
	{
		auto it = materials_by_name.find(name);
		if (it != materials_by_name.end())
		{
			return it->second;
		}
		
		return _default_material;
	}

	const Material& MaterialMap::add_material(const std::string& name)
	{
		Material material;
		material.id = next_id++;
		material.name = name;
		materials.push_back(material);
		materials_by_name.insert(MaterialContainer::value_type(name, material));
		
		return find_with_id(material.id);
	}
}