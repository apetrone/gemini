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

#include "datamodel/material.h"

namespace gemini
{
	namespace datamodel
	{
		static Material* _default_material = 0;


		MaterialMap::MaterialMap()
		{
			next_id = 0;
		}

		Material& MaterialMap::find_with_id(MaterialId id)
		{
			// no materials; use the default
			if (materials.empty() || id < 0)
			{
				return get_default_material();
			}

			// catch out of range ids
			assert(materials.size() >= (uint32_t)id);

			return materials[ static_cast<size_t>(id) ];
		}

		Material& MaterialMap::find_with_name(const String& name)
		{
			auto it = materials_by_name.find(name);
			if (it != materials_by_name.end())
			{
				return it->second;
			}

			return get_default_material();
		}

		Material& MaterialMap::add_material(const String& name)
		{
			Material material;
			material.id = next_id++;
			material.name = name;
			materials.push_back(material);
			materials_by_name.insert(MaterialContainer::value_type(name, material));

			return find_with_id(material.id);
		}

		MaterialMap::MaterialVector::iterator MaterialMap::begin()
		{
			return materials.begin();
		}

		MaterialMap::MaterialVector::iterator MaterialMap::end()
		{
			return materials.end();
		}

		MaterialMap::MaterialVector::const_iterator MaterialMap::begin() const
		{
			return materials.begin();
		}

		MaterialMap::MaterialVector::const_iterator MaterialMap::end() const
		{
			return materials.end();
		}

		void set_default_material(Material* material)
		{
			if (material)
			{
				material->id = -1;
				material->name = "default";
				_default_material = material;
			}
		}

		Material& get_default_material()
		{
			assert(_default_material != 0);
			return *_default_material;
		}
	} // namespace datamodel
} // namespace gemini
