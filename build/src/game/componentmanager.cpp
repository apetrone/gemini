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
#include <slim/xlog.h>
#include "componentmanager.hpp"
#include "componentlibrary.hpp"
#include "kernel.hpp"

#include <vector>
#include "components.hpp"



#include "camera.hpp"

#include "particlesystem.hpp"

#include <map>

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
		stream->update();
	}
}

void RenderControl::render_stream( assets::Material * material )
{
//	long offset;
//	rs.save_offset( offset );
	
	assert( material != 0 );

	assets::Shader * shader = assets::find_compatible_shader( attribs + material->requirements );

	assert( shader !=0 );
	rs.add_shader( shader );
	
	glm::mat4 object_matrix;
	
	rs.add_uniform_matrix4( shader->get_uniform_location("modelview_matrix"), &camera.matCam );
	rs.add_uniform_matrix4( shader->get_uniform_location("projection_matrix"), &camera.matProj );
	rs.add_uniform_matrix4( shader->get_uniform_location("object_matrix"), &object_matrix );
	
	rs.add_material( material, shader );
	rs.add_draw_call( stream->vertexbuffer );
	
	rs.run_commands();
	stream->reset();
//	rs.load_offset( offset );
} // render_stream


void RenderControl::render_emitter( ParticleEmitter * emitter )
{
	if ( !emitter || (emitter && !emitter->emitter_config) )
	{
		return;
	}

//	renderer::VertexStream stream;
	
//	stream.desc.add( renderer::VD_FLOAT3 );
//	stream.desc.add( renderer::VD_UNSIGNED_BYTE4 );
//	stream.desc.add( renderer::VD_FLOAT2 );
//	
//	stream.create(1024, 1024, renderer::DRAW_INDEXED_TRIANGLES);

	
	renderer::IndexType indices[] = { 0, 1, 2, 2, 3, 0 };
	glm::mat3 billboard = glm::transpose( glm::mat3(modelview) );
	
	emitter->world_position.interpolate( kernel::instance()->parameters().step_alpha );
	
	
	
	assets::ShaderString name("uv0");
	unsigned int test_attribs = assets::find_parameter_mask( name );
	
	name = "colors";
	test_attribs |= assets::find_parameter_mask(name);
	
	assets::Material * material = 0;
	assets::Shader * shader = 0;
	
	material = assets::materials()->find_with_id(emitter->emitter_config->material_id);
	shader = assets::find_compatible_shader( material->requirements + test_attribs );
	
	glm::mat4 object_matrix;
	
	
	
	
	rs.rewind();
	rs.add_blendfunc(renderer::BLEND_SRC_ALPHA, renderer::BLEND_ONE_MINUS_SRC_ALPHA);
	rs.add_state(renderer::STATE_BLEND, 1);
	rs.add_shader( shader );
	
	rs.add_state(renderer::STATE_DEPTH_TEST, 1);
	rs.add_state(renderer::STATE_DEPTH_WRITE, 0);
	
	rs.add_uniform_matrix4( shader->get_uniform_location("modelview_matrix"), &modelview );
	rs.add_uniform_matrix4( shader->get_uniform_location("projection_matrix"), &projection );
	rs.add_uniform_matrix4( shader->get_uniform_location("object_matrix"), &object_matrix );
	
	rs.add_material( material, shader );
	
	for( unsigned int p = 0; p < emitter->emitter_config->max_particles; ++p )
	{
		Particle * particle = &emitter->particle_list[ p ];
		if ( particle->life_remaining > 0 )
		{
			particle->position.interpolate( kernel::instance()->parameters().step_alpha );
			if ( !stream->has_room(4, 6) )
			{
				// stream is full; need to flush stream here!
				stream->update();
				rs.add_draw_call( stream->vertexbuffer );
				rs.run_commands();
				rs.rewind();
				stream->reset();
			}
			
			SpriteVertexType * v = (SpriteVertexType*)stream->request(4);
			glm::vec3 offset( particle->size, particle->size, 0 );
			glm::vec3 temp;
			
			temp = (billboard * -offset) + particle->position.render;
			v[0].x = temp.x;
			v[0].y = temp.y;
			v[0].z = temp.z;
			
			temp = (billboard * glm::vec3(offset[0], -offset[1], -offset[2])) + particle->position.render;
			v[1].x = temp.x;
			v[1].y = temp.y;
			v[1].z = temp.z;
			
			temp = (billboard * offset) + particle->position.render;
			v[2].x = temp.x;
			v[2].y = temp.y;
			v[2].z = temp.z;
			
			temp = (billboard * glm::vec3(-offset[0], offset[1], -offset[2])) + particle->position.render;
			v[3].x = temp.x;
			v[3].y = temp.y;
			v[3].z = temp.z;
			
			v[0].color = v[1].color = v[2].color = v[3].color = particle->color;
			
			v[0].u = 0; v[0].v = 0;
			v[1].u = 1; v[1].v = 0;
			v[2].u = 1; v[2].v = 1;
			v[3].u = 0; v[3].v = 1;
			
//			debugdraw::point(particle->position.render, Color(255,255,255), particle->size, 0.0f);
			stream->append_indices( indices, 6 );
		}
	}
	
	if (stream->last_vertex > 0)
	{
		stream->update();
		rs.add_draw_call( stream->vertexbuffer );
	}
		
	rs.add_state(renderer::STATE_BLEND, 0);
	rs.add_state(renderer::STATE_DEPTH_WRITE, 1);
	rs.run_commands();
	
//	stream.destroy();
} // render_emitter

