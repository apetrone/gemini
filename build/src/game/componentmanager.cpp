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



#include "camera.hpp"

struct SpriteVertexType
{
	float x, y, z;
	Color color;
	float u, v;
};

void add_sprite_to_stream( renderer::VertexStream & vb, int x, int y, int width, int height, const Color & color, float * texcoords )
{
	if ( vb.has_room(4, 6) )
	{
		SpriteVertexType * v = (SpriteVertexType*)vb.request(4);
		float hw = width/2.0f;
		float hh = height/2.0f;
		
		// x and y are assumed to be the center of the sprite
		// upper left corner; moving clockwise
		v[0].x = x - hw;
		v[0].y = y - hh;
		v[0].z = 0;
		v[0].color = color;
		
		v[1].x = x - hw;
		v[1].y = y + hh;
		v[1].z = 0;
		v[1].color = color;
		
		v[2].x = x + hw;
		v[2].y = y + hh;
		v[2].z = 0;
		v[2].color = color;
		
		v[3].x = x + hw;
		v[3].y = y - hh;
		v[3].z = 0;
		v[3].color = color;
		
		v[0].u = texcoords[0];
		v[0].v = texcoords[1];
		v[1].u = texcoords[2];
		v[1].v = texcoords[3];
		v[2].u = texcoords[4];
		v[2].v = texcoords[5];
		v[3].u = texcoords[6];
		v[3].v = texcoords[7];
		
		
		
		//		LOGV( "[%g %g, %g %g, %g %g, %g %g\n", v[0].x, v[0].y, v[1].x, v[1].y, v[2].x, v[2].y, v[3].x, v[3].y );
		
		renderer::IndexType indices[] = { 0, 1, 2, 2, 3, 0 };
		vb.append_indices( indices, 6 );
	}
}

void RenderControl::add_sprite_to_layer( unsigned short layer, int x, int y, int width, int height, const Color & color, float * texcoords )
{
	if ( stream )
	{
		add_sprite_to_stream(*stream, x, y, width, height, color, texcoords);
	}
}

void RenderControl::render_stream( Camera & camera, unsigned int attributes, assets::Material * material )
{
	long offset;
	rs.save_offset( offset );
	
	assert( material != 0 );
	
	stream->update();
	assets::Shader * shader = assets::find_compatible_shader( attributes + material->requirements );
	rs.add_shader( shader );
	
	glm::mat4 object_matrix;
	
	rs.add_uniform_matrix4( shader->get_uniform_location("modelview_matrix"), &camera.matCam );
	rs.add_uniform_matrix4( shader->get_uniform_location("projection_matrix"), &camera.matProj );
	rs.add_uniform_matrix4( shader->get_uniform_location("object_matrix"), &object_matrix );
	
	rs.add_material( material, shader );
	rs.add_draw_call( stream->vertexbuffer );
	
	rs.run_commands();
	stream->reset();
	rs.load_offset( offset );
} // render_stream

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
			(*sprite_it)->tick(step_alpha);
		}
	} // tick
	
	// 6. Draw
	void draw( RenderControl & render_control )
	{	
		SpriteContainer::TypeVector::iterator sprite_it = sprite.objects.begin();
		for( ; sprite_it != sprite.objects.end(); ++sprite_it )
		{
			(*sprite_it)->render( render_control );
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