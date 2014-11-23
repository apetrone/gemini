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
#pragma once

#include <platform/typedefs.h>

#include <string>
#include <vector>
#include <map>

namespace datamodel
{
	typedef int32_t MaterialId;
	struct Material
	{
		MaterialId id;
		std::string name;
	};

	// Maintains a list of materials
	class MaterialMap
	{
	public:
		typedef std::map<std::string, Material> MaterialContainer;
		typedef std::vector<Material> MaterialVector;

	private:
		MaterialContainer materials_by_name;
		MaterialVector materials;
		MaterialId next_id;
				
	public:
		MaterialMap();
		virtual ~MaterialMap() {}
		
		Material& find_with_id(MaterialId id);
		Material& find_with_name(const std::string& name);
		Material& add_material(const std::string& name);
		size_t size() const { return materials.size(); }
	
		MaterialVector::iterator begin();
		MaterialVector::iterator end();
		MaterialVector::const_iterator begin() const;
		MaterialVector::const_iterator end() const;
	};
};