namespace ComponentManager
{
	template <class Type>
	struct GenericStepPolicy
	{
		void step( Type * object, float delta_seconds )
		{
			object->step( delta_seconds );
		}
	};
	
	template <class Type>
	struct GenericTickPolicy
	{
		void tick( Type * object, float delta_seconds, float step_alpha )
		{
			object->tick( delta_seconds, step_alpha );
		}
	};

	
	
	ComponentLibrary & get_master()
	{
		static ComponentLibrary s_component_master;
		return s_component_master;
	}

	template <class Type, ComponentType type=MovementComponent, class StepPolicy=GenericStepPolicy<Type>, class TickPolicy=GenericTickPolicy<Type>>
	class ComponentContainer : public GenericComponentContainer
	{
	public:
		typedef std::vector<Type*> TypeVector;
		TypeVector objects;
		
		StepPolicy stepper;
		TickPolicy ticker;
		
		ComponentType get_component_type() const { return type; }
		
		ComponentContainer()
		{
			get_master().register_container(this, get_component_type());
		}
		
		~ComponentContainer()
		{
			purge();
		} // ~ComponentContainer
		
		virtual void purge()
		{
			typename TypeVector::iterator it = objects.begin();
			for( ; it != objects.end(); ++it )
			{
				DESTROY(Type, (*it));
			}
			objects.clear();
		} // purge
		
		virtual IComponent * create()
		{
			Type * object = CREATE(Type);
			objects.push_back(object);
			return object;
		} // create
		
		virtual void destroy( IComponent * object )
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
		
		virtual void for_each( ComponentCallback callback, void * data )
		{
			typename TypeVector::iterator it = objects.begin();
			for( ; it != objects.end(); ++it )
			{
				callback( (*it), data );
			}
		} // for_each
		
		virtual IComponent * find_id(unsigned int id)
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
		
		virtual void step( float delta_seconds )
		{
			typename TypeVector::iterator it = objects.begin();
			for( ; it != objects.end(); ++it )
			{
				stepper.step( (*it), delta_seconds );
			}
		} // step
		
		virtual void tick( float delta_seconds, float step_alpha )
		{
			typename TypeVector::iterator it = objects.begin();

			Type * object;
			for( ; it != objects.end(); )
			{
				ticker.tick((*it), delta_seconds, step_alpha );
				
				object = (*it);
				if ( object->component_flags & C_DISABLE )
				{
					it = objects.erase(it);
					DESTROY(Type, object);
				}
				else
				{
					++it;
				}
			}
		} // tick
	}; // ComponentContainer


