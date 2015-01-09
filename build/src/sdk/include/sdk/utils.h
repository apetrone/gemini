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
	} // namespace util
} // namespace gemini