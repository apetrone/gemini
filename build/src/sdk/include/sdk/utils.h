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

#include <string>
#include <map> // for EntityFactoryRegistrar

#include <sdk/engine_api.h>

class Entity;

namespace gemini
{
	// entity registration

	class IEngineEntity;


	class FactoryClass
	{
	public:
		virtual ~FactoryClass() {};

		virtual IEngineEntity* create() = 0;
	};

	class EntityFactoryRegistrar
	{
	public:
		typedef std::map<std::string, FactoryClass*> FactoryFromClassMap;
		FactoryFromClassMap dictionary;

		void register_class(const std::string& classname, FactoryClass* factory_class)
		{
			dictionary.insert(FactoryFromClassMap::value_type(classname, factory_class));
		}

		FactoryClass* find_factory_by_name(const std::string& classname)
		{
			FactoryFromClassMap::iterator it = dictionary.find(classname);
			if (it != dictionary.end())
			{
				return (*it).second;
			}

			return 0;
		}
	};

	EntityFactoryRegistrar& entity_factory_registrar();



	template <class Type>
	class EntityFactoryClass : public FactoryClass
	{
	public:

		EntityFactoryClass(const char* classname)
		{
			entity_factory_registrar().register_class(classname, this);
		}

		virtual IEngineEntity* create()
		{
			return new Type();
		}
	};

	namespace util
	{
		Entity* create_entity_by_classname(const char* classname);

		float clamp_rotation(float value);
	} // namespace util
} // namespace gemini
