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
#include <gemini/mem.h>

#include "entity.h"

EntityListType _entity_list;

EntityListType& entity_list()
{
	return _entity_list;
}

void entity_startup()
{
	Sqrat::RootTable root( script::get_vm() );
	
	// bind Entity to scripting language
	Sqrat::Class<Entity, EntityAllocator<Entity> > entity( script::get_vm() );
	entity.Func( "tick", &Entity::native_tick );
	entity.Func( "step", &Entity::native_step );
	entity.Var( "id", &Entity::id );
	entity.Prop( "name", &Entity::get_name, &Entity::set_name );
	entity.Func( "remove", &Entity::remove );
	
	root.Bind( "Entity", entity );
	
	// gamerules
	Sqrat::Class<GameRules> gamerules( script::get_vm() );
	gamerules.Func( "startup", &GameRules::native_startup );
	gamerules.Func( "tick", &GameRules::native_tick );
	gamerules.Func( "step", &GameRules::native_step );
	root.Bind( "GameRules", gamerules );
	

}

void entity_post_script_load()
{
	// execute script?
	Sqrat::RootTable root( script::get_vm() );
	Sqrat::Object gamerules = root.GetSlot( "gamerules" );
	if ( !gamerules.IsNull() )
	{
		GameRules * gr = gamerules.Cast<GameRules*>();
		if ( gr )
		{
			gr->startup();
		}
	}
}

void entity_step()
{
	Sqrat::RootTable root( script::get_vm() );
	Sqrat::Object gamerules = root.GetSlot( "gamerules" );
	if ( !gamerules.IsNull() )
	{
		GameRules * gr = gamerules.Cast<GameRules*>();
		if ( gr )
		{
			gr->step( kernel::instance()->parameters().step_interval_seconds );
		}
	}
	
	// tick entities
	EntityVector::iterator it =	entity_list().objects.begin();
	EntityVector::iterator end = entity_list().objects.end();
	for( ; it != end; ++it )
	{
		(*it)->step( kernel::instance()->parameters().step_interval_seconds );
	}
}

void entity_deferred_delete( bool only_deferred )
{
	// trim entities flagged for removal
	EntityVector::iterator it = entity_list().objects.begin();
	EntityVector::iterator end = entity_list().objects.end();
	for( ; it != end; ++it )
	{
		Entity * ent = (*it);
		if ((only_deferred && (ent->flags & Entity::EF_DELETE_INSTANCE)) || !only_deferred )
		{
//			LOGV( "removing flagged entity: %p\n", ent );
			it = entity_list().objects.erase( it );
//			ent->flags &= ~Entity::EF_DELETE_INSTANCE;
			delete ent;
		}
	}
}

void entity_tick()
{
	Sqrat::RootTable root( script::get_vm() );
	Sqrat::Object gamerules = root.GetSlot( "gamerules" );
	if ( !gamerules.IsNull() )
	{
		GameRules * gr = gamerules.Cast<GameRules*>();
		if ( gr )
		{
			gr->tick();
		}
	}
	
	// tick entities
	EntityVector::iterator it =	entity_list().objects.begin();
	EntityVector::iterator end = entity_list().objects.end();
	Entity * ent;
	for( ; it != end; ++it )
	{
		ent = (*it);
		if ( !(ent->flags & Entity::EF_DELETE_INSTANCE) )
		{
			(*it)->tick();
		}
	}
	
	entity_deferred_delete( true );
	
}

void entity_shutdown()
{
	LOGV("entity_shutdown!\n");
	entity_list().clear();
}


Entity::Entity()
{
	
	this->type = Logic;
	this->id = entity_list().count();
	this->flags = 0;
	entity_list().add( this );
	//	LOGV( "Entity() - %p, %zu\n", this, this->id );
	
	sq_resetobject( &instance );
	sq_resetobject( &class_object );
	
	// Assumes the OT_INSTANCE is at position 1 in the stack
	SQRESULT res = sq_getstackobj( script::get_vm(), 1, &instance );
	script::check_result(res, "getstackobj");
	
	res = sq_getclass( script::get_vm(), 1 );
	script::check_result(res, "getclass" );
	
	res = sq_getstackobj( script::get_vm(), -1, &class_object );
	script::check_result(res, "getstackobj");
	
	// pop the OT_CLASS
	sq_poptop( script::get_vm() );
	
	this->bind_functions();
} // Entity

Entity::~Entity()
{
	LOGV( "~Entity() - %p, %zu\n", this, this->id );
	entity_list().remove( this );
} // ~Entity

void Entity::step( float delta_seconds )
{
	if ( sq_isnull(this->on_step) || sq_isnull(this->instance) )
	{
		return;
	}
	
	SQRESULT res;
	sq_pushobject( script::get_vm(), this->on_step );
	sq_pushobject( script::get_vm(), this->instance );
	sq_pushfloat( script::get_vm(), delta_seconds );
	res = sq_call( script::get_vm(), 2, SQFalse, SQTrue );
	
	sq_pop( script::get_vm(), 1 );
	if ( SQ_FAILED(res) )
	{
		script::check_result( res, "sq_call" );
		sq_pop( script::get_vm(), 1 );
	}
} // step

void Entity::tick()
{
	if ( sq_isnull(this->on_tick) || sq_isnull(this->instance) )
	{
		return;
	}
	
	SQRESULT res;
	sq_pushobject( script::get_vm(), this->on_tick );
	sq_pushobject( script::get_vm(), this->instance );
	res = sq_call( script::get_vm(), 1, SQFalse, SQTrue );
	
	sq_pop( script::get_vm(), 1 );
	if ( SQ_FAILED(res) )
	{
		script::check_result( res, "sq_call" );
		sq_pop( script::get_vm(), 1 );
	}
} // tick


void Entity::bind_functions()
{
	//	LOGV( "Entity::bind_functions: %p\n", this );
	this->on_tick = script::find_member( this->class_object, "tick" );
	this->on_step = script::find_member( this->class_object, "step" );
} // bind_functions

void Entity::remove()
{
	this->flags = 1;
}

void Entity::native_step( float delta_seconds )
{
} // native_step

void Entity::native_tick()
{
	//	LOGV( "Entity::native_tick\n" );
} // native_tick