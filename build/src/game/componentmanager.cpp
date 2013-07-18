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
#include "typedefs.h"
#include "log.h"
#include "componentmanager.hpp"
#include <vector>

IComponent::IComponent(ComponentType component_type)
{
	// register this with the component manager
	ComponentManager::register_component(this, component_type);
}

namespace ComponentManager
{
	typedef std::vector<IComponent*> ComponentVector;
	ComponentVector components[ MaxComponentTypes ];
	
	void purge()
	{
		ComponentVector::iterator start, end;
		for( unsigned int type = 0; type < MaxComponentTypes; ++type )
		{
			start = components[type].begin();
			end = components[type].end();
			for( ; start != end; ++start )
			{
				DESTROY(IComponent, (*start));
			}
		}
	}
	
	void register_component( IComponent * component, ComponentType type )
	{
		components[ type ].push_back(component);
	}
	
	
	void update( float delta_sec )
	{
		
	}
}; // ComponentManager