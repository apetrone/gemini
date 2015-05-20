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
#pragma once

#include <platform/typedefs.h>
#include <core/str.h>

#include <vector>
#include <map>

namespace gemini
{
	namespace datamodel
	{
		typedef int32_t MaterialId;
		struct Material
		{
			MaterialId id;
			String name;
		};

		// Maintains a list of materials
		class MaterialMap
		{
		public:
			typedef std::map<String, Material> MaterialContainer;
			typedef std::vector<Material> MaterialVector;

		private:
			MaterialContainer materials_by_name;
			MaterialVector materials;
			MaterialId next_id;
					
		public:
			MaterialMap();
			virtual ~MaterialMap() {}
			
			Material& find_with_id(MaterialId id);
			Material& find_with_name(const String& name);
			Material& add_material(const String& name);
			size_t size() const { return materials.size(); }
		
			MaterialVector::iterator begin();
			MaterialVector::iterator end();
			MaterialVector::const_iterator begin() const;
			MaterialVector::const_iterator end() const;
		};


		void set_default_material(Material* material);
		Material& get_default_material();
	} // namespace datamodel
} // namespace gemini