	typedef IComponent* (*ComponentCreatorFunction)();
	typedef std::map< std::string, ComponentCreatorFunction > ComponentCreatorByString;
	ComponentCreatorByString & creator_map()
	{
		static ComponentCreatorByString _component_creator_map;
		return _component_creator_map;
	} // creator_map
	
	unsigned int component_type_to_index( ComponentTypes type )
	{
		return (unsigned int)type;
	} // component_type_to_index
	


	// 0: Add typedefs, static vars
	typedef ComponentContainer<Movement, MovementComponent> MovementContainer;
	MovementContainer movement;

	typedef ComponentContainer<InputMovement, InputMovementComponent> InputMovementContainer;
	InputMovementContainer inputmoves;
	
	typedef ComponentContainer<Sprite, SpriteComponent> SpriteContainer;
	SpriteContainer sprite;

	typedef ComponentContainer<Emitter, ParticleEmitterComponent> EmitterContainer;
	EmitterContainer emitters;
	
	typedef ComponentContainer<AABB2Collision, PhysicsComponent> PhysicsContainer;
	PhysicsContainer physics;
	
	
	// 1: Add Container creation
	IComponent * create_type( ComponentType type )
	{
		IComponent * component = 0;
		GenericComponentContainer * gcc = get_master().container_from_type( type );
		if ( gcc )
		{
			component = gcc->create();
		}
	
		return component;
	} // create_type
	
	// 2: Add Container destroy
	void destroy_type( IComponent * component )
	{
		ComponentType type = component->component_type();
		GenericComponentContainer * gcc = get_master().container_from_type( type );
		if ( gcc )
		{
			gcc->destroy( component );
		}
	} // destroy_type
	
	// 3. Add purge
	void purge()
	{
		get_master().purge();
	} // purge

	// 4. (optionally) Add step
	void step( float delta_seconds )
	{
		get_master().step( delta_seconds );
	} // step
	
	// 5. (optionally) Add tick
	void tick( float delta_seconds, float step_alpha )
	{
		get_master().tick( delta_seconds, step_alpha );
	} // tick
	
	// 6. Draw
	void draw( RenderControl & render_control )
	{	
		SpriteContainer::TypeVector::iterator sprite_it = sprite.objects.begin();
		for( ; sprite_it != sprite.objects.end(); ++sprite_it )
		{
			(*sprite_it)->render( render_control );
		}
		
		EmitterContainer::TypeVector::iterator emitter_it = emitters.objects.begin();
		for( ; emitter_it != emitters.objects.end(); ++emitter_it )
		{
			(*emitter_it)->render( render_control );
		}
	} // draw

	// 7. Add for_each support
	void for_each_component(ComponentType type, ComponentCallback callback, void * data)
	{
		GenericComponentContainer * gcc = get_master().container_from_type(type);
		if ( gcc )
		{
			return gcc->for_each( callback, data );
		}
	} // for_each_component
	
	// 8. Add find support
	IComponent * component_matching_id( unsigned int id, ComponentType type )
	{
		IComponent * component = 0;
		GenericComponentContainer * gcc = get_master().container_from_type(type);
		if ( gcc )
		{
			component = gcc->find_id(id);
		}
		return component;
	} // component_matching_id
	
	
	void register_components()
	{
		creator_map().insert(ComponentCreatorByString::value_type("Movement", Movement::creator));
		creator_map().insert(ComponentCreatorByString::value_type("InputMovement", InputMovement::creator));
		
		creator_map().insert(ComponentCreatorByString::value_type("Sprite", Sprite::creator));
		
		creator_map().insert(ComponentCreatorByString::value_type("Emitter", Emitter::creator));
		
		creator_map().insert(ComponentCreatorByString::value_type("Collision", AABB2Collision::creator));
	} // register_components
}; // ComponentManager
