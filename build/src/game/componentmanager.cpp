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
#include "kernel.hpp"

#include <vector>
#include "components.hpp"

namespace ComponentManager
{
	template <class Type>
	class ComponentContainer
	{
	public:
		typedef std::vector<Type*> TypeVector;
		TypeVector objects;
		
		~ComponentContainer()
		{
			purge();
		} // ~ComponentContainer
		
		void purge()
		{
			typename TypeVector::iterator it = objects.begin();
			for( ; it != objects.end(); ++it )
			{
				DESTROY(Type, (*it));
			}
			objects.clear();
		} // purge
		
		Type * create()
		{
			Type * object = CREATE(Type);
			objects.push_back(object);
			return object;
		} // create
		
		void destroy( Type * object )
		{
			if ( !object )
			{
				LOGV( "ComponentContainer could not destroy object! nullptr\n" );
				return;
			}
			
			typename TypeVector::iterator it = objects.begin();
			for( ; it != objects.end(); ++it )
			{
				if ( (*it) == object )
				{
					objects.erase(it);
					DESTROY(Type, (*it));
					break;
				}
			}
		} // destroy
	}; // ComponentContainer

	ComponentContainer<Movement> movement;
	

	ComponentFactory component_factory;
	ComponentVector components[ MaxComponentTypes ];
	
	void register_component( const char * component_name, ComponentFactory::TypeCreator creator )
	{
		LOGV("registering: %s\n", component_name);
		component_factory.register_class(creator, component_name);
	}
	
	IComponent * create_component( const char * component_name )
	{
		ComponentFactory::Record * record = component_factory.find_class( component_name );
		if ( !record )
		{
			LOGW( "No component found with name '%s'\n", component_name );
			return 0;
		}
		
		IComponent * component = record->creator();
		if ( component )
		{
			components[ component->component_type() ].push_back( component );

		}
		else
		{
			LOGW( "Unable to create component from record!\n" );
		}

		
		return component;
	} // create_component
	
	IComponent * create_type( ComponentType type )
	{
		IComponent * component = 0;
		
		
		if ( type == MovementComponent )
		{
			component = movement.create();
		}
	
		return component;
	} // create_type
	
	
	void destroy_type( IComponent * component )
	{
		ComponentType type = component->component_type();
		if ( type == MovementComponent )
		{
			movement.destroy( dynamic_cast<Movement*>(component) );
		}
	} // destroy_type
	
	void purge()
	{
		movement.purge();
		
//		ComponentVector::iterator start, end;
//		for( unsigned int type = 0; type < MaxComponentTypes; ++type )
//		{
//			start = components[type].begin();
//			end = components[type].end();
//			for( ; start != end; ++start )
//			{
//				DESTROY(IComponent, (*start));
//			}
//			components[type].clear();
//		}
	}



	void step( float delta_seconds )
	{
//		ComponentVector::iterator start, end;
//		for( unsigned int type = 0; type < MaxComponentTypes; ++type )
//		{
//			start = components[type].begin();
//			end = components[type].end();
//			for( ; start != end; ++start )
//			{
//				(*start)->step(delta_seconds);
//			}
//		}
	} // step
	
	void tick( float step_alpha )
	{
//		ComponentVector::iterator start, end;
//		for( unsigned int type = 0; type < MaxComponentTypes; ++type )
//		{
//			start = components[type].begin();
//			end = components[type].end();
//			for( ; start != end; ++start )
//			{
//				(*start)->tick(kernel::instance()->parameters().step_alpha);
//			}
//		}
	} // tick
	
	ComponentVector & component_list( ComponentType type )
	{
		assert(type >= 0 && type < MaxComponentTypes);
		return components[type];
	} // component_list
	
}; // ComponentManager



void Movement::step( float delta_seconds )
{
	
} // step

void Movement::tick( float step_alpha )
{
	
} // tick

void Renderable::render( RenderContext & rc )
{
	
} // render