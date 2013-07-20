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
		
		void for_each(ComponentCallback callback, void * data)
		{
			typename TypeVector::iterator it = objects.begin();
			for( ; it != objects.end(); ++it )
			{
				callback( (*it), data );
			}
		} // for_each
		
		Type * find_id(unsigned int id)
		{
			Type * object = 0;
			
			typename TypeVector::iterator it = objects.begin();
			for( ; it != objects.end(); ++it )
			{
				if ((*it)->reference_id == id)
				{
					object = (*it);
					break;
				}
			}

			return object;
		} // find_id
	}; // ComponentContainer

	typedef ComponentContainer<Movement> MovementContainer;
	MovementContainer movement;
	
	typedef ComponentContainer<Sprite> SpriteContainer;
	SpriteContainer sprite;
	
	// 1: Add Container creation
	IComponent * create_type( ComponentType type )
	{
		IComponent * component = 0;
		
		switch( type )
		{
			case MovementComponent:
				component = movement.create();
				break;
			case SpriteComponent:
				component = sprite.create();
				break;
				
			default:
				break;
		}

	
		return component;
	} // create_type
	
	// 2: Add Container destroy
	void destroy_type( IComponent * component )
	{
		ComponentType type = component->component_type();
		switch( type )
		{
			case MovementComponent:
				movement.destroy(dynamic_cast<Movement*>(component));
				break;
			case SpriteComponent:
				sprite.destroy(dynamic_cast<Sprite*>(component));
				break;
				
			default:
				break;
		}
	} // destroy_type
	
	// 3. Add purge
	void purge()
	{
		movement.purge();
		sprite.purge();
	} // purge

	// 4. (optionally) Add step
	void step( float delta_seconds )
	{
		MovementContainer::TypeVector::iterator movement_it = movement.objects.begin();
		for( ; movement_it != movement.objects.end(); ++movement_it )
		{
			(*movement_it)->step(delta_seconds);
		}
		
		SpriteContainer::TypeVector::iterator sprite_it = sprite.objects.begin();
		for( ; sprite_it != sprite.objects.end(); ++sprite_it )
		{
			(*sprite_it)->step(delta_seconds);
		}
	} // step
	
	// 5. (optionally) Add tick
	void tick( float step_alpha )
	{
		MovementContainer::TypeVector::iterator movement_it = movement.objects.begin();
		for( ; movement_it != movement.objects.end(); ++movement_it )
		{
			(*movement_it)->tick(step_alpha);
		}
		
		SpriteContainer::TypeVector::iterator sprite_it = sprite.objects.begin();
		for( ; sprite_it != sprite.objects.end(); ++sprite_it )
		{
			(*sprite_it)->render( renderer::driver() );
		}
	} // tick
	
	// 6. Draw
	void draw()
	{	
		SpriteContainer::TypeVector::iterator sprite_it = sprite.objects.begin();
		for( ; sprite_it != sprite.objects.end(); ++sprite_it )
		{
			(*sprite_it)->render( renderer::driver() );
		}
	} // draw

	// 7. Add for_each support
	void for_each_component(ComponentType type, ComponentCallback callback, void * data)
	{
		switch( type )
		{
			case MovementComponent:
				movement.for_each(callback, data);
				break;
			case SpriteComponent:
				sprite.for_each(callback, data);
				break;
				
			default:
				break;
		}
	} // for_each_component
	
	// 8. Add find support
	IComponent * component_matching_id( unsigned int id, ComponentType type )
	{
		IComponent * component = 0;
		switch( type )
		{
			case MovementComponent:
				component = movement.find_id(id);
				break;
			case SpriteComponent:
				component = sprite.find_id(id);
				break;
			default:
				break;
		}
		
		return component;
	} // component_matching_id
}; // ComponentManager



void Movement::step( float delta_seconds )
{
	this->position.step(delta_seconds);
	this->position.current.x += (delta_seconds * this->velocity.x);
	this->position.current.y += (delta_seconds * this->velocity.y);
} // step

void Movement::tick( float step_alpha )
{
	this->position.interpolate(step_alpha);
} // tick